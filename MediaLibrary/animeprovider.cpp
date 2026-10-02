#include "animeprovider.h"
#include "Common/threadtask.h"
#include "Common/network.h"
#include "Common/logger.h"
#include "Extension/Script/scriptmanager.h"
#include "Extension/Script/libraryscript.h"
#include "Extension/Script/matchscript.h"
#include "globalobjects.h"
#include <QImageReader>
#ifdef KSERVICE
#include "Service/kservice.h"
#endif

namespace
{
    const char *setting_MatchScriptId = "Script/DefaultMatchScript";
}

#ifdef KSERVICE
const QString AnimeProvider::kServiceMatchProviderId = QStringLiteral("KikoPlay.Service");
#endif

AnimeProvider::AnimeProvider(QObject *parent) : QObject(parent)
{
    QObject::connect(GlobalObjects::scriptManager, &ScriptManager::scriptChanged, this, [=](ScriptType type){
        if (type == ScriptType::LIBRARY)
        {
            emit infoProviderChanged();
        }
        else if (type == ScriptType::MATCH)
        {
            setMatchProviders();
            if (!matchProviderIds.contains(defaultMatchScriptId))
            {
                if (!matchProviders.empty())
                {
                    setDefaultMatchScript(matchProviders.first().second);
                }
                else
                {
                    defaultMatchScriptId = "";
                    GlobalObjects::appSetting->setValue(setting_MatchScriptId, defaultMatchScriptId);
                    emit defaultMacthProviderChanged("", "");
                }
            }
        }
    });
    setMatchProviders();
    defaultMatchScriptId = GlobalObjects::appSetting->value(setting_MatchScriptId).toString();
    auto script = GlobalObjects::scriptManager->getScript(defaultMatchScriptId).staticCast<MatchScript>();
    if (!script && !matchProviderIds.isEmpty())
    {
        setDefaultMatchScript(matchProviders.first().second);
    }
}

QList<QPair<QString, QString> > AnimeProvider::getSearchProviders()
{
    QList<QPair<QString, QString>> searchProviders;
    for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::LIBRARY))
    {
        LibraryScript *libScript = static_cast<LibraryScript *>(script.data());
        searchProviders.append({libScript->name(), libScript->id()});
    }
    return searchProviders;
}

void AnimeProvider::setDefaultMatchScript(const QString &scriptId)
{
    if(!matchProviderIds.contains(scriptId)) return;
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<MatchScript>();
    if(!script) return;
    defaultMatchScriptId = scriptId;
    GlobalObjects::appSetting->setValue(setting_MatchScriptId, scriptId);
    emit defaultMacthProviderChanged(script->name(), scriptId);
}

ScriptState AnimeProvider::animeSearch(const QString &scriptId, const QString &keyword, const QMap<QString, QString> &options, QList<AnimeLite> &results, TaskContext *ctx)
{
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    Network::ReqAbortFlagObj *abortFlag = nullptr;
    if (ctx)
    {
        QObject::connect(ctx, &TaskContext::cancelRequested, this, [&](){
            if (abortFlag) emit abortFlag->abort();
            if (script) script->stop();
        });
    }
    return task.Run([&](){
        abortFlag = Network::getAbortFlag();
        for (auto iter = options.cbegin(); iter != options.cend(); ++iter)
        {
            script->setSearchOption(iter.key(), iter.value());
        }
        return QVariant::fromValue(script->search(keyword, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getDetail(const AnimeLite &base, Anime *anime, TaskContext *ctx)
{
    auto script = GlobalObjects::scriptManager->getScript(base.scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    Network::ReqAbortFlagObj *abortFlag = nullptr;
    if (ctx)
    {
        QObject::connect(ctx, &TaskContext::cancelRequested, this, [&](){
            if (abortFlag) emit abortFlag->abort();
            if (script) script->stop();
        });
    }
    return task.Run([&](){
        abortFlag = Network::getAbortFlag();
        ScriptState state = script->getDetail(base, anime);
        return QVariant::fromValue(state);
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getEp(Anime *anime, QVector<EpInfo> &results, TaskContext *ctx)
{
    auto script = GlobalObjects::scriptManager->getScript(anime->scriptId()).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    Network::ReqAbortFlagObj *abortFlag = nullptr;
    if (ctx)
    {
        QObject::connect(ctx, &TaskContext::cancelRequested, this, [&](){
            if (abortFlag) emit abortFlag->abort();
            if (script) script->stop();
        });
    }
    return task.Run([&](){
        abortFlag = Network::getAbortFlag();
        return QVariant::fromValue(script->getEp(anime, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getTags(Anime *anime, QStringList &results, TaskContext *ctx)
{
    auto script = GlobalObjects::scriptManager->getScript(anime->scriptId()).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    Network::ReqAbortFlagObj *abortFlag = nullptr;
    if (ctx)
    {
        QObject::connect(ctx, &TaskContext::cancelRequested, this, [&](){
            if (abortFlag) emit abortFlag->abort();
            if (script) script->stop();
        });
    }
    return task.Run([&](){
        abortFlag = Network::getAbortFlag();
        return QVariant::fromValue(script->getTags(anime, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::matchDefault(const QString &path, MatchResult &result)
{
#ifdef KSERVICE
    if (KService::instance()->enableKServiceMatch())
    {
        return kMatch(path, result);
    }
#endif
    return match(defaultMatchScriptId, path, result);
}

ScriptState AnimeProvider::match(const QString &scriptId, const QString &path, MatchResult &result)
{
#ifdef KSERVICE
    if (scriptId == kServiceMatchProviderId) return kMatch(path, result);
#endif
    auto baseScript = GlobalObjects::scriptManager->getScript(scriptId);
    if (!baseScript || baseScript->type() != ScriptType::MATCH)
        return ScriptState(ScriptState::S_ERROR, QStringLiteral("Script invalid"));
    auto script = baseScript.staticCast<MatchScript>();
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    return task.Run([&](){
        return QVariant::fromValue(script->match(path, result));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::menuClick(const QString &mid, Anime *anime)
{
    auto script = GlobalObjects::scriptManager->getScript(anime->scriptId()).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    return task.Run([&](){
        return QVariant::fromValue(script->menuClick(mid, anime));
    }).value<ScriptState>();
}

void AnimeProvider::setMatchProviders()
{
    matchProviderIds.clear();
    matchProviders.clear();
    for (auto &script : GlobalObjects::scriptManager->scripts(ScriptType::MATCH))
    {
        MatchScript *matchScript = static_cast<MatchScript *>(script.data());
        matchProviders.append({matchScript->name(), matchScript->id()});
        matchProviderIds.insert(matchScript->id());
    }
}

#ifdef KSERVICE
ScriptState AnimeProvider::kMatch(const QString &path, MatchResult &result)
{
    QEventLoop eventLoop;
    ScriptState response;
    bool finished = false;
    result.success = false;
    auto conn = QObject::connect(KService::instance(), &KService::recognized, &eventLoop, [&](int status, const QString &errMsg, const QString &filePath, MatchResult match){
        if (filePath != path) return;
        finished = true;
        if (status == 1)
        {
            result = match;
            Logger::logger()->log(Logger::APP, QString("KService Match Success: %1: %2 %3").arg(path, result.name, result.ep.toString()));
        }
        else
        {
            response = ScriptState(ScriptState::S_ERROR, errMsg);
        }
        eventLoop.quit();
    });
    KService::instance()->fileRecognize(path);
    if (!finished) eventLoop.exec();
    QObject::disconnect(conn);
    return response;
}
#endif

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
    if(defaultMatchScript().isEmpty() && !matchProviderIds.isEmpty())
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

ScriptState AnimeProvider::animeSearch(const QString &scriptId, const QString &keyword, const QMap<QString, QString> &options, QList<AnimeLite> &results)
{
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    return task.Run([&](){
        for(auto iter = options.cbegin(); iter != options.cend(); ++iter)
        {
            script->setSearchOption(iter.key(), iter.value());
        }
        return QVariant::fromValue(script->search(keyword, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getDetail(const AnimeLite &base, Anime *anime)
{
    auto script = GlobalObjects::scriptManager->getScript(base.scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    return task.Run([&](){
        ScriptState state = script->getDetail(base, anime);
        return QVariant::fromValue(state);
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getEp(Anime *anime, QVector<EpInfo> &results)
{
    auto script = GlobalObjects::scriptManager->getScript(anime->scriptId()).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    return task.Run([&](){
        return QVariant::fromValue(script->getEp(anime, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getTags(Anime *anime, QStringList &results)
{
    auto script = GlobalObjects::scriptManager->getScript(anime->scriptId()).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::scriptManager->scriptThread);
    return task.Run([&](){
        return QVariant::fromValue(script->getTags(anime, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::match(const QString &scriptId, const QString &path, MatchResult &result)
{
#ifdef KSERVICE
    if (KService::instance()->enableKServiceMatch())
    {
        return kMatch(scriptId, path, result);
    }
#endif
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<MatchScript>();
    if(!script) return "Script invalid";
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
ScriptState AnimeProvider::kMatch(const QString &scriptId, const QString &path, MatchResult &result)
{
    QEventLoop eventLoop;
    QSharedPointer<MatchStatusObj> statusFlag{new MatchStatusObj, &MatchStatusObj::deleteLater};
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<MatchScript>();
    if (script) statusFlag->scriptValid = true;
    QObject::connect(statusFlag.get(), &MatchStatusObj::quit, &eventLoop, &QEventLoop::quit);

    auto conn = QObject::connect(KService::instance(), &KService::recognized, this, [=](int status, const QString &errMsg, const QString &filePath, MatchResult match){
        if (filePath != path) return;
        if (status != 1)
        {
            if (!statusFlag->scriptValid) statusFlag->quitEventLoop();
            return;
        }
        if (!statusFlag->downFlag)
        {
            statusFlag->downFlag = true;
            statusFlag->kServiceSuccess = true;
            statusFlag->kServiceMatch = match;
            statusFlag->quitEventLoop();
        }
    });
    KService::instance()->fileRecognize(path);

    Network::ReqAbortFlagObj *abortFlag = nullptr;
    ScriptState scriptRsp;
    QObject obj;
    if (script)
    {
        obj.moveToThread(GlobalObjects::scriptManager->scriptThread);
        QMetaObject::invokeMethod(&obj, [=, &abortFlag, &scriptRsp](){
            abortFlag = Network::getAbortFlag();
            MatchResult scriptMatchResult;
            ScriptState rsp = script->match(path, scriptMatchResult);
            QThread::msleep(400);  // wait KService 400ms
            if (scriptRsp && !statusFlag->downFlag)
            {
                statusFlag->downFlag = true;
                statusFlag->scriptSuccess = true;
                statusFlag->scriptMatch = scriptMatchResult;
                scriptRsp = rsp;
            }
            statusFlag->quitEventLoop();
        }, Qt::QueuedConnection);
    }
    eventLoop.exec();
    QObject::disconnect(conn);

    if (statusFlag->downFlag)
    {
        if (statusFlag->kServiceSuccess)
        {
            script->stop();
            if (abortFlag) emit abortFlag->abort();
            Logger::logger()->log(Logger::APP, QString("KService Match Success: %1: %2 %3").arg(path, result.name, result.ep.toString()));
            result = statusFlag->kServiceMatch;
            return ScriptState(ScriptState::S_NORM);
        }
        else if (statusFlag->scriptSuccess)
        {
            result = statusFlag->scriptMatch;
            return ScriptState(ScriptState::S_NORM);
        }
    }
    result.success = false;
    return scriptRsp;
}
#endif

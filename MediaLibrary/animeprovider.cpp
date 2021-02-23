#include "animeprovider.h"
#include "Common/threadtask.h"
#include "Common/network.h"
#include "Script/scriptmanager.h"
#include "Script/libraryscript.h"
#include "globalobjects.h"
namespace
{
    const char *setting_MatchScriptId = "Script/DefaultMatchScript";
}

AnimeProvider::AnimeProvider(QObject *parent) : QObject(parent)
{
    QObject::connect(GlobalObjects::scriptManager, &ScriptManager::scriptChanged, this, [=](ScriptType type){
        if(type==ScriptType::LIBRARY)
        {
            setMacthProviders();
            if(!matchProviderIds.contains(defaultMatchScriptId))
            {
                if(matchProviders.size()>0)
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
            emit matchProviderChanged();
        }
    });
    setMacthProviders();
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
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<LibraryScript>();
    if(!script) return;
    defaultMatchScriptId = scriptId;
    GlobalObjects::appSetting->setValue(setting_MatchScriptId, scriptId);
    emit defaultMacthProviderChanged(script->name(), scriptId);
}

ScriptState AnimeProvider::animeSearch(const QString &scriptId, const QString &keyword, QList<AnimeLite> &results)
{
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->search(keyword, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getDetail(const AnimeLite &base, Anime *anime)
{
    auto script = GlobalObjects::scriptManager->getScript(base.scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        ScriptState state = script->getDetail(base, anime);
        if(state)
        {
            if(QUrl(anime->coverURL()).isLocalFile())
            {
                QFile coverFile(anime->coverURL());
                if(coverFile.open(QIODevice::ReadOnly))
                    anime->_cover.loadFromData(coverFile.readAll());
                anime->_coverURL = "";
            }
            else
            {
                try {
                    anime->_cover.loadFromData(Network::httpGet(anime->coverURL(), QUrlQuery()));
                } catch (Network::NetworkError &error) {

                }
            }
            QStringList urls;
            QList<QUrlQuery> querys;
            QList<Character *> crts;
            for(auto &crt : anime->characters)
            {
                if(QUrl(crt.imgURL).isLocalFile())
                {
                    QFile crtImageFile(crt.imgURL);
                    if(crtImageFile.open(QIODevice::ReadOnly))
                        crt.image.loadFromData(crtImageFile.readAll());
                    crt.imgURL = "";
                }
                else
                {
                    urls.append(crt.imgURL);
                    querys.append(QUrlQuery());
                    crts.append(&crt);
                }
            }
            try
            {
                QList<QPair<QString, QByteArray> > results(Network::httpGetBatch(urls,querys));
                for(int i = 0; i<crts.size(); ++i)
                {
                    if(results[i].first.isEmpty())
                    {
                        crts[i]->image.loadFromData(results[i].second);
                    }
                }
            } catch (Network::NetworkError &error) {

            }

        }
        return QVariant::fromValue(state);
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getEp(Anime *anime, QList<EpInfo> &results)
{
    auto script = GlobalObjects::scriptManager->getScript(anime->scriptId()).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->getEp(anime, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getTags(Anime *anime, QStringList &results)
{
    auto script = GlobalObjects::scriptManager->getScript(anime->scriptId()).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->getTags(anime, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::match(const QString &scriptId, const QString &path, MatchResult &result)
{
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->match(path, result));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::menuClick(const QString &mid, Anime *anime)
{
    auto script = GlobalObjects::scriptManager->getScript(anime->scriptId()).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->menuClick(mid, anime));
    }).value<ScriptState>();
}

void AnimeProvider::setMacthProviders()
{
    matchProviderIds.clear();
    matchProviders.clear();
    for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::LIBRARY))
    {
        LibraryScript *libScript = static_cast<LibraryScript *>(script.data());
        if(libScript->supportMatch())
        {
            matchProviders.append({libScript->name(), libScript->id()});
            matchProviderIds.insert(libScript->id());
        }
    }
}

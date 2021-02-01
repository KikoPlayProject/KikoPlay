#include "animeprovider.h"
#include "Common/threadtask.h"
#include "Script/scriptmanager.h"
#include "Script/libraryscript.h"
#include "globalobjects.h"

AnimeProvider::AnimeProvider(QObject *parent) : QObject(parent)
{
    QObject::connect(GlobalObjects::scriptManager, &ScriptManager::scriptChanged, this, [=](ScriptType type, const QString &id){
        if(type==ScriptType::LIBRARY && matchProviderIds.contains(id))
        {
            emit matchProviderChanged();
        }
    });
}

QList<QPair<QString, QString> > AnimeProvider::getSearchProviders()
{
    QList<QPair<QString, QString>> searchProviders;
    for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::LIBRARY))
    {
        LibraryScript *libScript = static_cast<LibraryScript *>(script.data());
        searchProviders.append({libScript->id(), libScript->name()});
    }
    return searchProviders;
}

QList<QPair<QString, QString> > AnimeProvider::getMatchProviders()
{
    QList<QPair<QString, QString>> matchProviders;
    matchProviderIds.clear();
    for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::LIBRARY))
    {
        LibraryScript *libScript = static_cast<LibraryScript *>(script.data());
        if(libScript->supportMatch())
        {
            matchProviders.append({libScript->id(), libScript->name()});
            matchProviderIds.insert(libScript->id());
        }
    }
    return matchProviders;
}

ScriptState AnimeProvider::animeSearch(const QString &scriptId, const QString &keyword, QList<AnimeBase> &results)
{
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->search(keyword, results));
    }).value<ScriptState>();
}

ScriptState AnimeProvider::getDetail(const AnimeBase &base, Anime *anime)
{
    auto script = GlobalObjects::scriptManager->getScript(base.scriptId).staticCast<LibraryScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->getDetail(base, anime));
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

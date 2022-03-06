#include "danmuprovider.h"
#include "Common/threadtask.h"
#include "Script/scriptmanager.h"
#include "Script/danmuscript.h"
#include "Manager/danmumanager.h"
#include "Manager/pool.h"
#include "globalobjects.h"

DanmuProvider::DanmuProvider(QObject *parent) : QObject(parent)
{

}

QList<QPair<QString, QString>> DanmuProvider::getSearchProviders()
{
    QList<QPair<QString, QString>> searchProviders;
    for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::DANMU))
    {
        DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
        if(dmScript->supportSearch())
        {
            searchProviders.append({dmScript->name(), dmScript->id()});
        }
    }
    return searchProviders;
}

QStringList DanmuProvider::getSampleURLs()
{
    QStringList sampledURLs;
    for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::DANMU))
    {
        DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
        sampledURLs.append(dmScript->sampleURLs());
    }
    return sampledURLs;
}

ScriptState DanmuProvider::danmuSearch(const QString &scriptId, const QString &keyword, QList<DanmuSource> &results)
{
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<DanmuScript>();
    if(!script || !script->supportSearch()) return "Script invalid or Unsupport search";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->search(keyword, results));
    }).value<ScriptState>();
}

ScriptState DanmuProvider::getEpInfo(const DanmuSource *source, QList<DanmuSource> &results)
{
    auto script = GlobalObjects::scriptManager->getScript(source->scriptId).staticCast<DanmuScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        return QVariant::fromValue(script->getEpInfo(source, results));
    }).value<ScriptState>();
}

ScriptState DanmuProvider::getURLInfo(const QString &url, QList<DanmuSource> &results)
{
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::DANMU))
        {
            DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
            if(dmScript->supportURL(url))
            {
                return QVariant::fromValue(dmScript->getURLInfo(url, results));
            }
        }
        return QVariant::fromValue(ScriptState(ScriptState::S_ERROR, "Unsupported URL"));
    }).value<ScriptState>();
}

ScriptState DanmuProvider::downloadDanmu(const DanmuSource *item, QVector<DanmuComment *> &danmuList, DanmuSource **nItem)
{
    auto script = GlobalObjects::scriptManager->getScript(item->scriptId).staticCast<DanmuScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        DanmuSource *retItem = nullptr;
        ScriptState state = script->getDanmu(item, &retItem, danmuList);
        if(nItem) *nItem = retItem;
        return QVariant::fromValue(state);
    }).value<ScriptState>();
}

void DanmuProvider::checkSourceToLaunch(const QString &poolId)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QStringList supportedScripts;
        Pool *pool = GlobalObjects::danmuManager->getPool(poolId, false);
        if(!pool)
        {
            emit sourceCheckDown(poolId, supportedScripts);
            return;
        }
        QList<DanmuSource> sources;
        for(auto &src : pool->sources())
            sources.append(src);
        if(sources.size()==0)
        {
            emit sourceCheckDown(poolId, supportedScripts);
            return;
        }
        for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::DANMU))
        {
            DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
            bool ret = false;
            dmScript->hasSourceToLaunch(sources, ret);
            if(ret) supportedScripts.append(dmScript->id());
        }
        emit sourceCheckDown(poolId, supportedScripts);
    });
}

void DanmuProvider::launch(const QStringList &ids, const QString &poolId, const QList<DanmuSource> &sources, DanmuComment *comment)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QStringList status;
        for(auto &id : ids)
        {
            auto script =  GlobalObjects::scriptManager->getScript(id);
            DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
            ScriptState state = dmScript->launch(sources, comment);
            status.append(state.info);
        }
        emit danmuLaunchStatus(poolId, ids, status, comment);
    });
}


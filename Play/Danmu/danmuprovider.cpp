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

ScriptState DanmuProvider::downloadDanmu(const DanmuSource *item, QList<DanmuComment *> &danmuList, DanmuSource **nItem)
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


/*
DanmuAccessResult *ProviderManager::search(const QString &providerId, QString keyword)
{
    ProviderBase *provider=providers.value(providerId,nullptr);
    if(!provider || !provider->supportSearch())
    {
        DanmuAccessResult *result=new DanmuAccessResult;
        result->error=true;
        result->errorInfo=tr("Provider invalid or Unsupport search");
        return result;
    }
    QEventLoop eventLoop;
    DanmuAccessResult *curSearchInfo=nullptr;
    QObject::connect(provider,&ProviderBase::searchDone, &eventLoop, [&eventLoop,&curSearchInfo](DanmuAccessResult *searchInfo){
        curSearchInfo=searchInfo;
        eventLoop.quit();
    });
    QMetaObject::invokeMethod(provider, [provider,&keyword]() {
        provider->search(keyword);
    }, Qt::QueuedConnection);
    eventLoop.exec();
    if(!curSearchInfo)
    {
        DanmuAccessResult *result=new DanmuAccessResult;
        result->error=true;
        result->errorInfo=tr("Search Failed");
        return result;
    }
    return curSearchInfo;
}

DanmuAccessResult *ProviderManager::getEpInfo(const QString &providerId, DanmuSourceItem *item)
{
    ProviderBase *provider=providers.value(providerId,nullptr);
    if(!provider)
    {
        DanmuAccessResult *result=new DanmuAccessResult;
        result->error=true;
        result->errorInfo=tr("Provider invalid");
        return result;
    }
    QEventLoop eventLoop;
    DanmuAccessResult *curEpInfo=nullptr;
    QObject::connect(provider,&ProviderBase::epInfoDone, &eventLoop, [&eventLoop,&curEpInfo,item](DanmuAccessResult *epInfo,DanmuSourceItem *srcItem){
		if (item == srcItem)
		{
			curEpInfo = epInfo;
			eventLoop.quit();
		}
    });
    QMetaObject::invokeMethod(provider,"getEpInfo",Qt::QueuedConnection,Q_ARG(DanmuSourceItem *,item));
    eventLoop.exec();
    if(!curEpInfo)
    {
        DanmuAccessResult *result=new DanmuAccessResult;
        result->error=true;
        result->errorInfo=tr("Get EpInfo Failed");
        return result;
    }
    return curEpInfo;
}

DanmuAccessResult *ProviderManager::getURLInfo(QString &url)
{
    for(auto iter=providers.cbegin();iter!=providers.cend();++iter)
    {
        DanmuAccessResult *result=iter.value()->getURLInfo(url);
        if(result)return result;
    }
    DanmuAccessResult *result=new DanmuAccessResult;
    result->error=true;
    result->errorInfo=tr("Unsupported URL");
    return result;
}

QString ProviderManager::downloadDanmu(QString &providerId, DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    ProviderBase *provider=providers.value(providerId,nullptr);
    if(!provider)
    {
        return tr("Provider invalid");
    }
    QEventLoop eventLoop;
    QString errorInfo;
    QObject::connect(provider,&ProviderBase::downloadDone, &eventLoop, [&eventLoop,&errorInfo,item](QString errInfo,DanmuSourceItem *srcItem){
        if (item == srcItem)
        {
            errorInfo=errInfo;
            eventLoop.quit();
        }
    });
    QMetaObject::invokeMethod(provider,[provider,item,&danmuList](){
        provider->downloadDanmu(item,danmuList);
    },Qt::QueuedConnection);
    eventLoop.exec();
    return errorInfo;
}

QString ProviderManager::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    QString providerId = getProviderIdByURL(url);
    if(providers.contains(providerId))
    {
        return providers[providerId]->downloadBySourceURL(url, danmuList);
    }
    return tr("Unsupported Source");
}

QString ProviderManager::getProviderIdByURL(const QString &url)
{
    for(auto iter=providers.cbegin();iter!=providers.cend();++iter)
    {
        if(iter.value()->supportSourceURL(url))
            return iter.value()->id();
    }
    return QString();
}
*/

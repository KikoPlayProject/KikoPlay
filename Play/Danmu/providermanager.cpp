#include "providermanager.h"
#include "Provider/providerbase.h"
#include "Provider/bilibiliprovider.h"
#include "Provider/tucaoprovider.h"
#include "Provider/dililiprovider.h"
#include "Provider/dandanprovider.h"
#include "Provider/bahamutprovider.h"
#include "Provider/iqiyiprovider.h"
#include "globalobjects.h"

ProviderManager::ProviderManager(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<DanmuSourceItem *>("DanmuSourceItem*");

	registerProvider<BilibiliProvider>();
	registerProvider<TucaoProvider>();
    registerProvider<DililiProvider>();
	registerProvider<DandanProvider>();
	registerProvider<BahamutProvider>();
    registerProvider<IqiyiProvider>();
}

QStringList ProviderManager::getSearchProviders()
{
    QStringList searchProviders;
    for(ProviderBase *provider:orderedProviders)
    {
        if(provider->supportSearch())
            searchProviders.append(provider->id());
    }
    return searchProviders;
}

QStringList ProviderManager::getSupportedURLs()
{
    QStringList supportedURLs;
    for(ProviderBase *provider:orderedProviders)
    {
        supportedURLs+=provider->supportedURLs();
    }
    return supportedURLs;
}

QString ProviderManager::getSourceURL(const QString &providerId, DanmuSourceItem *item)
{
    ProviderBase *provider=providers.value(providerId,nullptr);
    return provider?provider->sourceURL(item):QString();
}

DanmuAccessResult *ProviderManager::search(const QString &providerId, QString keyword)
{
    ProviderBase *provider=providers.value(providerId,nullptr);
    if(!provider || !provider->supportSearch())
    {
        DanmuAccessResult *result=new DanmuAccessResult;
        result->error=true;
        result->errorInfo=tr("Provider invaild or Unsupport search");
        return result;
    }
    QMetaObject::invokeMethod(provider, [provider,&keyword]() {
		provider->search(keyword);
	}, Qt::QueuedConnection);
    QEventLoop eventLoop;
    DanmuAccessResult *curSearchInfo=nullptr;
    QObject::connect(provider,&ProviderBase::searchDone, &eventLoop, [&eventLoop,&curSearchInfo](DanmuAccessResult *searchInfo){
        curSearchInfo=searchInfo;
        eventLoop.quit();
    });
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
        result->errorInfo=tr("Provider invaild");
        return result;
    }
    QMetaObject::invokeMethod(provider,"getEpInfo",Qt::QueuedConnection,Q_ARG(DanmuSourceItem *,item));
    QEventLoop eventLoop;
    DanmuAccessResult *curEpInfo=nullptr;
    QObject::connect(provider,&ProviderBase::epInfoDone, &eventLoop, [&eventLoop,&curEpInfo,item](DanmuAccessResult *epInfo,DanmuSourceItem *srcItem){
		if (item == srcItem)
		{
			curEpInfo = epInfo;
			eventLoop.quit();
		}
    });
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
        return tr("Provider invaild");
    }
    QMetaObject::invokeMethod(provider,[provider,item,&danmuList](){
        provider->downloadDanmu(item,danmuList);
    },Qt::QueuedConnection);
    QEventLoop eventLoop;
    QString errorInfo;
    QObject::connect(provider,&ProviderBase::downloadDone, &eventLoop, [&eventLoop,&errorInfo](QString errInfo){
        errorInfo=errInfo;
        eventLoop.quit();
    });
    eventLoop.exec();
    return errorInfo;
}

QString ProviderManager::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    for(auto iter=providers.cbegin();iter!=providers.cend();++iter)
    {
        if(iter.value()->supportSourceURL(url))
            return iter.value()->downloadBySourceURL(url,danmuList);
    }
    return tr("Unsupported Source");
}

#ifndef PROVIDERMANAGER_H
#define PROVIDERMANAGER_H

#include <QObject>
#include "common.h"
#include "Provider/info.h"
class ProviderBase;
class ProviderManager : public QObject
{
    Q_OBJECT
public:
    explicit ProviderManager(QObject *parent = nullptr);
    QStringList getSearchProviders();
    QStringList getSupportedURLs();
    QString getSourceURL(const QString &providerId,DanmuSourceItem *item);
    DanmuAccessResult *search(const QString &providerId,QString keyword);
    DanmuAccessResult *getEpInfo(const QString &providerId,DanmuSourceItem *item);
    DanmuAccessResult *getURLInfo(QString &url);
    QString downloadDanmu(QString &providerId,DanmuSourceItem *item,QList<DanmuComment *> &danmuList);
    QString downloadBySourceURL(const QString &url,QList<DanmuComment *> &danmuList);
private:
    QMap<QString,ProviderBase *> providers;
    QList<ProviderBase *> orderedProviders;

    template<typename T>
    void registerProvider()
    {
        T *provider=new T();
        provider->moveToThread(GlobalObjects::workThread);
        providers.insert(provider->id(),provider);
        orderedProviders.append(provider);
    }
};

#endif // PROVIDERMANAGER_H

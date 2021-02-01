#ifndef PROVIDERMANAGER_H
#define PROVIDERMANAGER_H

#include <QObject>
#include <search.h>
#include "Script/scriptbase.h"
#include "common.h"
#include "Provider/info.h"
#ifndef Q_OS_WIN
#include "globalobjects.h"
#endif
class ProviderBase;
class DanmuProvider : public QObject
{
    Q_OBJECT
public:
    DanmuProvider(QObject *parent = nullptr);

    QList<QPair<QString, QString>> getSearchProviders();
    QStringList getSampleURLs();

    ScriptState danmuSearch(const QString &scriptId, const QString &keyword, QList<DanmuSource> &results);
    ScriptState getEpInfo(const DanmuSource *source, QList<DanmuSource> &results);
    ScriptState getURLInfo(const QString &url, QList<DanmuSource> &results);
    ScriptState downloadDanmu(const DanmuSource *item, QList<DanmuComment *> &danmuList, DanmuSource **nItem=nullptr);
    void checkSourceToLaunch(const QString &poolId, const QList<DanmuSource> &sources);
    void launch(const QStringList &ids, const QString &poolId, const QList<DanmuSource> &sources, DanmuComment *comment);

    //DanmuAccessResult *getURLInfo(QString &url);
    //QString downloadDanmu(QString &providerId,DanmuSourceItem *item,QList<DanmuComment *> &danmuList);
    //QString downloadBySourceURL(const QString &url,QList<DanmuComment *> &danmuList);
    //QString getProviderIdByURL(const QString &url);
signals:
    void sourceCheckDown(const QString &poolId, const QStringList &supportedScripts);
    void danmuLaunchStatus(const QString &poolId, const QStringList &scriptIds, const QStringList &status, DanmuComment *comment);
private:
    //QMap<QString,ProviderBase *> providers;
    //QList<ProviderBase *> orderedProviders;
    /*
    template<typename T>
    void registerProvider()
    {
        T *provider=new T();
        provider->moveToThread(GlobalObjects::workThread);
        providers.insert(provider->id(),provider);
        orderedProviders.append(provider);
    }
    */
};

#endif // PROVIDERMANAGER_H

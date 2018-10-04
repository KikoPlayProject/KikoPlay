#ifndef ACFUNPROVIDER_H
#define ACFUNPROVIDER_H

#include "providerbase.h"

class AcfunProvider : public ProviderBase
{
    Q_OBJECT
public:
    explicit AcfunProvider(QObject *parent = nullptr):ProviderBase(parent){}
    // ProviderBase interface
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "Acfun";}
    virtual QStringList supportedURLs();
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("acfun:%1").arg(item->id);}
    inline virtual bool supportSourceURL(const QString &url){return url.startsWith("acfun:")?true:false;}
public slots:
    virtual DanmuAccessResult *search(const QString &keyword);
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item);
    virtual DanmuAccessResult *getURLInfo(const QString &url);
    virtual QString downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList);
    virtual QString downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList);
private:
    void handleSearchReply(QJsonDocument &document, DanmuAccessResult *searchResult);
    void handleSp(DanmuSourceItem *item,DanmuAccessResult *result,int pageNo=1);
    void handleAi(DanmuSourceItem *item,DanmuAccessResult *result);
    void downloadAllDanmu(QList<DanmuComment *> &danmuList,int videoId);
};

#endif // ACFUNPROVIDER_H

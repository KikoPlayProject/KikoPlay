#ifndef TENCENTPROVIDER_H
#define TENCENTPROVIDER_H
#include "providerbase.h"

class TencentProvider : public ProviderBase
{
public:
    explicit TencentProvider(QObject *parent=nullptr):ProviderBase(parent){}
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "Tencent";}
    virtual QStringList supportedURLs();
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("tencent:%1;length:%2;%3").arg(item->strId).arg(item->subId).arg(formatTime(item->extra));}
    inline virtual bool supportSourceURL(const QString &url){return url.startsWith("tencent:")?true:false;}

public slots:
    virtual DanmuAccessResult *search(const QString &keyword);
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item);
    virtual DanmuAccessResult *getURLInfo(const QString &url);
    virtual QString downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList);
    virtual QString downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList);
private:
    void downloadAllDanmu(const QString &id, int length, QList<DanmuComment *> &danmuList);
    void handleSearchReply(QString &reply,DanmuAccessResult *result);
};

#endif // TENCENTPROVIDER_H

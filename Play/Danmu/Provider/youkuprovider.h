#ifndef YOUKUPROVIDER_H
#define YOUKUPROVIDER_H
#include "providerbase.h"

class YoukuProvider : public ProviderBase
{
public:
    explicit YoukuProvider(QObject *parent=nullptr):ProviderBase(parent){}
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "Youku";}
    virtual QStringList supportedURLs();
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("youku:%1;length:%2").arg(item->strId).arg(item->subId);}
    inline virtual bool supportSourceURL(const QString &url){return url.startsWith("youku:")?true:false;}

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

#endif // YOUKUPROVIDER_H

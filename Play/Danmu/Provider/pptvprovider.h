#ifndef PPTVPROVIDER_H
#define PPTVPROVIDER_H
#include "providerbase.h"

class PPTVProvider : public ProviderBase
{
public:
    explicit PPTVProvider(QObject *parent=nullptr):ProviderBase(parent){}
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "PPTV";}
    virtual QStringList supportedURLs();
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("pptv:%1;length:%2").arg(item->strId).arg(item->subId);}
    inline virtual bool supportSourceURL(const QString &url){return url.startsWith("pptv:")?true:false;}

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

#endif // PPTVPROVIDER_H

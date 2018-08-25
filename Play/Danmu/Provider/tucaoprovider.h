#ifndef TUCAOPROVIDER_H
#define TUCAOPROVIDER_H
#include "providerbase.h"

class TucaoProvider : public ProviderBase
{
    Q_OBJECT
public:
    explicit TucaoProvider(QObject *parent=nullptr):ProviderBase(parent){}
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "Tucao";}
    virtual QStringList supportedURLs();
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("hid:%1;p:%2").arg(item->id).arg(item->subId);}
    inline virtual bool supportSourceURL(const QString &url){return url.startsWith("hid:")?true:false;}
public slots:
    virtual DanmuAccessResult *search(const QString &keyword);
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item);
    virtual DanmuAccessResult *getURLInfo(const QString &url);
    virtual QString downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList);
    virtual QString downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList);
private:
    void handleSearchReply(QString &reply,DanmuAccessResult *result);
    void handleEpReply(QString &reply,DanmuSourceItem *item, DanmuAccessResult *result);
    void handleDownloadReply(QString &reply,QList<DanmuComment *> &danmuList);
};
#endif // TUCAOPROVIDER_H

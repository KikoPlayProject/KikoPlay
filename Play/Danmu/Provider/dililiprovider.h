#ifndef DILILIPROVIDER_H
#define DILILIPROVIDER_H
#include "providerbase.h"
class DililiProvider : public ProviderBase
{
    Q_OBJECT
public:
    explicit DililiProvider(QObject *parent = nullptr):ProviderBase(parent){}
    // ProviderBase interface
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "5dm";}
    virtual QStringList supportedURLs();
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("5dm:%1").arg(item->strId);}
    inline virtual bool supportSourceURL(const QString &url){return url.startsWith("5dm:")?true:false;}

public slots:
    virtual DanmuAccessResult *search(const QString &keyword);
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item);
    virtual DanmuAccessResult *getURLInfo(const QString &url);
    virtual QString downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList);
    virtual QString downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList);
private:
    void handleSearchReply(const QString &reply,DanmuAccessResult *result);
    void handleEpReply(const QString &reply, DanmuAccessResult *result);
    void handleDownloadReply(const QString &reply, QList<DanmuComment *> &danmuList);
};
#endif // DILILIPROVIDER_H

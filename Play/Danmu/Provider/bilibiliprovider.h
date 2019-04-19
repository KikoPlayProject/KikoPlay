#ifndef BILIBILIPROVIDER_H
#define BILIBILIPROVIDER_H
#include "providerbase.h"
class BilibiliProvider : public ProviderBase
{
    Q_OBJECT
public:
    explicit BilibiliProvider(QObject *parent=nullptr):ProviderBase(parent){}
    // ProviderBase interface
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "Bilibili";}
    virtual QStringList supportedURLs();
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("aid:%1;cid:%2;%3").arg(item->id).arg(item->subId).arg(formatTime(item->extra));}
    inline virtual bool supportSourceURL(const QString &url){return url.startsWith("aid:")?true:false;}
public slots:
    virtual DanmuAccessResult *search(const QString &keyword);
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item);
    virtual DanmuAccessResult *getURLInfo(const QString &url);
    virtual QString downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList);
    virtual QString downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList);
private:
    void handleSearchReply(QJsonDocument &document, DanmuAccessResult *searchResult);
    void handleBangumiReply(QJsonDocument &document,DanmuAccessResult *result);
    void handleViewReply(QJsonDocument &document, DanmuAccessResult *result, DanmuSourceItem *sItem);
    void decodeVideoList(const QByteArray &bytes, DanmuAccessResult *result, int aid);
    void decodeEpList(const QByteArray &bytes, DanmuAccessResult *result, int aid);
    void handleDownloadReply(QString &reply,QList<DanmuComment *> &danmuList);
};

#endif // BILIBILIPROVIDER_H

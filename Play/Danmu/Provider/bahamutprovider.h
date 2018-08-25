#ifndef BAHAMUTPROVIDER_H
#define BAHAMUTPROVIDER_H
#include "providerbase.h"

class BahamutProvider : public ProviderBase
{
    Q_OBJECT
public:
    explicit BahamutProvider(QObject *parent = nullptr):ProviderBase(parent){}
    // ProviderBase interface
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "Gamer";}
    virtual QStringList supportedURLs();
    virtual QString sourceURL(DanmuSourceItem *item){return QString("gamer:%1").arg(item->id);}
    virtual bool supportSourceURL(const QString &url){return url.startsWith("gamer:")?true:false;}

public slots:
    virtual DanmuAccessResult *search(const QString &keyword);
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item);
    virtual DanmuAccessResult *getURLInfo(const QString &url);
    virtual QString downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList);
    virtual QString downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList);

private:
    void handleSearchReply(const QString &reply,DanmuAccessResult *result);
    void handleEpReply(QString &reply,DanmuAccessResult *result);
    void handleDownloadReply(QJsonDocument &document, QList<DanmuComment *> &danmuList);

};

#endif // BAHAMUTPROVIDER_H

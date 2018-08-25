#ifndef DANDANPROVIDER_H
#define DANDANPROVIDER_H
#include "providerbase.h"

class DandanProvider : public ProviderBase
{
    Q_OBJECT
public:
    explicit DandanProvider(QObject *parent=nullptr):ProviderBase(parent){}
    // ProviderBase interface
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "Dandan";}
    inline virtual QStringList supportedURLs(){return QStringList();}
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("dandan:%1").arg(item->id);}
    inline virtual bool supportSourceURL(const QString &url){ return url.startsWith("dandan:")?true:false;}

public slots:
    virtual DanmuAccessResult *search(const QString &keyword);
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item);
    inline virtual DanmuAccessResult *getURLInfo(const QString &){return nullptr;}
    virtual QString downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList);
    virtual QString downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList);
private:
    void handleSearchReply(QJsonDocument &document, DanmuAccessResult *result);
    void handleDownloadReply(QJsonDocument &document, QList<DanmuComment *> &danmuList);
};

#endif // DANDANPROVIDER_H

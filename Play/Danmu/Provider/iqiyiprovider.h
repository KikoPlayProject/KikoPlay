#ifndef IQIYIPROVIDER_H
#define IQIYIPROVIDER_H
#include "providerbase.h"

class IqiyiProvider : public ProviderBase
{
    Q_OBJECT
public:
    explicit IqiyiProvider(QObject *parent=nullptr):ProviderBase(parent){}
public:
    inline virtual bool supportSearch(){return true;}
    inline virtual QString id(){return "Iqiyi";}
    virtual QStringList supportedURLs();
    inline virtual QString sourceURL(DanmuSourceItem *item){return QString("iqiyi:%1;length:%2").arg(item->strId).arg(item->subId);}
    inline virtual bool supportSourceURL(const QString &url){return url.startsWith("iqiyi:")?true:false;}

public slots:
    virtual DanmuAccessResult *search(const QString &keyword);
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item);
    virtual DanmuAccessResult *getURLInfo(const QString &url);
    virtual QString downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList);
    virtual QString downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList);
private:
    void handleSearchReply(QString &reply,DanmuAccessResult *result);
    void downloadAllDanmu(const QString &id, int length, QList<DanmuComment *> &danmuList);
    void decodeDanmu(const QByteArray &replyBytes,QList<DanmuComment *> &danmuList);
    int decompress(const QByteArray &input, QByteArray &output);
};

#endif // IQIYIPROVIDER_H

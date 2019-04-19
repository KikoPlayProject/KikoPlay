#ifndef PROVIDERBASE_H
#define PROVIDERBASE_H

#include <QObject>
#include "info.h"
#include "../common.h"
class ProviderBase : public QObject
{
    Q_OBJECT
public:
    ProviderBase(QObject *parent=nullptr):QObject(parent){}
    virtual bool supportSearch()=0;
    virtual QString id()=0;
    virtual QStringList supportedURLs()=0;
    virtual QString sourceURL(DanmuSourceItem *item)=0;
    virtual bool supportSourceURL(const QString &url)=0;
public slots:
    virtual DanmuAccessResult *search(const QString &keyword)=0;
    virtual DanmuAccessResult *getEpInfo(DanmuSourceItem *item)=0;
    virtual DanmuAccessResult *getURLInfo(const QString &url)=0;
    virtual QString downloadDanmu(DanmuSourceItem *item,QList<DanmuComment *> &danmuList)=0;
    virtual QString downloadBySourceURL(const QString &url,QList<DanmuComment *> &danmuList)=0;
signals:
    void searchDone(DanmuAccessResult *searchInfo);
    void epInfoDone(DanmuAccessResult *epInfo, DanmuSourceItem *srcItem);
    void downloadDone(QString errInfo, DanmuSourceItem *srcItem);
protected:
    inline static QString formatTime(int duration)
    {
        int min=duration/60;
        int sec=duration-min*60;
        return QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
    }
};
#endif // PROVIDERBASE_H

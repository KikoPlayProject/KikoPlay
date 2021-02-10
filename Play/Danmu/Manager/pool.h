#ifndef POOL_H
#define POOL_H

#include <QObject>
#include "../common.h"
#include "MediaLibrary/animeinfo.h"

class Pool : public QObject
{
    Q_OBJECT
public:
    Pool(const QString &id, const QString &animeTitle, const QString &epTitle, EpType type, double index, QObject *parent = nullptr);
    inline const QList<QSharedPointer<DanmuComment> > &comments(){return commentList;}
    inline const QMap<int,DanmuSource> &sources(){return sourcesTable;}
    inline const QString &id() const {return pid;}
    inline bool isUsed() const {return used;}
    inline const QString &animeTitle() const {return anime;}
    inline const QString &epTitle() const {return ep;}
    EpInfo toEp() const { EpInfo ep; ep.name = this->ep; ep.type = epType; ep.index = epIndex; return ep; }
public:
    int update(int sourceId=-1, QList<QSharedPointer<DanmuComment> > *incList=nullptr);
    int addSource(const DanmuSource &sourceInfo, QList<DanmuComment *> &danmuList, bool reset=false);
    bool deleteSource(int sourceId, bool applyDB=true);
    bool deleteDanmu(int pos);
    bool setTimeline(int sourceId, const QList<QPair<int, int>> &timelineInfo);
    bool setDelay(int sourceId, int delay);
    void setUsed(bool on);
    void setSourceVisibility(int srcId, bool show);
    void exportPool(const QString &fileName, bool useTimeline=true, bool applyBlockRule=false, const QList<int> &ids=QList<int>());
    void exportKdFile(QDataStream &stream, const QList<int> &ids=QList<int>());
    void exportSimpleInfo(int srcId, QList<SimpleDanmuInfo> &simpleDanmuList);
    QJsonArray exportJson();
    QJsonObject exportFullJson();
    static QJsonArray exportJson(const QList<QSharedPointer<DanmuComment> > &danmuList, bool useOrigin=false);
    QString getPoolCode(const QStringList &addition=QStringList()) const;
    bool addPoolCode(const QString &code, bool hasAddition=false);
    bool addPoolCode(const QJsonArray &infoArray);
    static QJsonArray getPoolCodeInfo(const QString &code);
private:
    QString pid;
    QString anime,ep;
    EpType epType;
    double epIndex;

    bool used;
    bool isLoaded;
    QList<QSharedPointer<DanmuComment> > commentList;
    QMap<int,DanmuSource> sourcesTable;

    bool load();
    bool clean();
    void setDelay(DanmuComment *danmu);
    QSet<QString> getDanmuHashSet(int sourceId=-1);
    void addSourceJson(const QJsonArray &array);

    friend class DanmuManager;
signals:
    void poolChanged(bool reset);
public slots:
};

#endif // POOL_H

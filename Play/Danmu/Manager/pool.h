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
    inline const QVector<QSharedPointer<DanmuComment> > &comments(){return commentList;}
    inline const QMap<int,DanmuSource> &sources(){return sourcesTable;}
    inline const QString &id() const {return pid;}
    inline bool isUsed() const {return used;}
    inline const QString &animeTitle() const {return anime;}
    inline const QString &epTitle() const {return ep;}
    EpInfo toEp() const { EpInfo ep; ep.name = this->ep; ep.type = epType; ep.index = epIndex; return ep; }
    QVariantMap toMap() const;
    bool needRefresh() const { return _refreshFlag; }
public:
    int update(int sourceId=-1, QVector<QSharedPointer<DanmuComment> > *incList=nullptr);
    int addSource(const DanmuSource &sourceInfo, QVector<DanmuComment *> &danmuList, bool reset=false, bool save = true);
    bool deleteSource(int sourceId, bool applyDB=true);
    bool deleteDanmu(int pos);
    bool setTimeline(int sourceId, const QVector<QPair<int, int> > &timelineInfo);
    bool setDelay(int sourceId, int delay);
    void setUsed(bool on);
    void setSourceVisibility(int srcId, bool show);
    void exportPool(const QString &fileName, bool useTimeline=true, bool applyBlockRule=false, const QList<int> &ids=QList<int>());
    void exportKdFile(QDataStream &stream, const QList<int> &ids=QList<int>());
    void exportSimpleInfo(int srcId, QVector<SimpleDanmuInfo> &simpleDanmuList);
    QJsonArray exportJson();
    QJsonObject exportFullJson();
    template<typename T>
    static QJsonArray exportJson(const QVector<T> &danmuList, bool useOrigin=false)
    {
        QJsonArray danmuArray;
        for(const auto &danmu:danmuList)
        {
            if(danmu->blockBy!=-1) continue;
            if(useOrigin)
            {
                danmuArray.append(QJsonArray({danmu->originTime/1000.0,danmu->type,danmu->color,danmu->source,danmu->text, danmu->sender,danmu->date}));
            }
            else
            {
                danmuArray.append(QJsonArray({danmu->time/1000.0,danmu->type,danmu->color,danmu->sender,danmu->text}));
            }
        }
        return danmuArray;
    }
    QString getPoolCode(const QStringList &addition=QStringList()) const;
    bool addPoolCode(const QString &code, bool hasAddition=false);
    bool addPoolCode(const QJsonArray &infoArray);
    static QJsonArray getPoolCodeInfo(const QString &code);
    void setRefreshFlag(bool flag) { _refreshFlag = flag; }
private:
    QString pid;
    QString anime,ep;
    EpType epType;
    double epIndex;

    bool used;
    bool isLoaded;
    std::atomic_bool _refreshFlag;
    QVector<QSharedPointer<DanmuComment> > commentList;
    QMap<int,DanmuSource> sourcesTable;

    bool load();
    bool clean();
    void setDelay(DanmuComment *danmu);
    QSet<QString> getDanmuHashSet(int sourceId=-1);
    void addSourceJson(const QJsonArray &array);

    friend class DanmuManager;
signals:
    void poolChanged(bool reset);
};

#endif // POOL_H

#ifndef DANMUPOOL_H
#define DANMUPOOL_H

#include <QAbstractItemModel>
#include "common.h"
struct StatisInfo
{
    QList<QPair<int,int> > countOfMinute;
    int maxCountOfMinute;
    int totalCount;
    int blockCount;
    int mergeCount;
};
class DanmuPool : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DanmuPool(QObject *parent = nullptr);
    ~DanmuPool(){qDeleteAll(prepareListPool);}

    inline QString getPoolID() const { return poolID; }
    inline QModelIndex getCurrentIndex(){return (currentPosition >= 0 && currentPosition < finalPool.count())?createIndex(currentPosition, 0,finalPool.at(currentPosition).data()):QModelIndex();}
    inline QHash<int,DanmuSourceInfo> &getSources(){return sourcesTable;}
    inline void recyclePrepareList(PrepareList *list){list->clear();prepareListPool.append(list);}
    inline bool isEmpty() const{return danmuPool.isEmpty();}
    inline int totalCount() const {return danmuPool.count();}
    inline const StatisInfo &getStatisInfo(){return statisInfo;}
    inline void reset(){currentTime=0;currentPosition=0;}

    QSharedPointer<DanmuComment> &getDanmu(const QModelIndex &index);
    int addDanmu(DanmuSourceInfo &sourceInfo, QList<DanmuComment *> &danmuList, bool resetModel=true);
    void resetModel();
    void deleteDanmu(QSharedPointer<DanmuComment> &danmu);
    void deleteSource(int sourceIndex);
    QList<SimpleDanmuInfo> getSimpleDanmuInfo(int sourceId);
private:
    QList<QSharedPointer<DanmuComment> > danmuPool;
    QList<QSharedPointer<DanmuComment> > finalPool;
    QHash<int,DanmuSourceInfo> sourcesTable;
    QLinkedList<PrepareList *> prepareListPool;
    StatisInfo statisInfo;
    int currentPosition;
    int currentTime;
    QString poolID;

    bool enableMerged;
    int mergeInterval; //ms
    int maxContentUnsimCount;
    int minMergeCount;
    void setMerged();
    bool contentSimilar(const DanmuComment *dm1, const DanmuComment *dm2);

    void loadDanmuFromDB();
    void setStatisInfo();
    QSet<QString> getDanmuHash(int sourceId);
public:
    void setDelay(DanmuSourceInfo *sourceInfo,int newDelay);
    void setMergeEnable(bool enable);
    void setMergeInterval(int val);
    void setMaxUnSimCount(int val);
    void setMinMergeCount(int val);
    void refreshTimeLineDelayInfo(DanmuSourceInfo *sourceInfo);
    void setPoolID(const QString &pid);
	void refreshCurrentPoolID();
    void testBlockRule(BlockRule *rule);
    void exportDanmu(int sourceId,const QString &fileName);
    void cleanUp();

signals:
    void statisInfoChange();
public slots:
    void mediaTimeElapsed(int newTime);
    void mediaTimeJumped(int newTime);

    // QAbstractItemModel interface
public:
    inline virtual int columnCount(const QModelIndex &) const {return 2;}
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // DANMUPOOL_H

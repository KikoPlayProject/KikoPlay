#ifndef DANMUPOOL_H
#define DANMUPOOL_H

#include <QAbstractItemModel>
#include "common.h"
struct StatisInfo
{
    QList<QPair<int,int> > countOfSecond;
    int maxCountOfMinute;
    int totalCount;
    int blockCount;
    int mergeCount;
};
class Pool;
class EventAnalyzer;
class DanmuPool : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DanmuPool(QObject *parent = nullptr);
    virtual ~DanmuPool();

    inline bool hasPool() const {return curPool!=emptyPool;}
    //inline QString getPoolID() const { return poolID; }
    inline QModelIndex getCurrentIndex(){return (currentPosition >= 0 && currentPosition < finalPool.count())?createIndex(currentPosition, 0,finalPool.at(currentPosition).data()):QModelIndex();}
    inline void recyclePrepareList(QList<DrawTask> *list){list->clear();prepareListPool.append(list);}
    inline bool isEmpty() const{return danmuPool.isEmpty();}
    inline int totalCount() const {return danmuPool.count();}
    inline const StatisInfo &getStatisInfo(){return statisInfo;}
    inline void reset(){currentTime=0;currentPosition=0;}
    inline Pool *getPool() {return curPool;}

    QSharedPointer<DanmuComment> getDanmu(const QModelIndex &index);
    void deleteDanmu(QSharedPointer<DanmuComment> danmu);

private:
    Pool *curPool,*emptyPool;
    QList<QSharedPointer<DanmuComment> > danmuPool;
    QList<QSharedPointer<DanmuComment> > finalPool;
    QList<QList<DrawTask> *> prepareListPool;
    StatisInfo statisInfo;
    EventAnalyzer *analyzer;
    int currentPosition;
    int currentTime;
   // QString poolID;

    bool enableAnalyze;
    bool enableMerged;
    int mergeInterval; //ms
    int maxContentUnsimCount;
    int minMergeCount;
    void setMerged();
    bool contentSimilar(const DanmuComment *dm1, const DanmuComment *dm2);
    void setAnalyzation();
    void setConnect(Pool *pool);

    void setStatisInfo();
public:
    void setAnalyzeEnable(bool enable);
    void setMergeEnable(bool enable);
    void setMergeInterval(int val);
    void setMaxUnSimCount(int val);
    void setMinMergeCount(int val);
    void setPoolID(const QString &pid);
    void testBlockRule(BlockRule *rule);
    void cleanUp();

signals:
    void statisInfoChange();
    void eventAnalyzeFinished(const QList<DanmuEvent> &);
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

#ifndef DANMUPOOL_H
#define DANMUPOOL_H

#include <QAbstractItemModel>
#include "common.h"
struct StatisInfo
{
    QVector<QPair<int,int> > countOfSecond;
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

    bool hasPool() const {return curPool!=emptyPool;}
    QModelIndex getCurrentIndex(){return (currentPosition >= 0 && currentPosition < finalPool.count())?createIndex(currentPosition, 0,finalPool.at(currentPosition).data()):QModelIndex();}
    void recyclePrepareList(QVector<DrawTask> *list){list->clear();prepareListPool.append(list);}
    bool isEmpty() const{return danmuPool.isEmpty();}
    int totalCount() const {return danmuPool.count();}
    const StatisInfo &getStatisInfo(){return statisInfo;}
    void reset(){currentTime=0;currentPosition=0;extendPos=0;}
    Pool *getPool() {return curPool;}
    int getCurrentTime() const {return currentTime;}
    bool isEnableAnalyze() const { return enableAnalyze; }
    bool isEnableMerge() const { return enableMerged; }
    int getMinMergeCount() const { return minMergeCount; }
    int getMaxUnsimCount() const { return maxContentUnsimCount; }
    int getMergeInterval() const { return mergeInterval / 1000; }
    bool isLoadLocalDanmu() const { return loadLoaclDanmu; }

    QSharedPointer<DanmuComment> getDanmu(const QModelIndex &index);
    void deleteDanmu(QSharedPointer<DanmuComment> danmu);
    void launch(const QList<QSharedPointer<DanmuComment>> userComments);
    bool addLocalDanmuFile(const QString &fileName);

private:
    Pool *curPool,*emptyPool;
    QVector<QSharedPointer<DanmuComment> > danmuPool;
    QVector<QSharedPointer<DanmuComment> > finalPool;
    QVector<QVector<DrawTask> *> prepareListPool;
    QVector<int> tmpSourceIds;
    StatisInfo statisInfo;
    EventAnalyzer *analyzer;
    int currentPosition;
    int currentTime;
    int extendPos;
    const int bundleSize=32;

    bool enableAnalyze;
    bool enableMerged;
    bool loadLoaclDanmu;
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
    void setLoadLocalDanmu(bool loadLocal);
    void setMergeInterval(int val);
    void setMaxUnSimCount(int val);
    void setMinMergeCount(int val);
    void setPoolID(const QString &pid);
    void testBlockRule(BlockRule *rule);

signals:
    void statisInfoChange();
    void eventAnalyzeFinished(const QVector<DanmuEvent> &);
    void poolIdChanged();
    void triggerUpdate();
    void triggerAdd();
    void triggerEditPool();
    void triggerEditBlockRules();

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

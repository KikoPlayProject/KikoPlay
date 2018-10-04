#ifndef DANMUPOOL_H
#define DANMUPOOL_H

#include <QAbstractItemModel>
#include "common.h"
struct StatisInfo
{
    QList<QPair<int,int> > countOfMinute;
    int maxCountOfMinute;
};
struct SimpleDanmuInfo
{
    int time;
    int originTime;
    QString text;
};
class DanmuPool : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DanmuPool(QObject *parent = nullptr);
    ~DanmuPool(){qDeleteAll(prepareListPool);}

    inline QString getPoolID() const { return poolID; }
    inline QSharedPointer<DanmuComment> &getDanmu(int row){return danmuPool[row];}
    inline QModelIndex getCurrentIndex(){return (currentPosition >= 0 && currentPosition < danmuPool.count())?createIndex(currentPosition, 0):QModelIndex();}
    inline QHash<int,DanmuSourceInfo> &getSources(){return sourcesTable;}
    inline void recyclePrepareList(PrepareList *list){list->clear();prepareListPool.append(list);}
    inline bool isEmpty() const{return danmuPool.isEmpty();}
    inline int totalCount() const {return danmuPool.count();}
    inline const StatisInfo &getStatisInfo(){return statisInfo;}
    inline void reset(){currentTime=0;currentPosition=0;}

    void addDanmu(DanmuSourceInfo &sourceInfo,QList<DanmuComment *> &danmuList);
    void deleteDanmu(QSharedPointer<DanmuComment> &danmu);
    void deleteSource(int sourceIndex);
    void loadDanmuFromDB();	
    QSet<QString> getDanmuHash(int sourceId);
    QList<SimpleDanmuInfo> getSimpleDanmuInfo(int sourceId);
private:
    QList<QSharedPointer<DanmuComment> > danmuPool;
    QHash<int,DanmuSourceInfo> sourcesTable;
    QList<PrepareList *> prepareListPool;
    StatisInfo statisInfo;
    int currentPosition;
    int currentTime;
    QString poolID;
    void saveDanmu(const DanmuSourceInfo *sourceInfo,const QList<DanmuComment *> *danmuList);
    void setStatisInfo();
public:
    void setDelay(DanmuSourceInfo *sourceInfo,int newDelay);
    void refreshTimeLineDelayInfo(DanmuSourceInfo *sourceInfo);
    inline void setPoolID(const QString &pid){poolID=pid;}
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
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const {return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const{return parent.isValid()?0:danmuPool.count();}
    inline virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:2;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // DANMUPOOL_H

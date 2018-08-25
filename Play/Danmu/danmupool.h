#ifndef DANMUPOOL_H
#define DANMUPOOL_H

#include <QAbstractItemModel>
#include "common.h"
class DanmuRender;
struct StatisInfo
{
    QList<QPair<int,int> > countOfMinute;
    int maxCountOfMinute;
};
class DanmuPool : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DanmuPool(QObject *parent = nullptr);
    ~DanmuPool(){qDeleteAll(prepareListPool);}

    void addDanmu(DanmuSourceInfo &sourceInfo,QList<DanmuComment *> &danmuList);
    void deleteSource(int sourceIndex);
    void loadDanmuFromDB();
    QHash<int,DanmuSourceInfo> &getSources(){return sourcesTable;}
    void reset();
	void cleanUp();
	QString getPoolID() const { return poolID; }
	QModelIndex getCurrentIndex();
    void testBlockRule(BlockRule *rule);
    inline QSharedPointer<DanmuComment> &getDanmu(int row){return danmuPool[row];}
    void deleteDanmu(QSharedPointer<DanmuComment> &danmu);
    inline void recyclePrepareList(PrepareList *list){list->clear();prepareListPool.append(list);}
    QSet<QString> getDanmuHash(int sourceId);
    void exportDanmu(int sourceId,QString &fileName);
    bool isEmpty() const{return danmuPool.isEmpty();}
    const StatisInfo &getStatisInfo(){return statisInfo;}
    int totalCount() const {return danmuPool.count();}
private:
    QList<QSharedPointer<DanmuComment> > danmuPool;
    QHash<int,DanmuSourceInfo> sourcesTable;
    QList<PrepareList *> prepareListPool;
    StatisInfo statisInfo;
    int currentPosition;
    int currentTime;
    DanmuRender *dmRender;
    QString poolID;
    void saveDanmu(const DanmuSourceInfo *sourceInfo,const QList<DanmuComment *> *danmuList);
    void setStatisInfo();
public:
    void setDelay(DanmuSourceInfo *sourceInfo,int newDelay);
    void setPoolID(QString pid);
	void refreshCurrentPoolID();
signals:
    void statisInfoChange();
public slots:
    void mediaTimeElapsed(int newTime);
    void mediaTimeJumped(int newTime);

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const{return parent.isValid()?0:danmuPool.count();}
    inline virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:2;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // DANMUPOOL_H

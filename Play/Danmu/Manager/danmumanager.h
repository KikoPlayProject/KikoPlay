#ifndef DANMUMANAGER_H
#define DANMUMANAGER_H

#include <QAbstractItemModel>
#include "../common.h"
#include "nodeinfo.h"
class Pool;
class DanmuManager : public QObject
{
    Q_OBJECT
    friend class Pool;
    friend class PoolStateLock;
public:
    explicit DanmuManager(QObject *parent = nullptr);
    virtual ~DanmuManager();
public:
    Pool *getPool(const QString &pid, bool loadDanmu=true);
    Pool *getPool(const QString &animeTitle, const QString &title, bool loadDanmu=true);
    void loadPoolInfo(QList<DanmuPoolNode *> &poolNodeList);
    void deletePool(const QList<DanmuPoolNode *> &deleteList);
    void updatePool(QList<DanmuPoolNode *> &updateList);
    void exportPool(const QList<DanmuPoolNode *> &exportList, const QString &dir, bool useTimeline=true, bool applyBlockRule=false);
    void exportKdFile(const QList<DanmuPoolNode *> &exportList, const QString &dir, const QString &comment="");
    int importKdFile(const QString &fileName, QWidget *parent);
    QStringList getAssociatedFile16Md5(const QString &pid);
    QString createPool(const QString &animeTitle, const QString &title, const QString &fileHash="");
    QString renamePool(const QString &pid, const QString &nAnimeTitle, const QString &nEpTitle);
    QString getFileHash(const QString &fileName);
public:
    enum MatchProvider
    {
        DanDan,Bangumi,Local
    };
    MatchInfo *searchMatch(MatchProvider from, const QString &keyword);
    MatchInfo *matchFrom(MatchProvider from, const QString &fileName);
    QString updateMatch(const QString &fileName,const MatchInfo *newMatchInfo);
    void removeMatch(const QString &fileName);
private:
    MatchInfo *ddSearch(const QString &keyword);
    MatchInfo *bgmSearch(const QString &keyword);
    MatchInfo *localSearch(const QString &keyword);
    MatchInfo *ddMatch(const QString &fileName);
    MatchInfo *localMatch(const QString &fileName);
    MatchInfo *searchInMatchTable(const QString &fileHash);
    void setMatch(const QString &fileHash, const QString &poolId);

signals:
    void workerStateMessage(const QString &msg);
private:
    void loadPool(Pool *pool);
    void updatePool(Pool *pool, QList<DanmuComment *> &outList, int sourceId=-1);
    QString getPoolId(const QString &animeTitle, const QString &title);
    void saveSource(const QString &pid, const DanmuSourceInfo *source, const QList<QSharedPointer<DanmuComment> > &danmuList);
    void deleteSource(const QString &pid, int sourceId);
    void deleteDanmu(const QString &pid, const QSharedPointer<DanmuComment> danmu);
    void updateSourceTimeline(const QString &pid, const DanmuSourceInfo *sourceInfo);
    void updateSourceDelay(const QString &pid, const DanmuSourceInfo *sourceInfo);
    QList<DanmuComment *> updateSource(const DanmuSourceInfo *sourceInfo, const QSet<QString> &danmuHashSet);

private:
    void loadAllPool();
    void refreshCache(Pool *newPool=nullptr);
    void deletePool(const QString &pid);

private:
    QMap<QString,int> poolDanmuCacheInfo;
    QMap<QString,Pool *> pools;
    QMutex *cacheLock,*removeLock;
    QReadWriteLock poolStateLock;
    QSet<QString> busyPoolSet;
    bool countInited;
    const int DanmuTableCount=5;
};
class PoolStateLock
{
    bool locked=false;
    QString pid;
public:
    static DanmuManager *manager;
    bool tryLock(const QString &pid)
    {
        manager->poolStateLock.lockForRead();
        if(manager->busyPoolSet.contains(pid))
        {
            locked=false;
            manager->poolStateLock.unlock();
            return false;
        }
        else
        {
            manager->poolStateLock.unlock();
            manager->poolStateLock.lockForWrite();
            manager->busyPoolSet.insert(pid);
            locked=true;
            this->pid=pid;
            manager->poolStateLock.unlock();
            return true;
        }
    }
    ~PoolStateLock()
    {
        if(locked)
        {
            manager->poolStateLock.lockForWrite();
            manager->busyPoolSet.remove(pid);
            manager->poolStateLock.unlock();
        }
    }
};

#endif // DANMUMANAGER_H

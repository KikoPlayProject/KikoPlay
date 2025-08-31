#ifndef DANMUMANAGER_H
#define DANMUMANAGER_H

#include <QAbstractItemModel>
#include "../common.h"
#include "Common/lrucache.h"
#include "nodeinfo.h"
#include "MediaLibrary/animeinfo.h"
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
    Pool *getPool(const QString &animeTitle, EpType epType, double epIndex, bool loadDanmu=true);
    void loadPoolInfo(QList<DanmuPoolNode *> &poolNodeList);
    void deletePool(const QList<DanmuPoolNode *> &deleteList);
    void updatePool(QList<DanmuPoolNode *> &updateList);
    void setPoolDelay(QList<DanmuPoolNode *> &updateList, int delay);
    void exportPool(const QList<DanmuPoolNode *> &exportList, const QString &dir, bool useTimeline=true, bool applyBlockRule=false);
    void exportKdFile(const QList<DanmuPoolNode *> &exportList, const QString &dir, const QString &comment="");
    int importKdFile(const QString &fileName, QWidget *parent);
    QStringList getMatchedFile16Md5(const QString &pid);
    QString createPool(const QString &animeTitle, EpType epType, double epIndex, const QString &epName="", const QString &fileHash="");
    QString createPool(const QString &path, const MatchResult &match);
    QString renamePool(const QString &pid, const QString &nAnimeTitle, EpType nType, double nIndex, const QString &nEpTitle);
    QString getFileHash(const QString &fileName);
    void sendDanmuAddedEvent(const QList<QPair<Pool *, QPair<DanmuSource, QString> > > &poolSrcs);
public:
    void localSearch(const QString &keyword,  QList<AnimeLite> &results);
    void localMatch(const QString &path, MatchResult &result);
    QString updateMatch(const QString &fileName, const MatchResult &newMatchInfo);
    void removeMatch(const QString &fileName);
private:
    void setMatch(const QString &fileHash, const QString &poolId);

signals:
    void workerStateMessage(const QString &msg);
private:
    void loadPool(Pool *pool);
    void updatePool(Pool *pool, QVector<DanmuComment *> &outList, int sourceId=-1);
    QString getPoolId(const QString &animeTitle, EpType epType, double epIndex);
    void saveSource(const QString &pid, const DanmuSource *source, const QVector<QSharedPointer<DanmuComment> > &danmuList);
    void deleteSource(const QString &pid, int sourceId);
    void deleteDanmu(const QString &pid, const QSharedPointer<DanmuComment> danmu);
    void updateSourceTimeline(const QString &pid, const DanmuSource *sourceInfo);
    void updateSourceDelay(const QString &pid, const DanmuSource *sourceInfo);
    QVector<DanmuComment *> updateSource(const DanmuSource *sourceInfo, const QSet<QString> &danmuHashSet);

private:
    void loadAllPool();
    void refreshCache(Pool *newPool=nullptr);
    void deletePool(const QString &pid);

private:
    QMap<QString,int> poolDanmuCacheInfo;
    QSharedPointer<LRUCache<QString, Pool *>> poolCache;
    QMap<QString,Pool *> pools;
    QRecursiveMutex poolsLock;
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

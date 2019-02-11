#ifndef DANMUMANAGER_H
#define DANMUMANAGER_H

#include <QAbstractItemModel>
#include "poolworker.h"
#include "../common.h"
class DanmuManager : public QObject
{
    Q_OBJECT
public:
    explicit DanmuManager(QObject *parent = nullptr);
    virtual ~DanmuManager();
public:
    void loadPoolInfo(QList<DanmuPoolNode *> &poolNodeList);
    void loadPool(const QString &pid, QList<QSharedPointer<DanmuComment> > &danmuList,QHash<int,DanmuSourceInfo> &sourcesTable);
    void loadSimpleDanmuInfo(const QString &pid, int srcId, QList<SimpleDanmuInfo> &simpleDanmuList);
    void exportPool(const QList<DanmuPoolNode *> &exportList, const QString &dir, bool useTimeline=true, bool applyBlockRule=false);
    void exportPool(const QString &pid,const QString &fileName);
    QJsonObject exportJson(const QString &pid);
    void deletePool(const QList<DanmuPoolNode *> &deleteList);
    void deleteSource(const QString &pid, int srcId);
    void deleteDanmu(const QString &pid, const DanmuComment *danmu);
    void updatePool(QList<DanmuPoolNode *> &updateList);
    void saveSource(const QString &pid, const DanmuSourceInfo *sourceInfo, const QList<DanmuComment *> *danmuList);
    void updateSourceDelay(const QString &pid, const DanmuSourceInfo *sourceInfo);
    void updateSourceDelay(const DanmuPoolSourceNode *srcNode);
    void updateSourceTimeline(const QString &pid, const DanmuSourceInfo *sourceInfo);
    void updateSourceTimeline(const DanmuPoolSourceNode *srcNode);
    void setDelay(DanmuComment *danmu, DanmuSourceInfo *srcInfo);
    DanmuPoolSourceNode *addSource(DanmuPoolNode *epNode, const DanmuSourceInfo *sourceInfo, const QList<DanmuComment *> *danmuList);
signals:
    void workerStateMessage(const QString &msg);
private:
    PoolWorker *poolWorker;
};
#endif // DANMUMANAGER_H

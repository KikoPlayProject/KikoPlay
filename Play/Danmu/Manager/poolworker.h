#ifndef POOLWORKER_H
#define POOLWORKER_H

#include <QtCore>
#include "nodeinfo.h"
#include "../common.h"
class PoolWorker : public QObject
{
    Q_OBJECT
public:
    explicit PoolWorker(QObject *parent=nullptr):QObject(parent){}
public slots:
    void loadPoolInfo(QList<DanmuPoolNode *> &poolNodeList);
    void exportPool(const QList<DanmuPoolNode *> &exportList, const QString &dir, bool useTimeline=true, bool applyBlockRule=false);
    void exportPool(const QString &pid, const QString &fileName);
    void deletePool(const QList<DanmuPoolNode *> &deleteList);
    void updatePool(QList<DanmuPoolNode *> &updateList);
    void saveSource(const QString &pid, const DanmuSourceInfo *sourceInfo, const QList<DanmuComment *> *danmuList);
    void exportJson(const QString &pid);
    void addSource(DanmuPoolNode *epNode, const DanmuSourceInfo *sourceInfo, const QList<DanmuComment *> *danmuList);
signals:
    void loadDone();
    void addDone(DanmuPoolSourceNode *newSrcNode);
    void exportDone();
    void deleteDone();
    void updateDone();
    void exportJsonDown(QJsonObject jsonObj);
    void exportSingleDown();
    void stateMessage(const QString &msg);
    void addToCurPool(const QString &idInfo, QList<DanmuComment *> danmuList);
private:
    const int DanmuTableCount=5;
    void exportEp(const DanmuPoolNode *epNode, const QString &fileName, bool useTimeline=true, bool applyBlockRule=false);
    void updateSource(DanmuPoolSourceNode *sourceNode, const QSet<QString> &danmuHashSet);
    QSet<QString> genDanmuHash(const QString &pid);
};
#endif // POOLWORKER_H

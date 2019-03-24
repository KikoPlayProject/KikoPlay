#ifndef MANAGERVIEW_H
#define MANAGERVIEW_H
#include <QAbstractItemModel>
#include "nodeinfo.h"
class DanmuManagerModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DanmuManagerModel(QObject *parent = nullptr);
    virtual ~DanmuManagerModel();
    void refreshList();
    void exportPool(const QString &dir, bool useTimeline=true, bool applyBlockRule=false);
    void deletePool();
    void updatePool();
    int totalDanmuCount();
    int totalPoolCount();
    bool hasSelected();
    DanmuPoolSourceNode *getSourceNode(const QModelIndex &index);
    DanmuPoolNode *getPoolNode(const QModelIndex &index);
    void addSrcNode(DanmuPoolNode *epNode, DanmuPoolSourceNode *srcNode);
private:
    QList<DanmuPoolNode *> animeNodeList;
    const QStringList headers={tr("Title"),tr("Source"),tr("Delay"),tr("Danmu Count")};

    void refreshChildrenCheckStatus(const QModelIndex &index);
    void refreshParentCheckStatus(const QModelIndex &index);

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    inline virtual int columnCount(const QModelIndex &) const {return headers.count();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
};
class PoolSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PoolSortProxyModel(QObject *parent = nullptr):QSortFilterProxyModel(parent){comparer.setNumericMode(true);}
private:
    QCollator comparer;
    // QSortFilterProxyModel interface
protected:
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
};

#endif // MANAGERVIEW_H

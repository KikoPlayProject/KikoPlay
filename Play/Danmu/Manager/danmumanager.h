#ifndef DANMUMANAGER_H
#define DANMUMANAGER_H

#include <QAbstractItemModel>
struct DanmuPoolInfo
{
    QString poolID;
    QString animeTitle;
    QString epTitle;
    int danmuCount;
};
class PoolInfoWorker : public QObject
{
    Q_OBJECT
public:
    explicit PoolInfoWorker(QObject *parent=nullptr):QObject(parent){}
public slots:
    void loadPoolInfo(QList<DanmuPoolInfo> &poolInfoList);
    void exportPool(const QList<DanmuPoolInfo> &exportList, const QString &dir);
    void deletePool(const QList<DanmuPoolInfo> &deleteList);
signals:
    void loadDone();
    void exportDone();
    void deleteDone();
};
class DanmuManager : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DanmuManager(QObject *parent = nullptr);
private:
    QList<DanmuPoolInfo> poolList;
    static PoolInfoWorker *poolWorker;
signals:

public slots:
    void refreshPoolInfo();
    void exportPool(QModelIndexList &exportIndexes, const QString &dir);
    void deletePool(QModelIndexList &deleteIndexes);
    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const {return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const{return parent.isValid()?0:poolList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:3;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // DANMUMANAGER_H

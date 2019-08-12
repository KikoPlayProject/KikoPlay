#ifndef DOWNLOADMODEL_H
#define DOWNLOADMODEL_H
#include <QAbstractItemModel>
#include <QIcon>
#include "util.h"
class Aria2JsonRPC;
class DownloadWorker : public QObject
{
    Q_OBJECT
public:
    explicit DownloadWorker(QObject *parent=nullptr):QObject(parent){}
private:

public slots:
    void loadTasks(QList<DownloadTask *> &items, int offset, int limit);
    void addTask(DownloadTask *task);
    void deleteTask(DownloadTask *task, bool deleteFile);
    void updateTaskInfo(const DownloadTask *task);
signals:
    void loadDone(int count);
};
class DownloadModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit DownloadModel(QObject *parent = nullptr);
    ~DownloadModel();
    enum DataRole
    {
        StatusRole=Qt::UserRole,
        TitleRole,
        TotalLengthRole,
        CompletedLengthRole,
        DownSpeedRole,
        UpSpeedRole
    };
    void setRPC(Aria2JsonRPC *aria2RPC);
    const QMap<QString, DownloadTask *> &getItems() const{return gidMap;}
    QList<DownloadTask *> &getAllTask() {return downloadTasks;}
private:
    QList<DownloadTask *> downloadTasks;
    QMap<QString ,DownloadTask *> gidMap;
    const QStringList headers={tr("Status"),tr("Title"),tr("Progress"),tr("Size"),tr("DownSpeed"),tr("Time Left")};
    const QStringList status={tr("Downloading"),tr("Seeding"),tr("Waiting"),tr("Paused"),tr("Complete"),tr("Error")};
    QIcon statusIcons[6]={QIcon(":/res/images/downloading.png"),
                                    QIcon(":/res/images/seeding.png"),
                                    QIcon(":/res/images/waiting.png"),
                                    QIcon(":/res/images/paused.png"),
                                    QIcon(":/res/images/completed.png"),
                                    QIcon(":/res/images/error.png")};
    const int limitCount=100;
    int currentOffset;
    bool hasMoreTasks;
    Aria2JsonRPC *rpc;
    static DownloadWorker *downloadWorker;
    void addTask(DownloadTask *task);
    void removeItem(DownloadTask *task, bool deleteFile);
    bool containTask(const QString &taskId);
    QString processKikoPlayCode(const QString &code);
signals:
    void magnetDone(const QString &path, const QString &magnet);
    void removeTask(const QString &gid);
    void taskFinish(DownloadTask *task);
public slots: 
    QString addUriTask(const QString &uri, const QString &dir, bool directlyDownload=false);
    QString addTorrentTask(const QByteArray &torrentContent,const QString &infoHash,const QString &dir,
                           const QString &selIndexes, const QString &magnet);
    void removeItem(QModelIndexList &removeIndexes, bool deleteFile);
    DownloadTask *getDownloadTask(const QModelIndex &index);
    void tryLoadTorrentContent(DownloadTask *task);
    QString restartDownloadTask(DownloadTask *task, bool allowOverwrite=false);
    void updateItemStatus(const QJsonObject &statusObj);
    void queryItemStatus();
    void saveItemStatus();
    void saveItemStatus(const DownloadTask *task);
    QString findFileUri(const QString &fileName);
    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:downloadTasks.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:6;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual void fetchMore(const QModelIndex &parent);
    inline virtual bool canFetchMore(const QModelIndex &) const {return hasMoreTasks;}
};
class TaskFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit TaskFilterProxyModel(QObject *parent = nullptr):QSortFilterProxyModel(parent){setFilterRole(DownloadModel::TitleRole);}
    void setTaskStatus(int status){taskStatus=status;}

private:
    int taskStatus;
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
};
#endif // DOWNLOADMODEL_H

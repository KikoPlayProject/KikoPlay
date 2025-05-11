#ifndef TASKPOOL_H
#define TASKPOOL_H

#include <QAbstractItemModel>
#include <QRunnable>
#include <QReadWriteLock>
#include <QThreadPool>

enum class TaskStatus
{
    Pending,
    Running,
    Finished,
    Failed,
    Cancelled
};

class KTask : public QObject
{
    Q_OBJECT
public:
    KTask(const QString taskName = "") : QObject(nullptr), _name(taskName),
        _status(TaskStatus::Pending), _cancelFlag(false), _progress(0)
    {}
    virtual ~KTask() {}

    QString name() const { return _name; }

    TaskStatus status() const
    {
        QReadLocker locker(&lock);
        return _status;
    }

    int progress() const
    {
        QReadLocker locker(&lock);
        return _progress;
    }

    QString info() const
    {
        QReadLocker locker(&lock);
        return _info;
    }

    virtual void cancel()
    {
        _cancelFlag = true;
        emit taskCancel();
    }

    virtual void run();

signals:
    void statusChanged(TaskStatus status);
    void progreeUpdated(int progress);
    void infoChanged(const QString &info, int flag);
    void taskCancel();

protected:
    virtual TaskStatus runTask() = 0;

    virtual void updateProgress(int value);

    virtual void setStatus(TaskStatus newStatus);

    virtual void setInfo(const QString &info, int flag = 0);

protected:
    mutable QReadWriteLock lock;
    QString _name;
    TaskStatus _status;
    std::atomic_bool _cancelFlag;
    int _progress;
    QString _info;
};

class KTaskWrapper : public QRunnable
{
public:
    KTaskWrapper(KTask *taskObj) : _taskObj(taskObj) {}
    void run() override;
private:
    KTask *_taskObj;
};

class TaskPool : public QAbstractItemModel
{
    Q_OBJECT
public:
    static TaskPool *instance();

    explicit TaskPool(QObject *parent = nullptr);
    ~TaskPool() override;

    void submitTask(KTask* task);

public:
    enum struct Columns
    {
        NAME,
        STATUS,
        PROGRESS,
        INFO,
    };

    inline virtual int columnCount(const QModelIndex &) const override { return 4; }
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override { return parent.isValid() ? QModelIndex() : createIndex(row,column); }
    virtual QModelIndex parent(const QModelIndex &child) const override { return QModelIndex(); }
    virtual int rowCount(const QModelIndex &parent) const override { return parent.isValid() ? 0 :_tasks.size(); }
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QList<KTask *> _tasks;
    QThreadPool _threadPool;
};
#endif // TASKPOOL_H

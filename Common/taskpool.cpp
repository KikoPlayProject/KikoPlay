#include "taskpool.h"
#include "logger.h"

TaskPool *TaskPool::instance()
{
    static TaskPool pool;
    return &pool;
}

TaskPool::TaskPool(QObject *parent) : QAbstractItemModel{parent}
{
    qRegisterMetaType<TaskStatus>("TaskStatus");
}

TaskPool::~TaskPool()
{

}

void TaskPool::submitTask(KTask *task)
{
    auto modelUpdate = [=](){
        beginInsertRows(QModelIndex(), _tasks.size(), _tasks.size());
        _tasks.append(task);
        endInsertRows();
    };
    if (QThread::currentThread() == thread())
    {
        modelUpdate();
    }
    else
    {
        QMetaObject::invokeMethod(this, [=](){
            modelUpdate();
        }, Qt::BlockingQueuedConnection);
    }

    QObject::connect(task, &KTask::statusChanged, this, [=](TaskStatus status){
        int index = _tasks.indexOf(task);
        if (index < 0) return;
        if (status == TaskStatus::Finished || status == TaskStatus::Cancelled || status == TaskStatus::Failed)
        {
            beginRemoveRows(QModelIndex(), index, index);
            _tasks.removeAt(index);
            task->deleteLater();
            endRemoveRows();
        }
        else
        {
            QModelIndex mIndex = this->index(index, (int)Columns::STATUS, QModelIndex());
            emit dataChanged(mIndex, mIndex);
        }
    }, Qt::QueuedConnection);
    QObject::connect(task, &KTask::progreeUpdated, this, [=](){
        int index = _tasks.indexOf(task);
        if (index >= 0)
        {
            QModelIndex mIndex = this->index(index, (int)Columns::PROGRESS, QModelIndex());
            emit dataChanged(mIndex, mIndex);
        }
    }, Qt::QueuedConnection);
    QObject::connect(task, &KTask::infoChanged, this, [=](){
        int index = _tasks.indexOf(task);
        if (index >= 0)
        {
            QModelIndex mIndex = this->index(index, (int)Columns::INFO, QModelIndex());
            emit dataChanged(mIndex, mIndex);
        }
    }, Qt::QueuedConnection);

    KTaskWrapper *wrapper = new KTaskWrapper(task);
    _threadPool.start(wrapper);
}

QVariant TaskPool::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    KTask *task = _tasks[index.row()];
    Columns col = static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
        switch (col)
        {
        case Columns::NAME:
        {
            return task->name();
        }
        case Columns::STATUS:
        {
            static const QStringList status{tr("Pending"), tr("Running"), tr("Finished"), tr("Failed"), tr("Cancelled")};
            return status[(int)task->status()];
        }
        case Columns::PROGRESS:
        {
            return task->progress();
        }
        case Columns::INFO:
            return task->info();
        }
        break;
    }
    default:
        return QVariant();
    }
    return QVariant();
}

QVariant TaskPool::headerData(int section, Qt::Orientation orientation, int role) const
{
    static const QStringList headers{ tr("Name"), tr("Status"), tr("Progress"), tr("Info"), };
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (section < headers.size()) return headers.at(section);
    }
    return QVariant();
}

void KTaskWrapper::run()
{
    QThread *srcThread = _taskObj->thread();
    _taskObj->moveToThread(QThread::currentThread());
    _taskObj->run();
    _taskObj->moveToThread(srcThread);
}

void KTask::run()
{
    setStatus(TaskStatus::Running);
    TaskStatus runStatus = runTask();
    if (_cancelFlag) runStatus = TaskStatus::Cancelled;
    setStatus(runStatus);
}

void KTask::updateProgress(int value)
{
    {
        QWriteLocker locker(&lock);
        _progress = value;
    }
    emit progreeUpdated(_progress);
}

void KTask::setStatus(TaskStatus newStatus)
{
    {
        QWriteLocker locker(&lock);
        _status = newStatus;
    }
    emit statusChanged(_status);
}

void KTask::setInfo(const QString &info, int flag)
{
    {
        QWriteLocker locker(&lock);
        _info = info;
    }
    emit infoChanged(_info, flag);
    Logger::logger()->log(Logger::APP, QString("[TaskInfo][%1]%2").arg(_name, info));
}

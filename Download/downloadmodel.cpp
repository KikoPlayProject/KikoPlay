#include "downloadmodel.h"
#include "globalobjects.h"
#include "aria2jsonrpc.h"
#include "trackersubscriber.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include "Common/network.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Common/dbmanager.h"

DownloadWorker *DownloadModel::downloadWorker=nullptr;
DownloadModel::DownloadModel(QObject *parent) : QAbstractItemModel(parent),currentOffset(0),
    hasMoreTasks(true),rpc(nullptr)
{
    downloadWorker=new DownloadWorker();
    downloadWorker->moveToThread(GlobalObjects::workThread);
    TrackerSubscriber::subscriber();
}

DownloadModel::~DownloadModel()
{
    saveItemStatus();
    qDeleteAll(downloadTasks);
}

void DownloadModel::setRPC(Aria2JsonRPC *aria2RPC)
{
    rpc=aria2RPC;
    //QObject::connect(rpc,&Aria2JsonRPC::refreshStatus,this,&DownloadModel::updateItemStatus);
}

void DownloadModel::addTask(DownloadTask *task)
{
    beginInsertRows(QModelIndex(),0,0);
    downloadTasks.prepend(task);
    endInsertRows();
    currentOffset++;
    //tasksMap.insert(task->taskID,task);
    if(!task->gid.isEmpty())
    {
        gidMap.insert(task->gid,task);
    }
    QMetaObject::invokeMethod(downloadWorker,[task](){
        downloadWorker->addTask(task);
    },Qt::QueuedConnection);
}

QString DownloadModel::addUriTask(const QString &uri, const QString &dir, bool directlyDownload)
{
    QString taskID(QCryptographicHash::hash(uri.toUtf8(),QCryptographicHash::Sha1).toHex());
    if(containTask(taskID))
        return QString(tr("The task already exists: \n%1").arg(uri));
    QString nUri(uri);
    if(nUri.startsWith("kikoplay:anime="))
        nUri = processKikoPlayCode(uri.mid(15));
    QJsonObject options;
    options.insert("dir", dir);
    options.insert("bt-metadata-only","true");
    options.insert("bt-save-metadata","true");
	options.insert("seed-time", QString::number(GlobalObjects::appSetting->value("Download/SeedTime", 5).toInt()));
    QStringList trackers(GlobalObjects::appSetting->value("Download/Trackers",QStringList()).toStringList());
    trackers.append(TrackerSubscriber::subscriber()->allTrackers());
    options.insert("bt-tracker", trackers.join(','));
    try
    {
       QString gid=rpc->addUri(nUri,options);
       DownloadTask *newTask=new DownloadTask();
       newTask->gid=gid;
       newTask->uri=nUri;
       newTask->createTime=QDateTime::currentSecsSinceEpoch();
       newTask->finishTime=0;
       newTask->taskID = taskID;
       newTask->dir=dir;
       newTask->title=nUri;
       newTask->directlyDownload=directlyDownload;
       addTask(newTask);
    }
    catch(RPCError &error)
    {
       return error.errorInfo;
    }
    return QString();
}

QString DownloadModel::addTorrentTask(const QByteArray &torrentContent, const QString &infoHash,
                                      const QString &dir, const QString &selIndexes, const QString &magnet)
{
    if(containTask(infoHash))
        return QString(tr("The task already exists: \n%1").arg(infoHash));
    QJsonObject options;
    options.insert("dir", dir);
    options.insert("rpc-save-upload-metadata","false");
    options.insert("select-file",selIndexes);
    options.insert("seed-time",QString::number(GlobalObjects::appSetting->value("Download/SeedTime",5).toInt()));
    QStringList trackers(GlobalObjects::appSetting->value("Download/Trackers",QStringList()).toStringList());
    trackers.append(TrackerSubscriber::subscriber()->allTrackers());
    options.insert("bt-tracker", trackers.join(','));
    try
    {
        QString gid=rpc->addTorrent(torrentContent.toBase64(),options);
        DownloadTask *btTask=new DownloadTask();
        btTask->gid=gid;
        btTask->createTime=QDateTime::currentSecsSinceEpoch();
        btTask->finishTime=0;
        btTask->taskID =infoHash;
        btTask->dir=dir;
        btTask->title=infoHash;
        btTask->torrentContent=torrentContent;
        btTask->selectedIndexes=selIndexes;
        btTask->uri=magnet;
        btTask->torrentContentState=1;
        addTask(btTask);
    }
    catch(RPCError &error)
    {
        return error.errorInfo;
    }
    return QString();
}

void DownloadModel::removeItem(DownloadTask *task, bool deleteFile)
{
    int row=downloadTasks.indexOf(task);
    if(row!=-1)
    {
        beginRemoveRows(QModelIndex(),row,row);
        downloadTasks.removeAt(row);
        endRemoveRows();
        currentOffset--;
    }
    QMetaObject::invokeMethod(downloadWorker,[task,deleteFile](){
        downloadWorker->deleteTask(task,deleteFile);
        //if(last)emit animeCountChanged();
    },Qt::QueuedConnection);
}

bool DownloadModel::containTask(const QString &taskId)
{
    QSqlQuery query(QSqlDatabase::database("Download_M"));
    query.prepare("select * from task where TaskID=?");
    query.bindValue(0,taskId);
    query.exec();
    if(query.first()) return true;
    return false;
}

QString DownloadModel::processKikoPlayCode(const QString &code)
{
    QByteArray base64Src(QByteArray::fromBase64(code.toUtf8()));
    QByteArray jsonBytes;
    if(Network::gzipDecompress(base64Src,jsonBytes)!=0) return QString();
    try
    {
        QJsonArray array(Network::toJson(jsonBytes).array());
        QString uri(array.takeAt(0).toString());
        QString animeTitle(array.takeAt(1).toString());
        QString epTitle(array.takeAt(2).toString());
        EpType epType((EpType)array.takeAt(3).toInt());
        double epIndex(array.takeAt(4).toDouble());
        QString file16MD5(array.takeAt(5).toString());
        QString pid = GlobalObjects::danmuManager->createPool(animeTitle,epType, epIndex, epTitle,file16MD5);
        Pool *pool=GlobalObjects::danmuManager->getPool(pid,false);
        pool->addPoolCode(array);
        return uri;
    } catch (Network::NetworkError &) {
        return QString();
    }
}

void DownloadModel::removeItem(QModelIndexList &removeIndexes, bool deleteFile)
{
    std::sort(removeIndexes.begin(),removeIndexes.end(),[](const QModelIndex &index1,const QModelIndex &index2){
        return index1.row()>index2.row();
    });
    for (const QModelIndex &index : removeIndexes)
    {
        if(!index.isValid())return;
        DownloadTask *task=downloadTasks.at(index.row());
        beginRemoveRows(QModelIndex(), index.row(), index.row());
        downloadTasks.removeAt(index.row());
        endRemoveRows();
        if(!task->gid.isEmpty())
        {
            gidMap.remove(task->gid);
            rpc->removeTask(task->gid);
        }
        //tasksMap.remove(task->taskID);
        //bool last=(index==removeIndexes.last());
        currentOffset--;
        QMetaObject::invokeMethod(downloadWorker,[task,deleteFile](){
            downloadWorker->deleteTask(task,deleteFile);
            //if(last)emit animeCountChanged();
        },Qt::QueuedConnection);
    }
}

void DownloadModel::updateItemStatus(const QJsonObject &statusObj)
{
    QString gid(statusObj.value("gid").toString());
    DownloadTask *item=gidMap.value(gid,nullptr);
    if(!item)return;
    item->totalLength=statusObj.value("totalLength").toString().toLongLong();
    item->completedLength=statusObj.value("completedLength").toString().toLongLong();
    item->downloadSpeed=statusObj.value("downloadSpeed").toString().toInt();
    item->uploadSpeed=statusObj.value("uploadSpeed").toString().toInt();
    item->connections=statusObj.value("connections").toString().toInt();
    item->numPieces=statusObj.value("numPieces").toString().toInt();
    item->pieceLength=statusObj.value("pieceLength").toString().toInt();
    item->bitfield=statusObj.value("bitfield").toString();
    if(statusObj.contains("numSeeders"))
    {
        item->seeders=statusObj.value("numSeeders").toString().toInt();
    }
    if(statusObj.contains("bittorrent"))
    {
        QJsonObject btObj(statusObj.value("bittorrent").toObject());
        QString newTitle(btObj.value("info").toObject().value("name").toString());
        if(newTitle!=item->title && !newTitle.isEmpty())
        {
            item->title=newTitle;
            QMetaObject::invokeMethod(downloadWorker,[item](){
                downloadWorker->updateTaskInfo(item);
            },Qt::QueuedConnection);
        }
    }
    else if(statusObj.contains("files"))
    {
        QJsonArray fileArray(statusObj.value("files").toArray());
        QString path(fileArray.first().toObject().value("path").toString());
        QFileInfo info(path);
        if(item->title!=info.fileName() && !info.fileName().isEmpty())
        {
            item->title=info.fileName();
            QMetaObject::invokeMethod(downloadWorker,[item](){
                downloadWorker->updateTaskInfo(item);
            },Qt::QueuedConnection);
        }
    }
    QString status(statusObj.value("status").toString());
    DownloadTask::Status newStatus;
    if (status == "active")
    {
        if (item->totalLength>0)
        {
            if (item->totalLength == item->completedLength && !statusObj.value("infoHash").isUndefined())
            {
                newStatus = DownloadTask::Seeding;
            }
            else
            {
                newStatus = DownloadTask::Downloading;
            }
        }
        else
        {
            newStatus = DownloadTask::Downloading;
        }
    }
    else if (status == "waiting")
    {
        newStatus = DownloadTask::Waiting;
    }
    else if (status == "paused")
    {
        newStatus = DownloadTask::Paused;
    }
    else if (status == "error")
    {
        newStatus = DownloadTask::Error;
    }
    else if (status == "complete")
    {
        newStatus = DownloadTask::Complete;
    }
    else
    {
        Q_ASSERT(false);
    }
    if (newStatus != item->status)
    {
        switch (newStatus)
        {
        case DownloadTask::Complete:
        {
            if (!statusObj.value("infoHash").isUndefined() && item->torrentContent.isEmpty())
            {
                QFileInfo info(item->dir,statusObj.value("infoHash").toString()+".torrent");
                gidMap.remove(gid);
                emit removeTask(gid);
                emit magnetDone(info.absoluteFilePath(),item->uri, item->directlyDownload);
                removeItem(item,true);
            }
            else
            {
                QMetaObject::invokeMethod(downloadWorker, [item]() {
                    downloadWorker->updateTaskInfo(item);
                }, Qt::QueuedConnection);
                if (item->status != DownloadTask::Seeding)
                {
                    emit taskFinish(item);
                }
            }
            break;
        }
        case DownloadTask::Seeding:
        {
            item->finishTime=QDateTime::currentSecsSinceEpoch();
            emit taskFinish(item);
            break;
        }
        default:
            break;
        }
        item->status=newStatus;
    }
    QModelIndex itemIndex(createIndex(downloadTasks.indexOf(item),0));
    emit dataChanged(itemIndex,itemIndex.sibling(itemIndex.row(),headers.count()-1));
}

DownloadTask *DownloadModel::getDownloadTask(const QModelIndex &index)
{
    if(!index.isValid())return nullptr;
    return downloadTasks.at(index.row());
}

void DownloadModel::tryLoadTorrentContent(DownloadTask *task)
{
    QSqlQuery query(QSqlDatabase::database("Download_M"));
    query.prepare("select Torrent from torrent where TaskID=?");
    query.bindValue(0,task->taskID);
    query.exec();
    if(query.first())
    {
        task->torrentContent=query.value(0).toByteArray();
        task->torrentContentState=1;
    }
    else
    {
        task->torrentContentState=0;
    }
}

QString DownloadModel::restartDownloadTask(DownloadTask *task, bool allowOverwrite)
{
    QJsonObject options;
    options.insert("dir", task->dir);
    if(task->torrentContentState==-1) tryLoadTorrentContent(task);
    if(task->torrentContent.isEmpty())
    {
        options.insert("bt-metadata-only","true");
        options.insert("bt-save-metadata","true");
    }
    else
    {
        options.insert("rpc-save-upload-metadata","false");
        options.insert("select-file",task->selectedIndexes);
        options.insert("seed-time",QString::number(GlobalObjects::appSetting->value("Download/SeedTime",5).toInt()));
        options.insert("bt-tracker",GlobalObjects::appSetting->value("Download/Trackers",QStringList()).toStringList().join(','));
    }
    if(allowOverwrite)
    {
        options.insert("allow-overwrite","true");
    }
    try
    {
       QString gid=task->torrentContent.isEmpty()?rpc->addUri(task->uri,options):
                                                  rpc->addTorrent(task->torrentContent.toBase64(),options);
       task->gid=gid;
       gidMap.insert(gid,task);
    }
    catch(RPCError &error)
    {
       return error.errorInfo;
    }
    return QString();
}

void DownloadModel::queryItemStatus()
{
    for(auto iter=gidMap.cbegin();iter!=gidMap.cend();++iter)
    {
        rpc->tellStatus(iter.key());
    }
}

void DownloadModel::saveItemStatus()
{
    for(auto iter=gidMap.begin();iter!=gidMap.end();++iter)
    {
        DownloadTask *task=iter.value();
        if(task->status!=DownloadTask::Complete)
        {
            QSqlQuery query(QSqlDatabase::database("Download_M"));
            query.prepare("update task set Title=?,FTime=?,TLength=?,CLength=?,SFIndexes=? where TaskID=?");
            query.bindValue(0,task->title);
            query.bindValue(1,task->finishTime);
            query.bindValue(2,task->totalLength);
            query.bindValue(3,task->completedLength);
            query.bindValue(4,task->selectedIndexes);
            query.bindValue(5,task->taskID);
            query.exec();
        }
    }
}

void DownloadModel::saveItemStatus(const DownloadTask *task)
{
    QMetaObject::invokeMethod(downloadWorker,[task](){
        downloadWorker->updateTaskInfo(task);
    },Qt::QueuedConnection);
}

QString DownloadModel::findFileUri(const QString &fileName)
{
    QFileInfo fi(fileName);
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Download));
    query.exec(QString("select Dir,URI from task where Title='%1'").arg(fi.fileName()));
    int dirNo=query.record().indexOf("Dir"),uriNo=query.record().indexOf("URI");
    QString uri;
    while (query.next())
    {
        QFileInfo tfi(query.value(dirNo).toString(),fi.fileName());
        if(tfi==fi)
        {
            uri=query.value(uriNo).toString();
            break;
        }
    }
    return uri;
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    const DownloadTask *downloadItem=downloadTasks.at(index.row());
    Columns col=static_cast<Columns>(index.column());
    switch (col)
    {
    case Columns::STATUS:
        if(role==Qt::DisplayRole)
            return status.at(downloadItem->status);
        else if(role==Qt::DecorationRole)
            return statusIcons[downloadItem->status];
        break;
    case Columns::TITLE:
        if(role==Qt::DisplayRole || role==Qt::ToolTipRole)
            return downloadItem->title;
        break;
    case Columns::SIZE:
        if(role==Qt::DisplayRole)
            return formatSize(false,downloadItem->totalLength);
        break;
    case Columns::DOWNSPEED:
        if(role==Qt::DisplayRole)
            return formatSize(true,downloadItem->downloadSpeed);
        break;
    case Columns::TIMELEFT:
        if(role==Qt::DisplayRole)
        {
            if(downloadItem->downloadSpeed==0)return "--";
            qint64 leftLength(downloadItem->totalLength-downloadItem->completedLength);
            int totalSecond(ceil((double)leftLength/downloadItem->downloadSpeed));
            if(totalSecond>24*60*60)return ">24h";
            int minuteLeft(totalSecond/60);
            int secondLeft=totalSecond-minuteLeft*60;
            if(minuteLeft>60)
            {
                int hourLeft=minuteLeft/60;
                minuteLeft=minuteLeft-hourLeft*60;
                return QString("%1:%2:%3").arg(hourLeft,2,10,QChar('0')).arg(minuteLeft,2,10,QChar('0')).arg(secondLeft,2,10,QChar('0'));
            }
            else
            {
                return QString("%1:%2").arg(minuteLeft,2,10,QChar('0')).arg(secondLeft,2,10,QChar('0'));
            }
        }
        break;
    case Columns::UPSPEED:
        if(role==Qt::DisplayRole)
            return formatSize(true, downloadItem->uploadSpeed);
        break;
    case Columns::CONNECTION:
        if(role==Qt::DisplayRole)
            return downloadItem->connections;
        break;
    case Columns::SEEDER:
        if(role==Qt::DisplayRole)
            return downloadItem->seeders;
        break;
    case Columns::DIR:
        if(role==Qt::DisplayRole || role==Qt::ToolTipRole)
            return downloadItem->dir;
        break;
    case Columns::CREATETIME:
        if(role == Qt::DisplayRole)
            return QDateTime::fromSecsSinceEpoch(downloadItem->createTime).toString("yyyy-MM-dd hh:mm:ss");
    case Columns::FINISHTIME:
        if(role == Qt::DisplayRole)
            return downloadItem->finishTime < downloadItem->createTime? "--" : QDateTime::fromSecsSinceEpoch(downloadItem->finishTime).toString("yyyy-MM-dd hh:mm:ss");
    default:
        break;
    }
    switch (role)
    {
    case DataRole::DownSpeedRole:
        return downloadItem->downloadSpeed;
    case DataRole::TotalLengthRole:
        return downloadItem->totalLength;
    case DataRole::CompletedLengthRole:
        return downloadItem->completedLength;
    case DataRole::DownloadTaskRole:
        return QVariant::fromValue((void *)downloadItem);
    }
    return QVariant();
}

QVariant DownloadModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.count())return headers.at(section);
    }
    return QVariant();
}

void DownloadModel::fetchMore(const QModelIndex &)
{
    QVector<DownloadTask *> moreTasks;
    QEventLoop eventLoop;
    QObject::connect(downloadWorker, &DownloadWorker::loadDone, &eventLoop, &QEventLoop::quit);
    hasMoreTasks = false;
    QMetaObject::invokeMethod(downloadWorker,[this,&moreTasks](){
        downloadWorker->loadTasks(moreTasks,currentOffset,limitCount);
    },Qt::QueuedConnection);
    eventLoop.exec();
    if(moreTasks.count()>0)
    {
		hasMoreTasks = true;
        beginInsertRows(QModelIndex(),downloadTasks.count(),downloadTasks.count()+moreTasks.count()-1);
        downloadTasks.append(moreTasks);
        endInsertRows();
        currentOffset+=moreTasks.count();
    }
}

void DownloadWorker::loadTasks(QVector<DownloadTask *> &items, int offset, int limit)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Download));
    query.exec(QString("select * from task order by CTime desc limit %1 offset %2").arg(limit).arg(offset));
    int idNo=query.record().indexOf("TaskID"),
        titleNo=query.record().indexOf("Title"),
        dirNo=query.record().indexOf("Dir"),
        cTimeNo=query.record().indexOf("CTime"),
        fTimeNo=query.record().indexOf("FTime"),
        tLengthNo=query.record().indexOf("TLength"),
        cLengthNo=query.record().indexOf("CLength"),
        uriNo=query.record().indexOf("URI"),
        sfIndexNo=query.record().indexOf("SFIndexes");
        //torrentNo=query.record().indexOf("Torrent");
    int count=0;
    while (query.next())
    {
        DownloadTask *task=new DownloadTask();
        task->taskID=query.value(idNo).toString();
        task->dir=query.value(dirNo).toString();
        task->title=query.value(titleNo).toString();
        task->createTime=query.value(cTimeNo).toLongLong();
        task->finishTime=query.value(fTimeNo).toLongLong();
        task->totalLength=query.value(tLengthNo).toLongLong();
        task->completedLength=query.value(cLengthNo).toLongLong();
        task->status=((task->totalLength==task->completedLength && task->totalLength>0)?DownloadTask::Complete:DownloadTask::Paused);
        task->uri=query.value(uriNo).toString();
        task->selectedIndexes=query.value(sfIndexNo).toString();
        //task->torrentContent=query.value(torrentNo).toByteArray();
        items.append(task);
        count++;
    }
    emit loadDone(count);
}

void DownloadWorker::addTask(DownloadTask *task)
{
    QSqlDatabase db = DBManager::instance()->getDB(DBManager::Download);
    QSqlQuery query(db);
    db.transaction();
    query.prepare("insert into task(TaskID,Dir,CTime,URI,SFIndexes) values(?,?,?,?,?)");
    query.bindValue(0,task->taskID);
    query.bindValue(1,task->dir);
    query.bindValue(2,task->createTime);
    query.bindValue(3,task->uri);
    query.bindValue(4,task->selectedIndexes);
    query.exec();    
    if(!task->torrentContent.isEmpty())
    {
        query.prepare("insert into torrent(TaskID,Torrent) values(?,?)");
        query.bindValue(0,task->taskID);
        query.bindValue(1,task->torrentContent);
        query.exec();
    }
    db.commit();
}

void DownloadWorker::deleteTask(DownloadTask *task, bool deleteFile)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Download));
    query.prepare("delete from task where TaskID=?");
    query.bindValue(0,task->taskID);
    query.exec();
    if(deleteFile && !task->title.isEmpty())
    {
        QFileInfo fi(task->dir,task->title);
        if(fi.exists())
        {
            if(fi.isDir())
            {
                QDir dir(fi.absoluteFilePath());
                if(!dir.removeRecursively())
                {
                    qInfo() << "delete failed: " << fi.absoluteFilePath();
                }
            }
            else
            {
                if(!fi.dir().remove(fi.fileName()))
                {
                    qInfo() << "delete failed: " << fi.fileName();
                }
            }
        }
        fi.setFile(task->dir,task->title+".aria2");
        if(fi.exists())
        {
            fi.dir().remove(fi.fileName());
        }
    }
    delete task;
}

void DownloadWorker::updateTaskInfo(const DownloadTask *task)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Download));
    query.prepare("update task set Title=?,FTime=?,TLength=?,CLength=?,SFIndexes=? where TaskID=?");
    query.bindValue(0,task->title);
    query.bindValue(1,task->finishTime);
    query.bindValue(2,task->totalLength);
    query.bindValue(3,task->completedLength);
    query.bindValue(4,task->selectedIndexes);
    query.bindValue(5,task->taskID);
    query.exec();
}

bool TaskFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    DownloadTask *task=static_cast<DownloadModel *>(sourceModel())->getDownloadTask(index);
    if(!task->title.contains(filterRegularExpression())) return false;
    switch (taskStatus)
    {
    case 0: //all
        return true;
    case 1: //active
        return task->status==DownloadTask::Downloading || task->status==DownloadTask::Error ||
                task->status==DownloadTask::Waiting || task->status==DownloadTask::Paused;
    case 2: //completed
        return task->status==DownloadTask::Complete || task->status==DownloadTask::Seeding;
    default:
        return true;
    }
}

bool TaskFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    int col=source_left.column();
    if(col==static_cast<int>(DownloadModel::Columns::SIZE))
    {
        qint64 ltotalLength=sourceModel()->data(source_left,DownloadModel::DataRole::TotalLengthRole).toLongLong(),
               rtotalLength=sourceModel()->data(source_right,DownloadModel::DataRole::TotalLengthRole).toLongLong();
        return ltotalLength<rtotalLength;
    }
    else if(col==static_cast<int>(DownloadModel::Columns::DOWNSPEED))
    {
        int lspeed=sourceModel()->data(source_left,DownloadModel::DataRole::DownSpeedRole).toLongLong(),
               rspeed=sourceModel()->data(source_right,DownloadModel::DataRole::DownSpeedRole).toLongLong();
        return lspeed<rspeed;
    }
    else if(col==static_cast<int>(DownloadModel::Columns::UPSPEED))
    {
        int lspeed=sourceModel()->data(source_left,DownloadModel::DataRole::UpSpeedRole).toLongLong(),
               rspeed=sourceModel()->data(source_right,DownloadModel::DataRole::UpSpeedRole).toLongLong();
        return lspeed<rspeed;
    }
    return QSortFilterProxyModel::lessThan(source_left,source_right);
}

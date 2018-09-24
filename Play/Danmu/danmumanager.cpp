#include "danmumanager.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QXmlStreamWriter>
#include <QFile>
#include <QFileInfo>

#include "common.h"
#include "globalobjects.h"
#include "danmupool.h"
PoolInfoWorker *DanmuManager::poolWorker=nullptr;
DanmuManager::DanmuManager(QObject *parent) : QAbstractItemModel(parent)
{
    poolWorker=new PoolInfoWorker();
    poolWorker->moveToThread(GlobalObjects::workThread);
}

void DanmuManager::refreshPoolInfo()
{
    QEventLoop eventLoop;
    QObject::connect(poolWorker,&PoolInfoWorker::loadDone, &eventLoop,&QEventLoop::quit);
    beginResetModel();
    poolList.clear();
    QMetaObject::invokeMethod(poolWorker,[this](){
        poolWorker->loadPoolInfo(poolList);
    },Qt::QueuedConnection);
    eventLoop.exec();
    endResetModel();
}

void DanmuManager::exportPool(QModelIndexList &exportIndexes, const QString &dir)
{
    QList<DanmuPoolInfo> exportList;
    for(const QModelIndex &index:exportIndexes)
    {
        if(!index.isValid())continue;
        exportList.append(poolList.at(index.row()));
    }
    if(exportList.count()==0) return;
    QEventLoop eventLoop;
    QObject::connect(poolWorker,&PoolInfoWorker::exportDone, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(poolWorker,[&exportList,&dir](){
        poolWorker->exportPool(exportList,dir);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

void DanmuManager::deletePool(QModelIndexList &deleteIndexes)
{
    QList<DanmuPoolInfo> deleteList;
    QList<int> rows;
    for(const QModelIndex &index:deleteIndexes)
    {
        if(!index.isValid())continue;
        if(poolList.at(index.row()).poolID==GlobalObjects::danmuPool->getPoolID())continue;
        deleteList.append(poolList.at(index.row()));
        rows.append(index.row());
    }
    if(deleteList.count()==0) return;
    std::sort(rows.rbegin(),rows.rend());
    for(auto iter=rows.begin();iter!=rows.end();++iter)
    {
        beginRemoveRows(QModelIndex(), *iter, *iter);
        poolList.removeAt(*iter);
        endRemoveRows();
    }
    QEventLoop eventLoop;
    QObject::connect(poolWorker,&PoolInfoWorker::deleteDone, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(poolWorker,[&deleteList](){
        poolWorker->deletePool(deleteList);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

QVariant DanmuManager::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    const DanmuPoolInfo &poolInfo=poolList.at(index.row());
    int col=index.column();
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col==0)
        {
            return poolInfo.animeTitle;
        }
        else if(col==1)
        {
            return poolInfo.epTitle;
        }
        else if(col==2)
        {
            return poolInfo.danmuCount;
        }
    }
    }
    return QVariant();
}

QVariant DanmuManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QString headers[]={tr("Anime"),tr("Episode"),tr("Danmu Count")};
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<3)return headers[section];
    }
    return QVariant();
}

void PoolInfoWorker::loadPoolInfo(QList<DanmuPoolInfo> &poolInfoList)
{
    QSqlQuery query(QSqlDatabase::database("WT"));
    query.exec("select * from bangumi");
    int idNo = query.record().indexOf("PoolID"),
        animeNo=query.record().indexOf("AnimeTitle"),
        epNo=query.record().indexOf("Title");
    while (query.next())
    {
        DanmuPoolInfo poolInfo;
        poolInfo.poolID=query.value(idNo).toString();
        poolInfo.animeTitle=query.value(animeNo).toString();
        poolInfo.epTitle=query.value(epNo).toString();
        poolInfoList.append(poolInfo);
    }
    query.exec("select PoolID,count(*) as DanmuCount from danmu group by PoolID");
    int pidNo = query.record().indexOf("PoolID"),
        countNo=query.record().indexOf("DanmuCount");
    QMap<QString,int> danmuCount;
    while (query.next())
    {
        danmuCount.insert(query.value(pidNo).toString(),query.value(countNo).toInt());
    }
    for(DanmuPoolInfo &poolInfo:poolInfoList)
    {

        poolInfo.danmuCount = danmuCount.value(poolInfo.poolID,0);
    }
    emit loadDone();
}

void PoolInfoWorker::exportPool(const QList<DanmuPoolInfo> &exportList, const QString &dir)
{
    QSqlQuery query(QSqlDatabase::database("WT"));
    DanmuComment tmpComment;
    for(const DanmuPoolInfo &poolInfo:exportList)
    {
        static int type[3]={1,5,4};
        static int fontSize[3]={25,28,36};
        QString animeTitle(poolInfo.animeTitle),epTitle(poolInfo.epTitle);
        animeTitle.replace(QRegExp("[\\\\/:*?\"<>|]"),"");
        epTitle.replace(QRegExp("[\\\\/:*?\"<>|]"),"");
        QFileInfo fi(dir,QString("%1-%2.xml").arg(animeTitle).arg(epTitle));
        QFile danmuFile(fi.absoluteFilePath());
        bool ret=danmuFile.open(QIODevice::WriteOnly|QIODevice::Text);
        if(!ret) break;
        QXmlStreamWriter writer(&danmuFile);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        writer.writeStartElement("i");

        QHash<int,int> sourcesTable;
        query.exec(QString("select ID,Delay from source where PoolID='%1'").arg(poolInfo.poolID));
        int idNo = query.record().indexOf("ID"),
            delayNo=query.record().indexOf("Delay");
        while (query.next())
        {
            sourcesTable.insert(query.value(idNo).toInt(),query.value(delayNo).toInt());
        }
        query.exec(QString("select * from danmu where PoolID='%1'").arg(poolInfo.poolID));
        int timeNo = query.record().indexOf("Time"),
            dateNo=query.record().indexOf("Date"),
            colorNo=query.record().indexOf("Color"),
            modeNo=query.record().indexOf("Mode"),
            sizeNo=query.record().indexOf("Size"),
            sourceNo=query.record().indexOf("Source"),
            userNo=query.record().indexOf("User"),
            textNo=query.record().indexOf("Text");
        while (query.next())
        {
            tmpComment.color=query.value(colorNo).toInt();
            tmpComment.date=query.value(dateNo).toLongLong();
            tmpComment.fontSizeLevel=DanmuComment::FontSizeLevel(query.value(sizeNo).toInt());
            tmpComment.sender=query.value(userNo).toString();
            tmpComment.type=DanmuComment::DanmuType(query.value(modeNo).toInt());
            tmpComment.source=query.value(sourceNo).toInt();
            tmpComment.text=query.value(textNo).toString();
            int delay=0;
            if(sourcesTable.contains(tmpComment.source))
            {
                delay=sourcesTable[tmpComment.source];
            }
            int time = query.value(timeNo).toInt();
            tmpComment.time=time+delay<0?time:time+delay;

            writer.writeStartElement("d");
            writer.writeAttribute("p", QString("%0,%1,%2,%3,%4,%5,%6,%7").arg(QString::number(tmpComment.time/1000.f,'f',2))
                                  .arg(type[tmpComment.type]).arg(fontSize[tmpComment.fontSizeLevel]).arg(tmpComment.color)
                                  .arg(tmpComment.date).arg("0").arg(tmpComment.sender).arg("0"));
            writer.writeCharacters(tmpComment.text);
            writer.writeEndElement();
        }
        writer.writeEndElement();
        writer.writeEndDocument();
        danmuFile.close();
    }
    emit exportDone();
}

void PoolInfoWorker::deletePool(const QList<DanmuPoolInfo> &deleteList)
{
    QSqlDatabase db=QSqlDatabase::database("WT");
    QSqlQuery query(QSqlDatabase::database("WT"));
    query.prepare("delete from bangumi where PoolID=?");
    db.transaction();
    for(const DanmuPoolInfo &poolInfo:deleteList)
    {
        query.bindValue(0,poolInfo.poolID);
        query.exec();
    }
    db.commit();
    emit deleteDone();
}

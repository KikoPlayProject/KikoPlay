#include "danmumanager.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QXmlStreamWriter>
#include <QFile>
#include <QFileInfo>

#include "../common.h"
#include "../blocker.h"
#include "../danmupool.h"
#include "globalobjects.h"

DanmuManager::DanmuManager(QObject *parent) : QObject(parent)
{
    poolWorker = new PoolWorker;
    poolWorker->moveToThread(GlobalObjects::workThread);
	qRegisterMetaType<QList<DanmuComment *> >("QList<DanmuComment *>");
    QObject::connect(poolWorker,&PoolWorker::stateMessage,this,&DanmuManager::workerStateMessage);
    QObject::connect(poolWorker,&PoolWorker::addToCurPool,this,[](const QString &idInfo, QList<DanmuComment *> danmuList){
        DanmuSourceInfo srcInfo;
        srcInfo.url=idInfo;
        GlobalObjects::danmuPool->addDanmu(srcInfo,danmuList);
    });
}

DanmuManager::~DanmuManager()
{
    poolWorker->deleteLater();
}

void DanmuManager::loadPoolInfo(QList<DanmuPoolNode *> &poolNodeList)
{
    QEventLoop eventLoop;
    QObject::connect(poolWorker,&PoolWorker::loadDone, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(poolWorker,[&poolNodeList,this](){
        poolWorker->loadPoolInfo(poolNodeList);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

void DanmuManager::loadPool(const QString &pid, QList<QSharedPointer<DanmuComment> > &danmuList, QHash<int, DanmuSourceInfo> &sourcesTable)
{
    QSqlQuery query(QSqlDatabase::database("Comment_M"));
    query.exec(QString("select * from source where PoolID='%1'").arg(pid));
    int idNo = query.record().indexOf("ID"),
        nameNo=query.record().indexOf("Name"),
        delayNo=query.record().indexOf("Delay"),
        urlNo=query.record().indexOf("URL"),
        timelineNo=query.record().indexOf("TimeLine");
    sourcesTable.clear();
    while (query.next())
    {
        DanmuSourceInfo sourceInfo;
        sourceInfo.delay=query.value(delayNo).toInt();
        sourceInfo.id=query.value(idNo).toInt();
        sourceInfo.name=query.value(nameNo).toString();
        sourceInfo.url=query.value(urlNo).toString();
        sourceInfo.show=true;
        sourceInfo.count=0;
        QStringList timelineList(query.value(timelineNo).toString().split(';',QString::SkipEmptyParts));
        QTextStream ts;
        for(QString &spaceInfo:timelineList)
        {
            ts.setString(&spaceInfo,QIODevice::ReadOnly);
            int start,duration;
            ts>>start>>duration;
            sourceInfo.timelineInfo.append(QPair<int,int>(start,duration));
        }
        std::sort(sourceInfo.timelineInfo.begin(),sourceInfo.timelineInfo.end(),[](const QPair<int,int> &s1,const QPair<int,int> &s2){
            return s1.first<s2.first;
        });
        sourcesTable.insert(sourceInfo.id,sourceInfo);
    }
    int tableId=DanmuPoolNode::idHash(pid);
    query.exec(QString("select * from danmu_%1 where PoolID='%2'").arg(tableId).arg(pid));
    int timeNo = query.record().indexOf("Time"),
        dateNo=query.record().indexOf("Date"),
        colorNo=query.record().indexOf("Color"),
        modeNo=query.record().indexOf("Mode"),
        sizeNo=query.record().indexOf("Size"),
        sourceNo=query.record().indexOf("Source"),
        userNo=query.record().indexOf("User"),
        textNo=query.record().indexOf("Text");
    danmuList.clear();
    while (query.next())
    {
        DanmuComment *danmu=new DanmuComment();
        danmu->color=query.value(colorNo).toInt();
        danmu->date=query.value(dateNo).toLongLong();
        danmu->fontSizeLevel=DanmuComment::FontSizeLevel(query.value(sizeNo).toInt());
        danmu->sender=query.value(userNo).toString();
        danmu->type=DanmuComment::DanmuType(query.value(modeNo).toInt());
        danmu->source=query.value(sourceNo).toInt();
        danmu->text=query.value(textNo).toString();
        danmu->originTime=query.value(timeNo).toInt();
        if(danmu->text.isEmpty())
        {
            delete danmu;
            continue;
        }
        Q_ASSERT(sourcesTable.contains(danmu->source));
        setDelay(danmu,&sourcesTable[danmu->source]);
        sourcesTable[danmu->source].count++;
        danmuList.append(QSharedPointer<DanmuComment>(danmu));
    }
}

void DanmuManager::loadSimpleDanmuInfo(const QString &pid, int srcId, QList<SimpleDanmuInfo> &simpleDanmuList)
{
    int tableId=DanmuPoolNode::idHash(pid);
    QSqlQuery query(QSqlDatabase::database("Comment_M"));
    query.exec(QString("select Time,Text from danmu_%1 where PoolID='%2' and Source=%3").arg(tableId).arg(pid).arg(srcId));
    int timeNo = query.record().indexOf("Time"),
        textNo=query.record().indexOf("Text");
    while (query.next())
    {
        SimpleDanmuInfo sd;
        sd.text=query.value(textNo).toString();
        sd.originTime=query.value(timeNo).toInt();
        sd.time=sd.originTime;
        simpleDanmuList.append(sd);
    }
}

void DanmuManager::exportPool(const QList<DanmuPoolNode *> &exportList, const QString &dir, bool useTimeline, bool applyBlockRule)
{
    QEventLoop eventLoop;
    QObject::connect(poolWorker,&PoolWorker::exportDone, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(poolWorker,[this,&exportList,&dir,useTimeline,applyBlockRule](){
        poolWorker->exportPool(exportList,dir,useTimeline,applyBlockRule);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

void DanmuManager::exportPool(const QString &pid, const QString &fileName)
{
    QEventLoop eventLoop;
    QObject::connect(poolWorker,&PoolWorker::exportSingleDown, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(poolWorker,[this,&pid,&fileName](){
        poolWorker->exportPool(pid,fileName);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

QJsonObject DanmuManager::exportJson(const QString &pid)
{
    QEventLoop eventLoop;
    QJsonObject retObj;
    QObject::connect(poolWorker,&PoolWorker::exportJsonDown, &eventLoop,[&eventLoop,&retObj](QJsonObject jsonObj){
        eventLoop.quit();
        retObj = jsonObj;
    });
    QMetaObject::invokeMethod(poolWorker,[this,&pid](){
        poolWorker->exportJson(pid);
    },Qt::QueuedConnection);
    eventLoop.exec();
    return retObj;
}

void DanmuManager::deletePool(const QList<DanmuPoolNode *> &deleteList)
{
    QEventLoop eventLoop;
    QObject::connect(poolWorker,&PoolWorker::deleteDone, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(poolWorker,[this, &deleteList](){
        poolWorker->deletePool(deleteList);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

void DanmuManager::deleteSource(const QString &pid, int srcId)
{
    QSqlDatabase db = QSqlDatabase::database("Comment_M");
    QSqlQuery query(QSqlDatabase::database("Comment_M"));
    db.transaction();
    query.exec(QString("delete from source where PoolID='%1' and ID=%2").arg(pid).arg(srcId));
    int tableId=DanmuPoolNode::idHash(pid);
    query.exec(QString("delete from danmu_%1 where PoolID='%2' and Source=%3").arg(tableId).arg(pid).arg(srcId));
    db.commit();
}

void DanmuManager::deleteDanmu(const QString &pid, const DanmuComment *danmu)
{
    QSqlQuery query(QSqlDatabase::database("Comment_M"));
    int tableId=DanmuPoolNode::idHash(pid);
    query.prepare(QString("delete from danmu_%1 where PoolID=? and Date=? and User=? and Text=? and Source=?").arg(tableId));
    query.bindValue(0,pid);
    query.bindValue(1,danmu->date);
    query.bindValue(2,danmu->sender);
    query.bindValue(3,danmu->text);
    query.bindValue(4,danmu->source);
    query.exec();
}

void DanmuManager::updatePool(QList<DanmuPoolNode *> &updateList)
{
    QEventLoop eventLoop;
    QObject::connect(poolWorker,&PoolWorker::updateDone, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(poolWorker,[this, &updateList](){
        poolWorker->updatePool(updateList);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

void DanmuManager::saveSource(const QString &pid, const DanmuSourceInfo *sourceInfo, const QList<DanmuComment *> *danmuList)
{
    QSqlDatabase db = QSqlDatabase::database("Comment_M");
    QSqlQuery query(db);
    db.transaction();
    if(sourceInfo)
    {
        query.prepare("insert into source(PoolID,ID,Name,Delay,URL,TimeLine) values(?,?,?,?,?,?)");
        query.bindValue(0,pid);
        query.bindValue(1,sourceInfo->id);
        query.bindValue(2,sourceInfo->name);
        query.bindValue(3,sourceInfo->delay);
        query.bindValue(4,sourceInfo->url);

        QString timelineInfo;
        QTextStream ts(&timelineInfo);
        for(auto &spaceItem:sourceInfo->timelineInfo)
        {
            ts<<spaceItem.first<<' '<<spaceItem.second<<';';
        }
        ts.flush();
        query.bindValue(5,timelineInfo);
        query.exec();
    }
    if(danmuList)
    {
        int tableId=DanmuPoolNode::idHash(pid);
        query.prepare(QString("insert into danmu_%1(PoolID,Time,Date,Color,Mode,Size,Source,User,Text) values(?,?,?,?,?,?,?,?,?)").arg(tableId));
        for(DanmuComment *danmu:*danmuList)
        {
            if(danmu->text.isEmpty()) continue;
            query.bindValue(0,pid);
            query.bindValue(1,danmu->originTime);
            query.bindValue(2,danmu->date);
            query.bindValue(3,danmu->color);
            query.bindValue(4,(int)danmu->type);
            query.bindValue(5,(int)danmu->fontSizeLevel);
            query.bindValue(6,danmu->source);
            query.bindValue(7,danmu->sender);
            query.bindValue(8,danmu->text);
            query.exec();
        }
    }
    db.commit();
}

void DanmuManager::updateSourceDelay(const QString &pid, const DanmuSourceInfo *sourceInfo)
{
    QSqlQuery query(QSqlDatabase::database("Comment_M"));
    query.prepare("update source set Delay= ? where PoolID=? and ID=?");
    query.bindValue(0,sourceInfo->delay);
    query.bindValue(1,pid);
    query.bindValue(2,sourceInfo->id);
    query.exec();
}

void DanmuManager::updateSourceDelay(const DanmuPoolSourceNode *srcNode)
{
    QSqlQuery query(QSqlDatabase::database("Comment_M"));
    query.prepare("update source set Delay= ? where PoolID=? and ID=?");
    query.bindValue(0,srcNode->delay);
    query.bindValue(1,srcNode->parent->idInfo);
    query.bindValue(2,srcNode->srcId);
    query.exec();
}

void DanmuManager::updateSourceTimeline(const QString &pid, const DanmuSourceInfo *sourceInfo)
{
    QSqlQuery query(QSqlDatabase::database("Comment_M"));
    query.prepare("update source set TimeLine= ? where PoolID=? and ID=?");
    QString timelineInfo;
    QTextStream ts(&timelineInfo);
    for(auto &spaceItem:sourceInfo->timelineInfo)
    {
        ts<<spaceItem.first<<' '<<spaceItem.second<<';';
    }
    ts.flush();
    query.bindValue(0,timelineInfo);
    query.bindValue(1,pid);
    query.bindValue(2,sourceInfo->id);
    query.exec();
}

void DanmuManager::updateSourceTimeline(const DanmuPoolSourceNode *srcNode)
{
    QSqlQuery query(QSqlDatabase::database("Comment_M"));
    query.prepare("update source set TimeLine= ? where PoolID=? and ID=?");
    query.bindValue(0,srcNode->timeline);
    query.bindValue(1,srcNode->parent->idInfo);
    query.bindValue(2,srcNode->srcId);
    query.exec();
}

void DanmuManager::setDelay(DanmuComment *danmu, DanmuSourceInfo *srcInfo)
{
    int delay=0;
    for(auto &spaceItem:srcInfo->timelineInfo)
    {
        if(danmu->originTime>spaceItem.first)delay+=spaceItem.second;
        else break;
    }
    delay+=srcInfo->delay;
    danmu->time=danmu->originTime+delay<0?danmu->originTime:danmu->originTime+delay;
}

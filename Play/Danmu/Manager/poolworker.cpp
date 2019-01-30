#include "poolworker.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include "../common.h"
#include "../blocker.h"
#include "../providermanager.h"
#include "../danmupool.h"
#include "danmumanager.h"
#include "globalobjects.h"

void PoolWorker::loadPoolInfo(QList<DanmuPoolNode *> &poolNodeList)
{
    QSqlQuery query(QSqlDatabase::database("Comment_W"));
    QMap<QString,DanmuPoolNode *> animeMap,epMap;
    //get all danmu pools
    query.exec("select * from pool");
    int idNo = query.record().indexOf("PoolID"),
        animeNo=query.record().indexOf("AnimeTitle"),
        epNo=query.record().indexOf("Title");
    while (query.next())
    {
        QString animeTitle(query.value(animeNo).toString());
        DanmuPoolNode *animeNode=animeMap.value(animeTitle,nullptr);
        if(!animeNode)
        {
            animeNode=new DanmuPoolNode(DanmuPoolNode::AnimeNode);
            animeNode->title=animeTitle;
            animeMap.insert(animeTitle,animeNode);
			poolNodeList.append(animeNode);
        }
        QString poolId(query.value(idNo).toString());
        Q_ASSERT(!epMap.contains(poolId));
        DanmuPoolNode *epNode=new DanmuPoolNode(DanmuPoolNode::EpNode,animeNode);
        epMap.insert(poolId,epNode);
        epNode->title=query.value(epNo).toString();
        epNode->idInfo=poolId;
    }
    //get source info
    query.exec("select * from source");
    int s_pidNo = query.record().indexOf("PoolID"),
        s_idNo = query.record().indexOf("ID"),
        s_nameNo = query.record().indexOf("Name"),
        s_urlNo=query.record().indexOf("URL"),
        s_delayNo=query.record().indexOf("Delay"),
        s_timelineNo=query.record().indexOf("TimeLine");
    while (query.next())
    {
        DanmuPoolNode *epNode=epMap.value(query.value(s_pidNo).toString(),nullptr);
        Q_ASSERT(epNode);
        DanmuPoolSourceNode *sourceNode=new DanmuPoolSourceNode(epNode);
        sourceNode->title=query.value(s_nameNo).toString();
        sourceNode->idInfo=query.value(s_urlNo).toString();
        sourceNode->srcId=query.value(s_idNo).toInt();
        sourceNode->delay=query.value(s_delayNo).toInt();
        sourceNode->timeline=query.value(s_timelineNo).toString();
    }
    //get danmu count
    for (int i = 0; i < DanmuTableCount; ++i)
    {
        query.exec(QString("select PoolID,Source,count(*) as DanmuCount from danmu_%1 group by PoolID,Source").arg(i));
        int pidNo = query.record().indexOf("PoolID"),
            srcNo = query.record().indexOf("Source"),
            countNo=query.record().indexOf("DanmuCount");
        while (query.next())
        {
            DanmuPoolNode *epNode=epMap.value(query.value(pidNo).toString(),nullptr);
            Q_ASSERT(epNode);
            int src_id=query.value(srcNo).toInt();
            for(DanmuPoolNode *srcNode:*epNode->children)
            {
                if(static_cast<DanmuPoolSourceNode *>(srcNode)->srcId==src_id)
                {
                    srcNode->danmuCount=query.value(countNo).toInt();
                    break;
                }
            }
        }
    }

    for(DanmuPoolNode *animeNode:poolNodeList)
    {
        animeNode->setCount();
    }
    emit loadDone();
    emit stateMessage("Done");
}

void PoolWorker::exportPool(const QList<DanmuPoolNode *> &exportList, const QString &dir, bool useTimeline, bool applyBlockRule)
{
    for(const DanmuPoolNode *node:exportList)
    {
        if(node->type==DanmuPoolNode::AnimeNode && node->checkStatus!=Qt::Unchecked)
        {
            QString animeTitle(node->title);
            animeTitle.replace(QRegExp("[\\\\/:*?\"<>|]"),"");
            for(DanmuPoolNode *epNode:*node->children)
            {
                if(epNode->checkStatus==Qt::Unchecked)continue;
                QString epTitle(epNode->title);
                epTitle.replace(QRegExp("[\\\\/:*?\"<>|]"),"");
                QFileInfo fi(dir,QString("%1-%2.xml").arg(animeTitle).arg(epTitle));
                emit stateMessage(tr("Exporting: %1").arg(fi.fileName()));
                exportEp(epNode,fi.absoluteFilePath(),useTimeline,applyBlockRule);
            }
        }
    }
    emit exportDone();
    emit stateMessage("Done");
}

void PoolWorker::exportPool(const QString &pid, const QString &fileName)
{
    QSqlQuery query(QSqlDatabase::database("Comment_W"));
    query.exec(QString("select * from source where PoolID='%1'").arg(pid));
    int s_idNo = query.record().indexOf("ID"),
        s_nameNo = query.record().indexOf("Name"),
        s_urlNo=query.record().indexOf("URL"),
        s_delayNo=query.record().indexOf("Delay"),
        s_timelineNo=query.record().indexOf("TimeLine");
    DanmuPoolNode epNode(DanmuPoolNode::EpNode);
    epNode.checkStatus=Qt::Checked;
	epNode.idInfo = pid;
    while (query.next())
    {
        DanmuPoolSourceNode *sourceNode=new DanmuPoolSourceNode(&epNode);
        sourceNode->title=query.value(s_nameNo).toString();
        sourceNode->idInfo=query.value(s_urlNo).toString();
        sourceNode->srcId=query.value(s_idNo).toInt();
        sourceNode->delay=query.value(s_delayNo).toInt();
        sourceNode->timeline=query.value(s_timelineNo).toString();
        sourceNode->checkStatus=Qt::Checked;
    }
    exportEp(&epNode,fileName,true,true);
    emit exportSingleDown();
}

void PoolWorker::deletePool(const QList<DanmuPoolNode *> &deleteList)
{
     QSqlDatabase db=QSqlDatabase::database("Comment_W");
     QSqlQuery query(db);
     db.transaction();
     for(const DanmuPoolNode *node:deleteList)
     {
         if(node->type==DanmuPoolNode::AnimeNode && node->checkStatus!=Qt::Unchecked)
         {
             for(DanmuPoolNode *epNode:*node->children)
             {
                 switch (epNode->checkStatus)
                 {
                 case Qt::Checked:
                 {
                     emit stateMessage(tr("Deleting: %1 %2").arg(node->title).arg(epNode->title));
                     query.prepare("delete from pool where PoolID=?");
                     query.bindValue(0,epNode->idInfo);
                     query.exec();
                     break;
                 }
                 case Qt::PartiallyChecked:
                 {
                     query.prepare("delete from source where PoolID=? and ID=?");
                     query.bindValue(0,epNode->idInfo);

                     QSqlQuery deleteDMQuery(db);
                     int tableId=DanmuPoolNode::idHash(epNode->idInfo);
                     deleteDMQuery.prepare(QString("delete from danmu_%1 where PoolID=? and Source=?").arg(tableId));
                     deleteDMQuery.bindValue(0,epNode->idInfo);

                     for(DanmuPoolNode *srcNode:*epNode->children)
                     {
                         if(srcNode->checkStatus!=Qt::Checked)continue;
                         emit stateMessage(tr("Deleting: %1 %2 %3").arg(node->title).arg(epNode->title).arg(srcNode->title));
                         query.bindValue(1,static_cast<DanmuPoolSourceNode *>(srcNode)->srcId);
                         query.exec();
                         deleteDMQuery.bindValue(1,static_cast<DanmuPoolSourceNode *>(srcNode)->srcId);
                         deleteDMQuery.exec();
                     }
                     break;
                 }
                 default:
                     break;
                 }
             }
         }
     }
     db.commit();
     emit deleteDone();
     emit stateMessage("Done");
}

void PoolWorker::updatePool(QList<DanmuPoolNode *> &updateList)
{
    for(const DanmuPoolNode *animeNode:updateList)
    {
        if(animeNode->checkStatus==Qt::Unchecked)continue;
        for(DanmuPoolNode *epNode:*animeNode->children)
        {
            if(epNode->checkStatus==Qt::Unchecked)continue;
            QSqlQuery query(QSqlDatabase::database("Comment_W"));
            int tableId=DanmuPoolNode::idHash(epNode->idInfo);
            query.exec(QString("select Time,Color,User,Text from danmu_%1 where PoolID='%2'").arg(tableId).arg(epNode->idInfo));
            int timeNo = query.record().indexOf("Time"),
                colorNo=query.record().indexOf("Color"),
                userNo=query.record().indexOf("User"),
                textNo=query.record().indexOf("Text");
            QSet<QString> danmuHashSet;
            while (query.next())
            {
                QByteArray hashData(QString("%0%1%2%3")
                                    .arg(query.value(textNo).toString())
                                    .arg(query.value(timeNo).toInt())
                                    .arg(query.value(userNo).toString())
                                    .arg(query.value(colorNo).toInt()).toUtf8());
                QString danmuHash(QString(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex()));
                danmuHashSet.insert(danmuHash);
            }
            for(DanmuPoolNode *sourceNode:*epNode->children)
            {
                if(sourceNode->checkStatus==Qt::Unchecked)continue;
                emit stateMessage(tr("Updating: %1 %2 %3").arg(animeNode->title).arg(epNode->title).arg(sourceNode->idInfo));
                updateSource(static_cast<DanmuPoolSourceNode *>(sourceNode),danmuHashSet);
            }
        }

    }
    emit updateDone();
    emit stateMessage("Done");
}

void PoolWorker::saveSource(const QString &pid, const DanmuSourceInfo *sourceInfo, const QList<DanmuComment *> *danmuList)
{
    QSqlDatabase db = QSqlDatabase::database("Comment_W");
    QSqlQuery query(QSqlDatabase::database("Comment_W"));
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

void PoolWorker::exportJson(const QString &pid)
{
    QSqlQuery query(QSqlDatabase::database("Comment_W"));
    query.exec(QString("select ID,Delay,TimeLine from source where PoolID='%1'").arg(pid));
    int idNo = query.record().indexOf("ID"),
        delayNo=query.record().indexOf("Delay"),
        timelineNo=query.record().indexOf("TimeLine");
    QHash<int,DanmuSourceInfo> sourcesTable;
    while (query.next())
    {
        DanmuSourceInfo sourceInfo;
        sourceInfo.delay=query.value(delayNo).toInt();
        sourceInfo.id=query.value(idNo).toInt();
        QStringList timelineList(query.value(timelineNo).toString().split(';',QString::SkipEmptyParts));
        QTextStream ts;
        for(QString &spaceInfo:timelineList)
        {
            ts.setString(&spaceInfo,QIODevice::ReadOnly);
            int start,duration;
            ts>>start>>duration;
            sourceInfo.timelineInfo.append(QPair<int,int>(start,duration));
        }
        sourcesTable.insert(sourceInfo.id,sourceInfo);
    }
    DanmuComment tmpComment;
    QJsonArray danmuArray;
    int tableId=DanmuPoolNode::idHash(pid);
    query.exec(QString("select * from danmu_%1 where PoolID='%2'").arg(tableId).arg(pid));
    int timeNo = query.record().indexOf("Time"),
            colorNo=query.record().indexOf("Color"),
            modeNo=query.record().indexOf("Mode"),
            sourceNo=query.record().indexOf("Source"),
            userNo=query.record().indexOf("User"),
            textNo=query.record().indexOf("Text");
    while (query.next())
    {
        tmpComment.color=query.value(colorNo).toInt();
        tmpComment.sender=query.value(userNo).toString();
        tmpComment.type=DanmuComment::DanmuType(query.value(modeNo).toInt());
        tmpComment.source=query.value(sourceNo).toInt();
        tmpComment.text=query.value(textNo).toString();
        tmpComment.originTime=query.value(timeNo).toInt();
        if(GlobalObjects::blocker->isBlocked(&tmpComment))continue;
        Q_ASSERT(sourcesTable.contains(tmpComment.source));
        GlobalObjects::danmuManager->setDelay(&tmpComment,&sourcesTable[tmpComment.source]);
        QJsonArray danmuObj={tmpComment.time/1000.0,tmpComment.type,tmpComment.color,tmpComment.sender,tmpComment.text};
        danmuArray.append(danmuObj);
    }
    QJsonObject resposeObj
    {
        {"code", 0},
        {"data", danmuArray}
    };
    emit exportJsonDown(resposeObj);
}

void PoolWorker::exportEp(const DanmuPoolNode *epNode, const QString &fileName, bool useTimeline, bool applyBlockRule)
{
    if(epNode->checkStatus==Qt::Unchecked)return;
    QHash<int,DanmuSourceInfo> sourcesTable;
    QStringList srcCondition;
    for(DanmuPoolNode *srcNode:*epNode->children)
    {
        if(srcNode->checkStatus!=Qt::Checked)continue;
        DanmuSourceInfo sourceInfo=static_cast<DanmuPoolSourceNode *>(srcNode)->toSourceInfo();
        sourcesTable.insert(sourceInfo.id,sourceInfo);
        srcCondition<<QString(" Source=%1 ").arg(sourceInfo.id);
    }
    QFile danmuFile(fileName);
    bool ret=danmuFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&danmuFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("i");
    QSqlQuery query(QSqlDatabase::database("Comment_W"));
    int tableId=DanmuPoolNode::idHash(epNode->idInfo);
    query.exec(QString("select * from danmu_%1 where PoolID='%2' and (%3)").arg(tableId).arg(epNode->idInfo).arg(srcCondition.join("or")));
    int timeNo = query.record().indexOf("Time"),
        dateNo=query.record().indexOf("Date"),
        colorNo=query.record().indexOf("Color"),
        modeNo=query.record().indexOf("Mode"),
        sizeNo=query.record().indexOf("Size"),
        sourceNo=query.record().indexOf("Source"),
        userNo=query.record().indexOf("User"),
        textNo=query.record().indexOf("Text");
    DanmuComment tmpComment;
    static int type[3]={1,5,4};
    static int fontSize[3]={25,28,36};
    while (query.next())
    {
        tmpComment.color=query.value(colorNo).toInt();
        tmpComment.date=query.value(dateNo).toLongLong();
        tmpComment.fontSizeLevel=DanmuComment::FontSizeLevel(query.value(sizeNo).toInt());
        tmpComment.sender=query.value(userNo).toString();
        tmpComment.type=DanmuComment::DanmuType(query.value(modeNo).toInt());
        tmpComment.source=query.value(sourceNo).toInt();
        tmpComment.text=query.value(textNo).toString();
        tmpComment.originTime=query.value(timeNo).toInt();
        if(applyBlockRule && GlobalObjects::blocker->isBlocked(&tmpComment)) continue;
        if(useTimeline)
        {
            Q_ASSERT(sourcesTable.contains(tmpComment.source));
            GlobalObjects::danmuManager->setDelay(&tmpComment,&sourcesTable[tmpComment.source]);
        }
        else
        {
            tmpComment.time=tmpComment.originTime;
        }

        writer.writeStartElement("d");
        writer.writeAttribute("p", QString("%0,%1,%2,%3,%4,%5,%6,%7").arg(QString::number(tmpComment.time/1000.0,'f',2))
                              .arg(type[tmpComment.type]).arg(fontSize[tmpComment.fontSizeLevel]).arg(tmpComment.color)
                              .arg(tmpComment.date).arg("0").arg(tmpComment.sender).arg("0"));
        QString danmuText;
        for(const QChar &ch:tmpComment.text)
        {
            if(ch == 0x9 //\t
                    || ch == 0xA //\n
                    || ch == 0xD //\r
                    || (ch >= 0x20 && ch <= 0xD7FF)
                    || (ch >= 0xE000 && ch <= 0xFFFD)
                    )
                danmuText.append(ch);
        }
        writer.writeCharacters(danmuText);
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndDocument();
}

void PoolWorker::updateSource(DanmuPoolSourceNode *sourceNode, const QSet<QString> &danmuHashSet)
{
    QList<DanmuComment *> tmpList;
    QString errInfo = GlobalObjects::providerManager->downloadBySourceURL(sourceNode->idInfo,tmpList);
    if(!errInfo.isEmpty())return;
    if(GlobalObjects::danmuPool->getPoolID()==sourceNode->parent->idInfo)
    {
        emit addToCurPool(sourceNode->idInfo,tmpList);
        return;
    }
    for(auto iter=tmpList.begin();iter!=tmpList.end();)
    {
        QByteArray hashData(QString("%0%1%2%3").arg((*iter)->text).arg((*iter)->originTime).arg((*iter)->sender).arg((*iter)->color).toUtf8());
        QString danmuHash(QString(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex()));
        if(danmuHashSet.contains(danmuHash) || (*iter)->text.isEmpty())
        {
            delete *iter;
            iter=tmpList.erase(iter);
        }
        else
        {
            (*iter)->source=sourceNode->srcId;
            ++iter;
        }
    }
    if(tmpList.count()>0)
    {
        sourceNode->danmuCount+=tmpList.count();
        saveSource(sourceNode->parent->idInfo,nullptr,&tmpList);
        qDeleteAll(tmpList);
    }

}


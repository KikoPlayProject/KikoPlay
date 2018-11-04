#include "danmupool.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QSqlQuery>
#include <QSqlRecord>

#include "danmurender.h"
#include "globalobjects.h"
#include "blocker.h"
#include "Play/Playlist/playlist.h"
namespace
{
    struct
    {
        inline bool operator ()(const QSharedPointer<DanmuComment> &danmu,int time) const
        {
            return danmu->time<time;
        }
    } DanmuComparer;
    struct
    {
        inline bool operator()(const QSharedPointer<DanmuComment> &dm1,const QSharedPointer<DanmuComment> &dm2) const
        {
            return dm1->time<dm2->time;
        }
    } DanmuSPCompare;
}
DanmuPool::DanmuPool(QObject *parent) : QAbstractItemModel(parent),currentPosition(0),currentTime(0)
{

}

void DanmuPool::addDanmu(DanmuSourceInfo &sourceInfo,QList<DanmuComment *> &danmuList)
{
    DanmuSourceInfo *source(nullptr);
    bool containSource=false;
    int maxId=0;
    for(auto iter=sourcesTable.begin();iter!=sourcesTable.end();++iter)
    {
        if(iter.value().url==sourceInfo.url)
        {
            source=&iter.value();
            source->count+=sourceInfo.count;
            containSource=true;
            break;
        }
		if (iter.value().id >= maxId)maxId = iter.value().id + 1;
    }
    if(!source)
    {
        DanmuSourceInfo newSource;
        newSource.id=maxId;
        newSource.delay=sourceInfo.delay;
        newSource.count=sourceInfo.count;
        newSource.url=sourceInfo.url;
        newSource.name=sourceInfo.name;
        newSource.show=true;
        sourcesTable.insert(maxId,newSource);
		source = &sourcesTable[maxId];
    }

    for(DanmuComment *danmu:danmuList)
    {
        int delay=0;
        if(containSource)
        { 
            for(auto &spaceItem: source->timelineInfo)
            {
                if(danmu->originTime>spaceItem.first)delay+=spaceItem.second;
                else break;
            }
        }
        int newTime = danmu->originTime + source->delay + delay;
        danmu->time = newTime>0?newTime:danmu->originTime;
        danmu->source=source->id;
        danmuPool.append(QSharedPointer<DanmuComment>(danmu));
    }
    GlobalObjects::blocker->checkDanmu(danmuList);
    saveDanmu(containSource?nullptr:source,&danmuList);
    beginResetModel();
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
	currentPosition = std::lower_bound(danmuPool.begin(), danmuPool.end(), currentTime, DanmuComparer) - danmuPool.begin();
    setStatisInfo();
}

void DanmuPool::deleteSource(int sourceIndex)
{
    if(!sourcesTable.contains(sourceIndex))return;
    sourcesTable.remove(sourceIndex);
    QCoreApplication::processEvents();
    beginResetModel();
    for(auto iter=danmuPool.begin();iter!=danmuPool.end();)
    {
        if((*iter)->source==sourceIndex)
        {
            //delete *iter;
            iter=danmuPool.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    endResetModel();
    mediaTimeJumped(currentTime);
    if(!poolID.isEmpty())
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.exec(QString("delete from danmu where PoolID='%1' and Source=%2").arg(poolID).arg(sourceIndex));
        query.exec(QString("delete from source where PoolID='%1' and ID=%2").arg(poolID).arg(sourceIndex));
    }
    setStatisInfo();
}

void DanmuPool::loadDanmuFromDB()
{
    if(poolID.isEmpty())return;
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.exec(QString("select * from source where PoolID='%1'").arg(poolID));
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
    query.exec(QString("select * from danmu where PoolID='%1'").arg(poolID));
    int timeNo = query.record().indexOf("Time"),
        dateNo=query.record().indexOf("Date"),
        colorNo=query.record().indexOf("Color"),
        modeNo=query.record().indexOf("Mode"),
        sizeNo=query.record().indexOf("Size"),
        sourceNo=query.record().indexOf("Source"),
        userNo=query.record().indexOf("User"),
        textNo=query.record().indexOf("Text");
    beginResetModel();
    //qDeleteAll(danmuPool);
    danmuPool.clear();
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
        int delay=0;
        if(sourcesTable.contains(danmu->source))
        {
            for(auto &spaceItem:sourcesTable[danmu->source].timelineInfo)
            {
                if(danmu->originTime>spaceItem.first)delay+=spaceItem.second;
                else break;
            }
            delay+=sourcesTable[danmu->source].delay;
            sourcesTable[danmu->source].count++;
        }
        danmu->time=danmu->originTime+delay<0?danmu->originTime:danmu->originTime+delay;
        danmuPool.append(QSharedPointer<DanmuComment>(danmu));
    }
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
    GlobalObjects::blocker->checkDanmu(danmuPool);
    setStatisInfo();
}

void DanmuPool::cleanUp()
{
	sourcesTable.clear();
	beginResetModel();
	danmuPool.clear();
	endResetModel();
    poolID=QString();
	reset();
    setStatisInfo();
}

void DanmuPool::testBlockRule(BlockRule *rule)
{
    for(QSharedPointer<DanmuComment> &danmu:danmuPool)
    {
        if(danmu->blockBy==-1)
        {
            if(rule->blockTest(danmu.data()))
                danmu->blockBy=rule->id;
        }
        else if(danmu->blockBy==rule->id)
        {
            if(!rule->blockTest(danmu.data()))
                danmu->blockBy=-1;
        }
    }
    GlobalObjects::danmuRender->removeBlocked();
}

void DanmuPool::deleteDanmu(QSharedPointer<DanmuComment> &danmu)
{
    if(!poolID.isEmpty())
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.exec(QString("delete from danmu where PoolID='%1' and Date=%2 and User='%3' and Text='%4' and Source=%5")
                    .arg(poolID).arg(danmu->date).arg(danmu->sender).arg(danmu->text).arg(danmu->source));
    }
    sourcesTable[danmu->source].count--;
	int row = danmuPool.indexOf(danmu);
	beginRemoveRows(QModelIndex(), row, row);
	danmuPool.removeOne(danmu);
    endRemoveRows();
    setStatisInfo();
}

QSet<QString> DanmuPool::getDanmuHash(int sourceId)
{
    QSet<QString> hashSet;
    for(QSharedPointer<DanmuComment> &danmu:danmuPool)
    {
        if(danmu->source==sourceId)
        {
            QByteArray hashData(QString("%0%1%2%3").arg(danmu->text).arg(danmu->originTime).arg(danmu->sender).arg(danmu->color).toUtf8());
            QString danmuHash(QString(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex()));
            hashSet.insert(danmuHash);
        }
    }
    return hashSet;
}

QList<SimpleDanmuInfo> DanmuPool::getSimpleDanmuInfo(int sourceId)
{
    QList<SimpleDanmuInfo> simpleDanmuList;
    for(QSharedPointer<DanmuComment> &danmu:danmuPool)
    {
        if(danmu->source==sourceId)
        {
            SimpleDanmuInfo sdi;
            sdi.originTime=danmu->originTime;
            sdi.time=sdi.originTime;
            sdi.text=danmu->text;
            simpleDanmuList.append(sdi);
        }
    }
    return simpleDanmuList;
}

void DanmuPool::exportDanmu(int sourceId, const QString &fileName)
{
    static int type[3]={1,5,4};
    static int fontSize[3]={25,28,36};
    QFile danmuFile(fileName);
    bool ret=danmuFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&danmuFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("i");
    for(QSharedPointer<DanmuComment> &danmu:danmuPool)
    {
        if(sourceId==-1 || danmu->source==sourceId)
        {
            writer.writeStartElement("d");
            writer.writeAttribute("p", QString("%0,%1,%2,%3,%4,%5,%6,%7").arg(QString::number(danmu->time/1000.f,'f',2))
                                  .arg(type[danmu->type]).arg(fontSize[danmu->fontSizeLevel]).arg(danmu->color)
                                  .arg(danmu->date).arg("0").arg(danmu->sender).arg("0"));
            QString danmuText;
            for(const QChar &ch:danmu->text)
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
    }
    writer.writeEndElement();
    writer.writeEndDocument();
    danmuFile.close();
}

void DanmuPool::saveDanmu(const DanmuSourceInfo *sourceInfo, const QList<DanmuComment *> *danmuList)
{
    if(poolID.isEmpty())return;
    QSqlDatabase db=QSqlDatabase::database("MT");
    db.transaction();
    if(sourceInfo)
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.prepare("insert into source(PoolID,ID,Name,Delay,URL,TimeLine) values(?,?,?,?,?,?)");
        query.bindValue(0,poolID);
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

        QSqlQuery query(QSqlDatabase::database("MT"));
        query.prepare("insert into danmu(PoolID,Time,Date,Color,Mode,Size,Source,User,Text) values(?,?,?,?,?,?,?,?,?)");
        for(DanmuComment *danmu:*danmuList)
        {
            query.bindValue(0,poolID);
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

void DanmuPool::setStatisInfo()
{
    statisInfo.countOfMinute.clear();
    statisInfo.maxCountOfMinute=0;
    int curMinuteCount=0;
    int startTime=0;
    for(auto iter=danmuPool.cbegin();iter!=danmuPool.cend();++iter)
    {
        if(iter==danmuPool.cbegin())
        {
            startTime=(*iter)->time;
        }
        if((*iter)->time-startTime<1000)
            curMinuteCount++;
        else
        {
            statisInfo.countOfMinute.append(QPair<int,int>(startTime/1000,curMinuteCount));
            if(curMinuteCount>statisInfo.maxCountOfMinute)
                statisInfo.maxCountOfMinute=curMinuteCount;
            curMinuteCount=1;
            startTime=(*iter)->time;
        }
    }
	statisInfo.countOfMinute.append(QPair<int, int>(startTime / 1000, curMinuteCount));
	if (curMinuteCount>statisInfo.maxCountOfMinute)
		statisInfo.maxCountOfMinute = curMinuteCount;
    emit statisInfoChange();

}

void DanmuPool::setDelay(DanmuSourceInfo *sourceInfo,int newDelay)
{
    if(sourceInfo->delay==newDelay)return;
    for(auto iter=danmuPool.begin();iter!=danmuPool.end();++iter)
    {
        QSharedPointer<DanmuComment> cur = *iter;
		if (cur->source == sourceInfo->id)
		{
            int delay=newDelay;
            for(auto &spaceItem:sourceInfo->timelineInfo)
            {
                if(cur->originTime>spaceItem.first)delay+=spaceItem.second;
                else break;
            }
            int newTime = cur->originTime + delay;
            cur->time=newTime>0?newTime:cur->originTime;
		}
    }
    sourceInfo->delay=newDelay;
    beginResetModel();
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
    currentPosition = std::lower_bound(danmuPool.begin(), danmuPool.end(), currentTime, DanmuComparer) - danmuPool.begin();
    if(!poolID.isEmpty())
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.exec(QString("update source set Delay= %1 where PoolID='%2' and ID=%3").arg(newDelay).arg(poolID).arg(sourceInfo->id));
    }
    setStatisInfo();
}

void DanmuPool::refreshTimeLineDelayInfo(DanmuSourceInfo *sourceInfo)
{
    for(auto iter=danmuPool.begin();iter!=danmuPool.end();++iter)
    {
        QSharedPointer<DanmuComment> cur = *iter;
        if (cur->source == sourceInfo->id)
        {
            int delay=0;
            for(auto &spaceItem:sourceInfo->timelineInfo)
            {
                if(cur->originTime>spaceItem.first)delay+=spaceItem.second;
                else break;
            }
            int newTime = cur->originTime + delay + sourceInfo->delay;
            cur->time=newTime>0?newTime:cur->originTime;
        }
    }
    beginResetModel();
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
    currentPosition = std::lower_bound(danmuPool.begin(), danmuPool.end(), currentTime, DanmuComparer) - danmuPool.begin();
    if(!poolID.isEmpty())
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.prepare("update source set TimeLine= ? where PoolID=? and ID=?");
        QString timelineInfo;
        QTextStream ts(&timelineInfo);
        for(auto &spaceItem:sourceInfo->timelineInfo)
        {
            ts<<spaceItem.first<<' '<<spaceItem.second<<';';
        }
        ts.flush();
        query.bindValue(0,timelineInfo);
        query.bindValue(1,poolID);
        query.bindValue(2,sourceInfo->id);
        query.exec();
    }
    setStatisInfo();
}

void DanmuPool::refreshCurrentPoolID()
{
	const PlayListItem *currentItem = GlobalObjects::playlist->getCurrentItem();
	if (currentItem && currentItem->poolID != poolID)
	{
		if (!poolID.isEmpty())
			cleanUp();		
		poolID = currentItem->poolID;
	}
}

void DanmuPool::mediaTimeElapsed(int newTime)
{
    if(currentTime>newTime)
    {
        QCoreApplication::instance()->processEvents();
        currentTime=newTime;
        return;
    }
    currentTime=newTime;
    PrepareList *prepareList(nullptr);
    if(!prepareListPool.isEmpty())
    {
        prepareList= prepareListPool.takeFirst();
    }
    else
    {
        prepareList=new PrepareList;
    }
    for(;currentPosition<danmuPool.length();++currentPosition)
    {
        int curTime=danmuPool.at(currentPosition)->time;
        if(curTime<0)continue;
        if(curTime<newTime)
        {
            auto dm=danmuPool.at(currentPosition);
            if (dm->blockBy == -1 && sourcesTable[dm->source].show)
			{
                prepareList->append(QPair<QSharedPointer<DanmuComment>,DanmuDrawInfo*>(dm,nullptr));
			}
        }
        else
            break;
    }
    if(prepareList->length()>0)
    {
        GlobalObjects::danmuRender->prepareDanmu(prepareList);
    }
	else
	{
		recyclePrepareList(prepareList);
	}
}

void DanmuPool::mediaTimeJumped(int newTime)
{
#ifdef QT_DEBUG
    qDebug()<<"pool:media time jumped,newTime:"<<newTime<<",currentTime:"<<currentTime<<",currentPos"<<currentPosition;
#endif
    currentTime=newTime;
    currentPosition=std::lower_bound(danmuPool.begin(),danmuPool.end(),newTime,DanmuComparer)-danmuPool.begin();
    GlobalObjects::danmuRender->cleanup();
#ifdef QT_DEBUG
    qDebug()<<"pool:media time jumped,currentPos"<<currentPosition;
#endif
}

QVariant DanmuPool::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    auto comment=danmuPool[index.row()];
    int col=index.column();
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col==0)
        {
            int sec_total=comment->time/1000;
            int min=sec_total/60;
            int sec=sec_total-min*60;
            return QString("%1:%2").arg(min,2,10,QChar('0')).arg(sec,2,10,QChar('0'));
        }
        else if(col==1)
        {
            return comment->text;
        }
        break;
    }
    case Qt::ForegroundRole:
    {
        if(comment->blockBy!=-1)
            return QBrush(QColor(150,150,150));
        return QBrush(QColor(comment->color>>16,(comment->color>>8)&0xff,comment->color&0xff,200));
    }
    case Qt::ToolTipRole:
    {
        static QString types[3]={tr("Roll"),tr("Top"),tr("Bottom")};
        return tr("User: %1\nTime: %2\nText: %3\nType: %4%5").arg(comment->sender).
                arg(QDateTime::fromSecsSinceEpoch(comment->date).toString("yyyy-MM-dd hh:mm:ss"))
                .arg(comment->text).arg(types[comment->type])
                .arg(comment->blockBy==-1?"":tr("\nBlock By Rule:%1").arg(comment->blockBy));
    }
	case Qt::FontRole:
		if (comment->blockBy != -1 && col==1)
			return QFont("Microsoft YaHei UI", 11, -1, true);
    default:
        return QVariant();
    }
	return QVariant();
}

QVariant DanmuPool::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QString headers[]={tr("Time"),tr("Content")};
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<2)return headers[section];
    }
    return QVariant();
}

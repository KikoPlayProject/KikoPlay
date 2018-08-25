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
    for(auto iter=sourcesTable.begin();iter!=sourcesTable.end();iter++)
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
        newSource.open=true;
        sourcesTable.insert(maxId,newSource);
		source = &sourcesTable[maxId];
    }

    for(DanmuComment *danmu:danmuList)
    {
        if(containSource)
        {
            int newTime = danmu->time + source->delay;
            if (newTime > 0)danmu->time = newTime;
        }
        danmu->source=source->id;
        danmuPool.append(QSharedPointer<DanmuComment>(danmu));
    }
    GlobalObjects::blocker->checkDanmu(danmuList);
    saveDanmu(containSource?nullptr:source,&danmuList);
    beginResetModel();
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
    mediaTimeJumped(currentTime);
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
            iter++;
        }
    }
    endResetModel();
    mediaTimeJumped(currentTime);
    if(poolID.isEmpty())return;
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.exec(QString("delete from danmu where PoolID='%1' and Source=%2").arg(poolID).arg(sourceIndex));
    query.exec(QString("delete from source where PoolID='%1' and ID=%2").arg(poolID).arg(sourceIndex));
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
        urlNo=query.record().indexOf("URL");
    sourcesTable.clear();
    while (query.next())
    {
        DanmuSourceInfo sourceInfo;
        sourceInfo.delay=query.value(delayNo).toInt();
        sourceInfo.id=query.value(idNo).toInt();
        sourceInfo.name=query.value(nameNo).toString();
        sourceInfo.url=query.value(urlNo).toString();
        sourceInfo.open=true;
        sourceInfo.count=0;
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
        //danmu->blocked=false;
        danmu->blockBy=-1;
        danmu->color=query.value(colorNo).toInt();
        danmu->date=query.value(dateNo).toLongLong();
        danmu->fontSizeLevel=DanmuComment::FontSizeLevel(query.value(sizeNo).toInt());
        danmu->sender=query.value(userNo).toString();
        danmu->type=DanmuComment::DanmuType(query.value(modeNo).toInt());
        danmu->source=query.value(sourceNo).toInt();
        danmu->text=query.value(textNo).toString();
        int delay=0;
        if(sourcesTable.contains(danmu->source))
        {
            delay=sourcesTable[danmu->source].delay;
            sourcesTable[danmu->source].count++;
        }
		int time = query.value(timeNo).toInt();
        danmu->time=time+delay<0?time:time+delay;
        danmuPool.append(QSharedPointer<DanmuComment>(danmu));
    }
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
    GlobalObjects::blocker->checkDanmu(danmuPool);
    setStatisInfo();
}

void DanmuPool::reset()
{
    currentTime=0;
    currentPosition=0;
}

void DanmuPool::cleanUp()
{
	sourcesTable.clear();
	beginResetModel();
	danmuPool.clear();
	endResetModel();
	reset();
    setStatisInfo();
}

QModelIndex DanmuPool::getCurrentIndex()
{
	if (currentPosition >= 0 && currentPosition < danmuPool.count())
		return createIndex(currentPosition, 0);
    return QModelIndex();
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
            QByteArray hashData(QString("%0%1%2%3").arg(danmu->text).arg(danmu->date).arg(danmu->sender).arg(danmu->color).toUtf8());
            QString danmuHash(QString(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex()));
            hashSet.insert(danmuHash);
        }
    }
    return hashSet;
}

void DanmuPool::exportDanmu(int sourceId, QString &fileName)
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
            writer.writeAttribute("p", QString("%0,%1,%2,%3,%4,%5,%6,%7").arg(QString::number(danmu->time/60000.f,'f',2))
                                  .arg(type[danmu->type]).arg(fontSize[danmu->fontSizeLevel]).arg(danmu->color)
                                  .arg(danmu->date).arg("0").arg(danmu->sender).arg("0"));
            writer.writeCharacters(danmu->text);
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
    if(sourceInfo)
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.prepare("insert into source(PoolID,ID,Name,Delay,URL) values(?,?,?,?,?)");
        query.bindValue(0,poolID);
        query.bindValue(1,sourceInfo->id);
        query.bindValue(2,sourceInfo->name);
        query.bindValue(3,sourceInfo->delay);
        query.bindValue(4,sourceInfo->url);
        query.exec();
    }
    if(danmuList)
    {
        QSqlDatabase db=QSqlDatabase::database("MT");
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.prepare("insert into danmu(PoolID,Time,Date,Color,Mode,Size,Source,User,Text) values(?,?,?,?,?,?,?,?,?)");
        db.transaction();
        for(DanmuComment *danmu:*danmuList)
        {
            query.bindValue(0,poolID);
            query.bindValue(1,danmu->time);
            query.bindValue(2,danmu->date);
            query.bindValue(3,danmu->color);
            query.bindValue(4,(int)danmu->type);
            query.bindValue(5,(int)danmu->fontSizeLevel);
            query.bindValue(6,danmu->source);
            query.bindValue(7,danmu->sender);
            query.bindValue(8,danmu->text);
            query.exec();
        }
        db.commit();
    }
}

void DanmuPool::setStatisInfo()
{
    statisInfo.countOfMinute.clear();
    statisInfo.maxCountOfMinute=0;
    int curMinuteCount=0;
    int startTime=0;
    for(auto iter=danmuPool.cbegin();iter!=danmuPool.cend();iter++)
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
    for(auto iter=danmuPool.begin();iter!=danmuPool.end();iter++)
    {
        QSharedPointer<DanmuComment> cur = *iter;
		if (cur->source == sourceInfo->id)
		{
			int newTime = cur->time + newDelay - sourceInfo->delay;
			if (newTime > 0)cur->time = newTime;
		}
    }
    sourceInfo->delay=newDelay;
    beginResetModel();
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
    mediaTimeJumped(currentTime);
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.exec(QString("update source set Delay= %1 where PoolID='%2' and ID=%3").arg(newDelay).arg(poolID).arg(sourceInfo->id));
    setStatisInfo();
}

void DanmuPool::setPoolID(QString pid)
{
    poolID=pid;
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
#ifdef QT_DEBUG
       // qDebug()<<"pool:media time elapsed,currentTime>newTime,newTime:"<<newTime<<",currentTime"<<currentTime<<",currentPos"<<currentPosition;
#endif
        QCoreApplication::instance()->processEvents();
        currentTime=newTime;
        return;
    }
#ifdef QT_DEBUG
    //qDebug()<<"pool:media time elapsed,newTime:"<<newTime<<",currentTime:"<<currentTime<<",currentPos"<<currentPosition;
#endif
//    if(currentTime+1000<newTime)
//    {
//#ifdef QT_DEBUG
//       // qDebug()<<"pool:media time elapsed,currentTime+1000<newTime,newTime:"<<newTime<<",currentTime"<<currentTime<<",currentPos"<<currentPosition;
//#endif
//        QCoreApplication::instance()->processEvents();
//        currentTime=newTime;
//        return;
//    }
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
    QList<QSharedPointer<DanmuComment> > *newDanmu=new QList<QSharedPointer<DanmuComment> >();//*cachedDanmu=new QList<DanmuComment *>();
    for(;currentPosition<danmuPool.length();currentPosition++)
    {
        int curTime=danmuPool.at(currentPosition)->time;
        if(curTime<0)continue;
        if(curTime<newTime)
        {
            auto dm=danmuPool.at(currentPosition);
			if (dm->blockBy == -1 && sourcesTable[dm->source].open)
			{
                prepareList->append(QPair<QSharedPointer<DanmuComment>,DanmuDrawInfo*>(dm,nullptr));
				newDanmu->append(dm);
			}
        }
        else
            break;
    }
    if(newDanmu->length()>0)
    {
        GlobalObjects::danmuRender->prepareDanmu(prepareList);
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

QModelIndex DanmuPool::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()){
        return createIndex(row, column);
    }
    return QModelIndex();
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
    }
    case Qt::ForegroundRole:
    {
        if(comment->blockBy!=-1)
            return QBrush(QColor(150,150,150));
        return QBrush(QColor(comment->color>>16,(comment->color>>8)&0xff,comment->color&0xff,200));
        /*switch (comment->type) {
        case DanmuComment::Rolling:
            return QBrush(QColor(200,200,200));
        case DanmuComment::Top:
            return QBrush(QColor(0,177,211));
        case DanmuComment::Bottom:
            return QBrush(QColor(84,195,23));
        default:
            break;
        }*/
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

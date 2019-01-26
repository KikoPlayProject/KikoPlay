#include "danmupool.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QSqlQuery>
#include <QSqlRecord>

#include "Render/danmurender.h"
#include "globalobjects.h"
#include "blocker.h"
#include "Manager/danmumanager.h"
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

int DanmuPool::addDanmu(DanmuSourceInfo &sourceInfo,QList<DanmuComment *> &danmuList)
{
    DanmuSourceInfo *source(nullptr);
    bool containSource=false;
    int maxId=0;
    for(auto iter=sourcesTable.begin();iter!=sourcesTable.end();++iter)
    {
        if(iter.value().url==sourceInfo.url)
        {
            source=&iter.value();
            containSource=true;
            break;
        }
		if (iter.value().id >= maxId)maxId = iter.value().id + 1;
    }
    if(source)
    {
        QSet<QString> danmuHashSet(getDanmuHash(source->id));
        for(auto iter=danmuList.begin();iter!=danmuList.end();)
        {
            QByteArray hashData(QString("%0%1%2%3").arg((*iter)->text).arg((*iter)->originTime).arg((*iter)->sender).arg((*iter)->color).toUtf8());
            QString danmuHash(QString(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex()));
            if(danmuHashSet.contains(danmuHash))
            {
                delete *iter;
                iter=danmuList.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        if(danmuList.count()==0) return 0;
        source->count += danmuList.count();
    }
    else
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
        GlobalObjects::danmuManager->setDelay(danmu,source);
        danmu->source=source->id;
        danmuPool.append(QSharedPointer<DanmuComment>(danmu));
    }
    GlobalObjects::blocker->checkDanmu(danmuList);
    if(!poolID.isEmpty())
    {
        GlobalObjects::danmuManager->saveSource(poolID,containSource?nullptr:source,&danmuList);
    }
    beginResetModel();
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
	currentPosition = std::lower_bound(danmuPool.begin(), danmuPool.end(), currentTime, DanmuComparer) - danmuPool.begin();
    setStatisInfo();
    return danmuList.count();
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
        GlobalObjects::danmuManager->deleteSource(poolID,sourceIndex);
    }
    setStatisInfo();
}

void DanmuPool::loadDanmuFromDB()
{
    if(poolID.isEmpty())return;
    beginResetModel();
    GlobalObjects::danmuManager->loadPool(poolID,danmuPool,sourcesTable);
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
        GlobalObjects::danmuManager->deleteDanmu(poolID,danmu.data());
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
    sourceInfo->delay=newDelay;
    for(auto iter=danmuPool.begin();iter!=danmuPool.end();++iter)
    {
        QSharedPointer<DanmuComment> cur = *iter;
		if (cur->source == sourceInfo->id)
		{
            GlobalObjects::danmuManager->setDelay(cur.data(),sourceInfo);
		}
    }
    beginResetModel();
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
    currentPosition = std::lower_bound(danmuPool.begin(), danmuPool.end(), currentTime, DanmuComparer) - danmuPool.begin();
    if(!poolID.isEmpty())
    {
        GlobalObjects::danmuManager->updateSourceDelay(poolID,sourceInfo);
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
            GlobalObjects::danmuManager->setDelay(cur.data(),sourceInfo);
        }
    }
    beginResetModel();
    std::sort(danmuPool.begin(),danmuPool.end(),DanmuSPCompare);
    endResetModel();
    currentPosition = std::lower_bound(danmuPool.begin(), danmuPool.end(), currentTime, DanmuComparer) - danmuPool.begin();
    if(!poolID.isEmpty())
    {
        GlobalObjects::danmuManager->updateSourceTimeline(poolID,sourceInfo);
    }
    setStatisInfo();
}

void DanmuPool::setPoolID(const QString &pid)
{
    if(pid.isEmpty())
    {
        cleanUp();
    }
    else
    {
        if(pid!=poolID)
        {
			cleanUp();
            poolID=pid;
            loadDanmuFromDB();
        }
    }

}

void DanmuPool::refreshCurrentPoolID()
{
	const PlayListItem *currentItem = GlobalObjects::playlist->getCurrentItem();
	if (currentItem && currentItem->poolID != poolID)
	{
        setPoolID(currentItem->poolID);
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
    prepareList=prepareListPool.isEmpty()?new PrepareList:prepareListPool.takeFirst();
    const int bundleSize=20;
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
                if(prepareList->size()>=bundleSize)
                {
                    GlobalObjects::danmuRender->prepareDanmu(prepareList);
                    prepareList=prepareListPool.isEmpty()?new PrepareList :prepareListPool.takeFirst();
                }
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
    {
		if (comment->blockBy != -1 && col==1)
			return QFont("Microsoft YaHei UI", 11, -1, true);
        break;
    }
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

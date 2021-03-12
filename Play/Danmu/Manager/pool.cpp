#include "pool.h"
#include "danmumanager.h"
#include "Script/scriptmanager.h"
#include "globalobjects.h"
#include "../blocker.h"
#include "Common/network.h"
namespace
{
    struct
    {
        inline bool operator()(const QSharedPointer<DanmuComment> &dm1,const QSharedPointer<DanmuComment> &dm2) const
        {
            return dm1->time<dm2->time;
        }
    } DanmuSPCompare;
}

Pool::Pool(const QString &id, const QString &animeTitle, const QString &epTitle, EpType type, double index, QObject *parent):
     QObject(parent),pid(id),anime(animeTitle),ep(epTitle),epType(type), epIndex(index), used(false),isLoaded(false)
{

}

bool Pool::load()
{
    if(!isLoaded && !pid.isEmpty())
    {
        PoolStateLock locker;
        if(!locker.tryLock(pid)) return false;
        GlobalObjects::danmuManager->loadPool(this);
        GlobalObjects::blocker->checkDanmu(commentList);
        isLoaded=true;
        return true;
    }
    GlobalObjects::blocker->checkDanmu(commentList);
    return false;
}

bool Pool::clean()
{
    PoolStateLock locker;
    if(!locker.tryLock(pid)) return false;
    QList<QSharedPointer<DanmuComment> > emptyList;
    commentList.swap(emptyList);
    isLoaded=false;
    return true;
}

int Pool::update(int sourceId, QList<QSharedPointer<DanmuComment> > *incList)
{
    if(sourcesTable.isEmpty()) return 0;
    if(sourceId!=-1 && !sourcesTable.contains(sourceId)) return 0;
    PoolStateLock locker;
    if(!locker.tryLock(pid)) return 0;
    QList<DanmuComment *> tList;
    GlobalObjects::danmuManager->updatePool(this,tList,sourceId);
    QList<QSharedPointer<DanmuComment> > spList;
    if(sourceId!=-1)
    {
        sourcesTable[sourceId].count+=tList.count();
        for(auto comment:tList)
        {
            QSharedPointer<DanmuComment> sp(comment);
            commentList.append(sp);
            spList.append(sp);
            setDelay(comment);
        }
    }
    else
    {
        for(auto &comment:tList)
        {
            sourcesTable[comment->source].count++;
            QSharedPointer<DanmuComment> sp(comment);
            commentList.append(sp);
            spList.append(sp);
            setDelay(comment);
        }
    }
    GlobalObjects::blocker->checkDanmu(tList);
    if(incList!=nullptr) *incList=spList;
    if(!pid.isEmpty()) GlobalObjects::danmuManager->saveSource(pid,nullptr,spList);
    if(tList.count()>0 && used)
    {
        std::sort(commentList.begin(),commentList.end(),DanmuSPCompare);
        emit poolChanged(true);
    }
    return tList.count();
}

int Pool::addSource(const DanmuSource &sourceInfo, QList<DanmuComment *> &danmuList, bool reset)
{
    PoolStateLock locker;
    if(!locker.tryLock(pid)) return -1;
    DanmuSource *source(nullptr);
    bool containSource=false;
    for(auto iter=sourcesTable.begin();iter!=sourcesTable.end();++iter)
    {
        if(iter->scriptId == sourceInfo.scriptId && iter.value().scriptData==sourceInfo.scriptData)
        {
            source=&iter.value();
            containSource=true;
            break;
        }
    }
    if(source)
    {
        QSet<QString> &&danmuHashSet=getDanmuHashSet(source->id);
        for(auto iter=danmuList.begin();iter!=danmuList.end();)
        {
            QByteArray hashData(QString("%0%1%2%3").arg((*iter)->text, QString::number((*iter)->originTime), (*iter)->sender, QString::number((*iter)->color)).toUtf8());
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
        if(danmuList.count()==0)
        {
            return 0;
        }
        source->count += danmuList.count();
    }
    else
    {
        DanmuSource newSource(sourceInfo);
        newSource.id=sourcesTable.isEmpty()?0:sourcesTable.lastKey()+1;
        newSource.count=danmuList.count();
        sourcesTable.insert(newSource.id,newSource);
        source = &sourcesTable[newSource.id];
    }
    GlobalObjects::blocker->checkDanmu(danmuList);
    QList<QSharedPointer<DanmuComment> > tmpList;
    for(DanmuComment *danmu:danmuList)
    {
        danmu->source=source->id;
		setDelay(danmu);
        QSharedPointer<DanmuComment> sp(danmu);
        commentList.append(sp);
        tmpList.append(sp);
    }
    if(!pid.isEmpty())GlobalObjects::danmuManager->saveSource(pid,containSource?nullptr:source,tmpList);
    if(reset && used)
    {
        std::sort(commentList.begin(),commentList.end(),DanmuSPCompare);
        emit poolChanged(true);
    }
    return source->id;
}

bool Pool::deleteSource(int sourceId, bool applyDB)
{
    if(!sourcesTable.contains(sourceId)) return false;
    PoolStateLock locker;
    if(!locker.tryLock(pid)) return false;
    sourcesTable.remove(sourceId);
    for(auto iter=commentList.begin();iter!=commentList.end();)
    {
        if((*iter)->source==sourceId)
            iter=commentList.erase(iter);
        else
            ++iter;
    }
    if(!pid.isEmpty() && applyDB) GlobalObjects::danmuManager->deleteSource(pid,sourceId);
    if(used)
    {
        emit poolChanged(true);
    }
    return true;
}

bool Pool::deleteDanmu(int pos)
{
    if(pos>=0 && pos<commentList.size())
    {
        PoolStateLock locker;
        if(!locker.tryLock(pid)) return false;
        sourcesTable[commentList.at(pos)->source].count--;
        if(!pid.isEmpty())GlobalObjects::danmuManager->deleteDanmu(pid, commentList.at(pos));
        commentList.removeAt(pos);
        return true;
    }
    return false;
}

bool Pool::setTimeline(int sourceId, const QList<QPair<int, int>> &timelineInfo)
{
    if(!sourcesTable.contains(sourceId)) return false;
    PoolStateLock locker;
    if(!locker.tryLock(pid)) return false;
    DanmuSource *srcInfo=&sourcesTable[sourceId];
    srcInfo->timelineInfo=timelineInfo;
    for(auto iter=commentList.cbegin();iter!=commentList.cend();++iter)
    {
        DanmuComment *cur = (*iter).data();
        if (cur->source == sourceId) setDelay(cur);
    }
    if(!pid.isEmpty()) GlobalObjects::danmuManager->updateSourceTimeline(pid,srcInfo);
    if(used)
    {
        std::sort(commentList.begin(),commentList.end(),DanmuSPCompare);
        emit poolChanged(false);
    }
    return true;
}

bool Pool::setDelay(int sourceId, int delay)
{
    if(!sourcesTable.contains(sourceId)) return false;
    DanmuSource *srcInfo=&sourcesTable[sourceId];
    if(srcInfo->delay==delay)return true;
    PoolStateLock locker;
    if(!locker.tryLock(pid)) return false;
    srcInfo->delay=delay;
    for(auto iter=commentList.cbegin();iter!=commentList.cend();++iter)
    {
        DanmuComment *cur = (*iter).data();
        if (cur->source == sourceId) setDelay(cur);
    }
    if(!pid.isEmpty()) GlobalObjects::danmuManager->updateSourceDelay(pid,srcInfo);
    if(used)
    {
        std::sort(commentList.begin(),commentList.end(),DanmuSPCompare);
        emit poolChanged(false);
    }
    return true;
}

void Pool::setUsed(bool on)
{
    used=on;
    if(used) std::sort(commentList.begin(),commentList.end(),DanmuSPCompare);
}

void Pool::setSourceVisibility(int srcId, bool show)
{
    if(!sourcesTable.contains(srcId)) return;
    sourcesTable[srcId].show=show;
}

void Pool::exportPool(const QString &fileName, bool useTimeline, bool applyBlockRule, const QList<int> &ids)
{
    QFile danmuFile(fileName);
    bool ret=danmuFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&danmuFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("i");
    static int type[3]={1,5,4};
    static int fontSize[3]={25,18,36};
    for(const auto &danmu:commentList)
    {
        if(applyBlockRule && danmu->blockBy!=-1) continue;
        if(!ids.isEmpty() && !ids.contains(danmu->source)) continue;
        writer.writeStartElement("d");
        writer.writeAttribute("p", QString("%0,%1,%2,%3,%4,0,%5,0").arg(QString::number((useTimeline?danmu->time:danmu->originTime)/1000.0,'f',2))
                              .arg(type[danmu->type]).arg(fontSize[danmu->fontSizeLevel]).arg(danmu->color)
                              .arg(danmu->date).arg(danmu->sender));
        QString danmuText;
        for(const QChar &ch:danmu->text)
        {
            if((ch>=0x0 && ch<=0x8) || (ch>=0xb && ch<=0xc) || (ch>=0xe && ch<=0x1f))
                continue;
            danmuText.append(ch);
//            if(ch == 0x9 //\t
//                    || ch == 0xA //\n
//                    || ch == 0xD //\r
//                    || (ch >= 0x20 && ch <= 0xD7FF)
//                    || (ch >= 0xE000 && ch <= 0xFFFD)
//                    )
//                danmuText.append(ch);
        }
        writer.writeCharacters(danmuText);
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndDocument();
}

void Pool::exportKdFile(QDataStream &stream, const QList<int> &ids)
{
    PoolStateLock lock;
    if(!lock.tryLock(pid)) return;

    stream<<anime<<epType<<epIndex<<ep;
    stream<<GlobalObjects::danmuManager->getMatchedFile16Md5(pid).join(';');
	auto srcList(sourcesTable.values());
	int count = 0;
	for (auto iter = srcList.begin(); iter != srcList.end();)
	{
		if (!ids.isEmpty() && !ids.contains(iter->id))iter = srcList.erase(iter);
		else 
		{
			count += iter->count;
			++iter;
		}
	}
    stream<<srcList;
    stream<<count;
	int i = 0;
    for(const auto &danmu:commentList)
    {
        if(!ids.isEmpty() && !ids.contains(danmu->source)) continue;
        stream<<*danmu;
		++i;
    }
	Q_ASSERT(i == count);
}

void Pool::exportSimpleInfo(int srcId, QList<SimpleDanmuInfo> &simpleDanmuList)
{
    for(const auto &danmu:commentList)
    {
        if(danmu->source!=srcId) continue;
        SimpleDanmuInfo sd;
        sd.text=danmu->text;
        sd.originTime=danmu->originTime;
        sd.time=sd.originTime;
        simpleDanmuList.append(sd);
    }
}

QJsonArray Pool::exportJson()
{
    return exportJson(commentList);
}

QJsonObject Pool::exportFullJson()
{
    QJsonArray danmuArray(exportJson(commentList, true));
    QJsonArray sourceArray;
    for(auto &source:sourcesTable)
    {
        QString scriptName;
        auto s = GlobalObjects::scriptManager->getScript(source.scriptId);
        if(s) scriptName = s->name();
        QJsonObject sourceObj
        {
            {"name", source.title},
            {"id", source.id},
            {"duration", source.duration},
            {"delay", source.delay},
            {"scriptId", source.scriptId},
            {"scriptName", scriptName},
            {"scriptData", source.scriptData},
            {"timeline", source.timelineStr()}
        };
        sourceArray.append(sourceObj);
    }
    QJsonObject poolObj
    {
        {"source", sourceArray},
        {"comment", danmuArray}
    };
    return poolObj;
}

QJsonArray Pool::exportJson(const QList<QSharedPointer<DanmuComment> > &danmuList, bool useOrigin)
{
    QJsonArray danmuArray;
    for(const auto &danmu:danmuList)
    {
        if(danmu->blockBy!=-1) continue;
        if(useOrigin)
        {
            danmuArray.append(QJsonArray({danmu->originTime/1000.0,danmu->type,danmu->color,danmu->source,danmu->text}));
        }
        else
        {
            danmuArray.append(QJsonArray({danmu->time/1000.0,danmu->type,danmu->color,danmu->sender,danmu->text}));
        }
    }
    return danmuArray;
}

QString Pool::getPoolCode(const QStringList &addition) const
{
    if(sourcesTable.isEmpty() && addition.isEmpty()) return QString();
    QJsonArray poolArray;
    //addition: magnet,animeName,epName,file16MBHash
    for(const QString &item:addition)
        poolArray.append(QJsonValue(item));
    for(const auto &src:sourcesTable)
    {
        QFileInfo fi(src.scriptData);
        if(fi.exists()) continue;
        poolArray.append(QJsonArray({src.title,src.scriptId,src.scriptData,src.delay,src.timelineStr()}));
    }
    if(poolArray.isEmpty()) return QString();
    QByteArray compressedBytes;
    Network::gzipCompress(QJsonDocument(poolArray).toJson(QJsonDocument::Compact),compressedBytes);
    return compressedBytes.toBase64();
}

bool Pool::addPoolCode(const QString &code, bool hasAddition)
{
    QJsonArray poolArray(getPoolCodeInfo(code));
    if(poolArray.isEmpty()) return false;
    if(hasAddition && poolArray.count()<4) return false;
    int i=hasAddition?4:0;
    QList<DanmuComment *> emptyList;
    for(;i<poolArray.count();++i)
    {
        addSourceJson(poolArray[i].toArray());
    }
    if(used)
    {
        emit poolChanged(true);
    }
    return true;
}

bool Pool::addPoolCode(const QJsonArray &infoArray)
{
    if(infoArray.isEmpty()) return false;
    for(int i=0;i<infoArray.count();++i)
    {
        addSourceJson(infoArray[i].toArray());
    }
    if(used)
    {
        emit poolChanged(true);
    }
    return true;
}

QJsonArray Pool::getPoolCodeInfo(const QString &code)
{
    QByteArray base64Src(QByteArray::fromBase64(code.toUtf8()));
    QByteArray jsonBytes;
    if(Network::gzipDecompress(base64Src,jsonBytes)!=0) return QJsonArray();
    try
    {
        return Network::toJson(jsonBytes).array();
    } catch (Network::NetworkError &) {
        return QJsonArray();
    }
}


QSet<QString> Pool::getDanmuHashSet(int sourceId)
{
    if(sourceId != -1 && !sourcesTable.contains(sourceId)) return QSet<QString>();
    QSet<QString> hashSet;
    for(QSharedPointer<DanmuComment> &danmu:commentList)
    {
        if(danmu->source==sourceId || sourceId==-1)
        {
            QByteArray hashData(QString("%0%1%2%3").arg(danmu->text).arg(danmu->originTime).arg(danmu->sender).arg(danmu->color).toUtf8());
            hashSet.insert(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex());
        }
    }
    return hashSet;
}

void Pool::addSourceJson(const QJsonArray &array)
{
    if(array.count()!=5) return;
    DanmuSource srcInfo;
    srcInfo.title=array[0].toString();
    srcInfo.scriptId = array[1].toString();
    srcInfo.scriptData = array[2].toString();
    srcInfo.delay=array[3].toInt();
    srcInfo.count=0;
    srcInfo.setTimeline(array[3].toString());
    QList<DanmuComment *> emptyList;
    addSource(srcInfo,emptyList);
}

void Pool::setDelay(DanmuComment *danmu)
{
    auto srcInfo=&sourcesTable[danmu->source];
    int delay=0;
    for(auto &spaceItem:srcInfo->timelineInfo)
    {
        if(danmu->originTime>spaceItem.first)delay+=spaceItem.second;
        else break;
    }
    delay+=srcInfo->delay;
    danmu->time=danmu->originTime+delay<0?danmu->originTime:danmu->originTime+delay;
}

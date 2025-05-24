#include "danmumanager.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QXmlStreamWriter>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include "pool.h"
#include "Common/threadtask.h"
#include "Common/network.h"
#include "Common/logger.h"
#include "Common/dbmanager.h"
#include "../common.h"
#include "../blocker.h"
#include "../danmuprovider.h"
#include "globalobjects.h"

DanmuManager *PoolStateLock::manager=nullptr;
DanmuManager::DanmuManager(QObject *parent) : QObject(parent),countInited(false)
{
    poolCache.reset(new LRUCache<QString, Pool *>("DanmuPool", [](Pool *p){return !p->used && p->clean();}));
    PoolStateLock::manager=this;
    loadAllPool();
}

DanmuManager::~DanmuManager()
{
    for(auto pool:pools) pool->deleteLater();
}

Pool *DanmuManager::getPool(const QString &pid, bool loadDanmu)
{
    Pool *pool = nullptr;
    {
        QMutexLocker locker(&poolsLock);
        pool = pools.value(pid,nullptr);
    }
    if(pool && loadDanmu)
    {
        pool->load();
        poolCache->put(pool->pid, pool);
    }
    return pool;
}

Pool *DanmuManager::getPool(const QString &animeTitle, EpType epType, double epIndex, bool loadDanmu)
{
    return getPool(getPoolId(animeTitle, epType, epIndex),loadDanmu);
}

void DanmuManager::loadPoolInfo(QList<DanmuPoolNode *> &poolNodeList)
{
    if(!countInited)
    {
        ThreadTask task(GlobalObjects::workThread);
        task.Run([this](){
            //get danmu count
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
            for (int i = 0; i < DanmuTableCount; ++i)
            {
                query.exec(QString("select PoolID,Source,count(PoolID) as DanmuCount from danmu_%1 group by PoolID,Source").arg(i));
                int pidNo = query.record().indexOf("PoolID"),
                    srcNo = query.record().indexOf("Source"),
                    countNo=query.record().indexOf("DanmuCount");
                while (query.next())
                {
                    Pool *pool = pools.value(query.value(pidNo).toString(),nullptr);
                    Q_ASSERT(pool);
                    int src_id=query.value(srcNo).toInt();
                    if(pool->sourcesTable.contains(src_id))
                    {
                        pool->sourcesTable[src_id].count=query.value(countNo).toInt();
                    }
                }

            }
            return 0;
        });
        countInited=true;
    }
    qDeleteAll(poolNodeList);
    poolNodeList.clear();
    QMap<QString,DanmuPoolNode *> animeMap;
    for(Pool *pool:pools)
    {
        DanmuPoolNode *animeNode=animeMap.value(pool->anime,nullptr);
        if(!animeNode)
        {
            animeNode=new DanmuPoolNode(DanmuPoolNode::AnimeNode);
            animeNode->title=pool->anime;
            animeMap.insert(pool->anime,animeNode);
            poolNodeList.append(animeNode);
        }
        EpInfo ep(pool->epType, pool->epIndex, pool->ep);
        DanmuPoolEpNode *epNode=new DanmuPoolEpNode(ep,animeNode);
        epNode->idInfo=pool->pid;
        for(const auto &src:pool->sourcesTable)
        {
            new DanmuPoolSourceNode(src, epNode);
        }
    }
    for(DanmuPoolNode *animeNode:poolNodeList)
    {
        animeNode->setCount();
    }
    emit workerStateMessage("Done");
}

void DanmuManager::exportPool(const QList<DanmuPoolNode *> &exportList, const QString &dir, bool useTimeline, bool applyBlockRule)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([this,&exportList,&dir,useTimeline,applyBlockRule](){
        for(const DanmuPoolNode *node:exportList)
        {
            if(node->type==DanmuPoolNode::AnimeNode && node->checkStatus!=Qt::Unchecked)
            {
                QString animeTitle(node->title);
                animeTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"),"");
                for(DanmuPoolNode *epNode:*node->children)
                {
                    if(epNode->checkStatus==Qt::Unchecked)continue;
                    QString epTitle(epNode->title);
                    epTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"),"");
                    QFileInfo fi(dir,QString("%1-%2.xml").arg(animeTitle, epTitle));
                    emit workerStateMessage(tr("Exporting: %1").arg(fi.fileName()));
                    Pool *pool=getPool(epNode->idInfo);
                    QList<int> srcList;
                    if(epNode->checkStatus!=Qt::Checked)
                    {
                        for(DanmuPoolNode *srcNode:*epNode->children)
                        {
                            if(srcNode->checkStatus==Qt::Checked)
                                srcList<<static_cast<DanmuPoolSourceNode *>(srcNode)->srcId;
                        }
                    }
                    pool->exportPool(fi.absoluteFilePath(),useTimeline,applyBlockRule,srcList);
                }
            }
        }
        emit workerStateMessage("Done");
        return 0;
    });
}

void DanmuManager::exportKdFile(const QList<DanmuPoolNode *> &exportList, const QString &dir, const QString &comment)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([this,&exportList, dir,comment](){
        for(const DanmuPoolNode *node:exportList)
        {
            if(node->type==DanmuPoolNode::AnimeNode && node->checkStatus!=Qt::Unchecked)
            {
                QString animeTitle(node->title);
                animeTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"),"");
                QFileInfo fi(dir,QString("%1.kd").arg(animeTitle));
                emit workerStateMessage(tr("Exporting: %1").arg(fi.fileName()));
                QFile kdFile(fi.absoluteFilePath());
                bool ret=kdFile.open(QIODevice::WriteOnly);
                if(!ret)
                {
                    emit workerStateMessage(tr("Create File Failed: %1").arg(fi.fileName()));
                    continue;
                }
                QByteArray content;
                QBuffer buffer(&content);
                buffer.open(QIODevice::WriteOnly);
                QDataStream ds(&buffer);
                for(DanmuPoolNode *epNode:*node->children)
                {
                    if(epNode->checkStatus==Qt::Unchecked)continue;
                    Pool *pool=getPool(epNode->idInfo);
                    QList<int> srcList;
                    if(epNode->checkStatus!=Qt::Checked)
                    {
                        for(DanmuPoolNode *srcNode:*epNode->children)
                        {
                            if(srcNode->checkStatus==Qt::Checked)
                                srcList<<static_cast<DanmuPoolSourceNode *>(srcNode)->srcId;
                        }
                    }
                    ds<<int(0x23);
                    pool->exportKdFile(ds,srcList);
                }
                QByteArray compressedContent;
                Network::gzipCompress(content,compressedContent);
                QDataStream fs(&kdFile);
                fs<<QString("kd")<<comment<<compressedContent;
            }
        }
        emit workerStateMessage("Done");
        return 0;
    });
}

int DanmuManager::importKdFile(const QString &fileName, QWidget *parent)
{
    QFile kdFile(fileName);
    bool ret=kdFile.open(QIODevice::ReadOnly);
    if(!ret) return -1;
    QDataStream fs(&kdFile);
    QString head,comment;
    fs>>head;
    if(head!="kd") return -2;
    fs>>comment;
    bool readContent=comment.isEmpty();
    if(!comment.isEmpty())
    {
        QMessageBox::StandardButton btn =
                QMessageBox::information(parent,tr("Kd Comment"),comment,
                                 QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Ok);
        readContent=(btn==QMessageBox::Ok);
    }
    if(!readContent) return 0;
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([this,&fs](){
        QByteArray compressedConent,content;
        fs>>compressedConent;
        if(Network::gzipDecompress(compressedConent,content)!=0) return -2;
        QBuffer buffer(&content);
        buffer.open(QIODevice::ReadOnly);
        QDataStream ds(&buffer);
        while(true)
        {
            int poolFlag=0;
            ds>>poolFlag;
            if(poolFlag!=0x23) break;

            QString curAnime,curEp,file16MD5;
            EpType epType;
            double epIndex;
            QList<DanmuSource> srcInfoList;
            QHash<int, QPair<DanmuSource,QVector<DanmuComment *>>> danmuInfo;
            int danmuConut=0;

            ds>>curAnime>>epType>>epIndex>>curEp>>file16MD5>>srcInfoList>>danmuConut;
            emit workerStateMessage(tr("Adding: %1-%2").arg(curAnime,curEp));
            for(auto &src:srcInfoList)
            {
                danmuInfo[src.id].first=src;
            }
            while(danmuConut-- > 0)
            {
                DanmuComment *danmu=new DanmuComment;
                ds>>*danmu;
                if(danmu->type!=DanmuComment::UNKNOW)
                {
                    Q_ASSERT(danmuInfo.contains(danmu->source));
                    danmuInfo[danmu->source].second.append(danmu);
                }
                else delete danmu;
            }
            QStringList file16Md5List(file16MD5.split(';',Qt::SkipEmptyParts));
            QString pid(this->createPool(curAnime,epType, epIndex, curEp,file16Md5List.count()==1?file16Md5List.first():""));
            if(file16Md5List.count()>1)
            {
                for(const QString &md5:file16Md5List) this->setMatch(md5,pid);
            }
            Pool *pool=getPool(pid);
			Q_ASSERT(pool);
            int c = 1;
            for(auto &srcItem:danmuInfo)
            {
                pool->addSource(srcItem.first,srcItem.second, c==danmuInfo.size());
                ++c;
            }
        }
        emit workerStateMessage("Done");
        return 1;
    }).toInt();
}

QStringList DanmuManager::getMatchedFile16Md5(const QString &pid)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
    query.exec(QString("select MD5 from match where PoolID='%1'").arg(pid));
    QStringList md5s;
    while (query.next())
    {
        md5s<<query.value(0).toString();
    }
    return md5s;
}

QString DanmuManager::updateMatch(const QString &fileName, const MatchResult &newMatchInfo)
{
    return createPool(newMatchInfo.name, newMatchInfo.ep.type, newMatchInfo.ep.index, newMatchInfo.ep.name, getFileHash(fileName));
}

void DanmuManager::removeMatch(const QString &fileName)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
    query.exec(QString("delete from match where MD5='%1'").arg(getFileHash(fileName)));
}


QString DanmuManager::getFileHash(const QString &fileName)
{
    QFile mediaFile(fileName);
    bool ret=mediaFile.open(QIODevice::ReadOnly);
    if(!ret) return QString();
    QByteArray file16MB = mediaFile.read(16*1024*1024);
    return QCryptographicHash::hash(file16MB,QCryptographicHash::Md5).toHex();
}

void DanmuManager::localSearch(const QString &keyword, QList<AnimeLite> &results)
{
    results.clear();
    QHash<QString, QList<Pool *>> animePools;
    for(Pool *pool:pools)
    {
        if(pool->anime.contains(keyword))
        {
            animePools[pool->anime].append(pool);
        }
    }
    for(auto iter=animePools.begin(); iter!=animePools.end(); ++iter)
    {
        AnimeLite anime;
        anime.name = iter.key();
        anime.epList.reset(new QVector<EpInfo>);
        for(auto p : iter.value())
        {
            EpInfo ep;
            ep.name = p->ep;
            ep.type = p->epType;
            ep.index = p->epIndex;
            anime.epList->append(ep);
        }
        std::sort(anime.epList->begin(), anime.epList->end());
        results.append(anime);
    }
}

void DanmuManager::localMatch(const QString &path, MatchResult &result)
{
    QString hashStr(getFileHash(path));
    do
    {
        if(hashStr.isEmpty()) break;
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
        query.exec(QString("select poolID from match where MD5='%1'").arg(hashStr));
        if(!query.first()) break;
        QString poolID=query.value(0).toString();
        Pool *pool=getPool(poolID,false);
        if(!pool) break;
        result.name = pool->anime;
        result.ep.name = pool->ep;
        result.ep.type = pool->epType;
        result.ep.index = pool->epIndex;
        result.ep.localFile = path;
        result.success = true;
        return;
    }while(false);
    result.success = false;
}

QString DanmuManager::createPool(const QString &animeTitle, EpType epType, double epIndex, const QString &epName,  const QString &fileHash)
{
    QString poolId(getPoolId(animeTitle, epType, epIndex));
    Pool *pool = nullptr;
    {
        QMutexLocker locker(&poolsLock);
        pool = pools.value(poolId, nullptr);
    }
    if(!pool)
    {
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
        query.prepare("insert into pool(PoolID,Anime,EpType,EpIndex,EpName) values(?,?,?,?,?)");
        query.bindValue(0,poolId);
        query.bindValue(1,animeTitle);
        query.bindValue(2,epType);
        query.bindValue(3,epIndex);
        query.bindValue(4,epName);
        query.exec();
        QMutexLocker locker(&poolsLock);
        pools.insert(poolId,new Pool(poolId,animeTitle,epName,epType,epIndex));
    }
    else
    {
        if(!epName.isEmpty() && pool->ep != epName)
        {
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
            query.prepare("update pool set EpName=? where PoolID=?");
            query.bindValue(0,epName);
            query.bindValue(1,poolId);
            query.exec();
            pool->ep = epName;
        }
    }
    if(!fileHash.isEmpty())
        setMatch(fileHash,poolId);
    return poolId;
}

QString DanmuManager::createPool(const QString &path, const MatchResult &match)
{
    return createPool(match.name, match.ep.type, match.ep.index, match.ep.name, getFileHash(path));
}

QString DanmuManager::renamePool(const QString &pid, const QString &nAnimeTitle, EpType nType, double nIndex,  const QString &nEpTitle)
{
    Pool *pool=getPool(pid,false);
    if(!pool) return QString();
    if(nAnimeTitle==pool->anime && nType==pool->epType && nIndex==pool->epIndex)
    {
        if(nEpTitle != pool->ep)
        {
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
            query.prepare("update pool set EpName=? where PoolID=?");
            query.bindValue(0,nEpTitle);
            query.bindValue(1,pid);
            query.exec();
            pool->ep = nEpTitle;
        }
        return pool->pid;
    }
    QString npid(getPoolId(nAnimeTitle, nType, nIndex));
    if(pools.contains(npid)) return QString();
    PoolStateLock lock;
    if(!lock.tryLock(pid)) return QString();
    int oldId=DanmuPoolNode::idHash(pool->pid),newId=DanmuPoolNode::idHash(npid);
    QSqlDatabase db(DBManager::instance()->getDB(DBManager::Comment));
    db.transaction();

    QSqlQuery query(db);
    query.prepare("update pool set PoolID=?,Anime=?,EpType=?,EpIndex=?,EpName=? where PoolID=?");
    query.bindValue(0,npid);
    query.bindValue(1,nAnimeTitle);
    query.bindValue(2,nType);
    query.bindValue(3,nIndex);
    query.bindValue(4,nEpTitle);
    query.bindValue(5,pool->pid);
    query.exec();

    if(oldId!=newId)
    {
        query.prepare(QString("insert into danmu_%1 select * from danmu_%2 where PoolID=?").arg(newId).arg(oldId));
        query.bindValue(0,npid);
        query.exec();
        query.exec(QString("delete from danmu_%1 where PoolID='%2'").arg(oldId).arg(npid));
    }

    if(!db.commit()) return QString();

    QMutexLocker locker(&poolsLock);
    poolCache->remove(pid, false);

    pools.remove(pid);
    pool->pid=npid;
    pool->anime=nAnimeTitle;
    pool->epType=nType;
    pool->epIndex=nIndex;
    pool->ep=nEpTitle;
    pools.insert(npid,pool);

    return npid;
}

QString DanmuManager::getPoolId(const QString &animeTitle, EpType epType, double epIndex)
{
    QByteArray hashData = QString("%1 %2.%3").arg(animeTitle).arg(epType).arg(epIndex).toUtf8();
    return QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex();
}

void DanmuManager::setMatch(const QString &fileHash, const QString &poolId)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
    query.exec(QString("select * from match where MD5='%1'").arg(fileHash));
    if(query.first())
    {
        query.exec(QString("delete from match where MD5='%1'").arg(fileHash));
    }
    query.prepare("insert into match(MD5,PoolID) values(?,?)");
    query.bindValue(0,fileHash);
    query.bindValue(1,poolId);
    query.exec();
}

void DanmuManager::deletePool(const QList<DanmuPoolNode *> &deleteList)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([&deleteList,this](){
        QSqlDatabase db = DBManager::instance()->getDB(DBManager::Comment);
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
                        emit workerStateMessage(tr("Deleting: %1 %2").arg(node->title, epNode->title));
                        PoolStateLock lock;
                        if(lock.tryLock(epNode->idInfo))
                        {
                            deletePool(epNode->idInfo);
                            query.prepare("delete from pool where PoolID=?");
                            query.bindValue(0,epNode->idInfo);
                            query.exec();
                        }
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
                        Pool *pool=getPool(epNode->idInfo,false);
                        for(DanmuPoolNode *srcNode:*epNode->children)
                        {
                            if(srcNode->checkStatus!=Qt::Checked)continue;
                            if(pool->deleteSource(static_cast<DanmuPoolSourceNode *>(srcNode)->srcId,false))
                            {
                                emit workerStateMessage(tr("Deleting: %1 %2 %3").arg(node->title, epNode->title, srcNode->title));
                                query.bindValue(1,static_cast<DanmuPoolSourceNode *>(srcNode)->srcId);
                                query.exec();
                                deleteDMQuery.bindValue(1,static_cast<DanmuPoolSourceNode *>(srcNode)->srcId);
                                deleteDMQuery.exec();
                            }
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
        query.exec("VACUUM;");
        emit workerStateMessage("Done");
        return 0;
    });
}

void DanmuManager::deleteSource(const QString &pid, int srcId)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([pid,srcId](){
        QSqlDatabase db = DBManager::instance()->getDB(DBManager::Comment);
        QSqlQuery query(db);
        db.transaction();
        query.exec(QString("delete from source where PoolID='%1' and ID=%2").arg(pid).arg(srcId));
        int tableId=DanmuPoolNode::idHash(pid);
        query.exec(QString("delete from danmu_%1 where PoolID='%2' and Source=%3").arg(tableId).arg(pid).arg(srcId));
        db.commit();
    });
}

void DanmuManager::deleteDanmu(const QString &pid, const QSharedPointer<DanmuComment> danmu)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([pid,danmu](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
        int tableId=DanmuPoolNode::idHash(pid);
        query.prepare(QString("delete from danmu_%1 where PoolID=? and Date=? and User=? and Text=? and Source=?").arg(tableId));
        query.bindValue(0,pid);
        query.bindValue(1,danmu->date);
        query.bindValue(2,danmu->sender);
        query.bindValue(3,danmu->text);
        query.bindValue(4,danmu->source);
        query.exec();
    });
}

void DanmuManager::updatePool(QList<DanmuPoolNode *> &updateList)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([this,&updateList](){
        for(const DanmuPoolNode *animeNode:updateList)
        {
            if(animeNode->checkStatus==Qt::Unchecked)continue;
            for(DanmuPoolNode *epNode:*animeNode->children)
            {
                if(epNode->checkStatus==Qt::Unchecked)continue;
                Pool *pool=getPool(epNode->idInfo);
                for(DanmuPoolNode *sourceNode:*epNode->children)
                {
                    if(sourceNode->checkStatus==Qt::Unchecked)continue;
                    emit workerStateMessage(tr("Updating: %1 %2 %3").arg(animeNode->title, epNode->title, sourceNode->idInfo));
                    DanmuPoolSourceNode *srcNode(static_cast<DanmuPoolSourceNode *>(sourceNode));
                    pool->update(srcNode->srcId);
                    srcNode->danmuCount=pool->sources()[srcNode->srcId].count;
                }
            }
        }
        emit workerStateMessage("Done");
        return 0;
    });
}

void DanmuManager::setPoolDelay(QList<DanmuPoolNode *> &updateList, int delay)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([this,&updateList, delay](){
        for(const DanmuPoolNode *animeNode:updateList)
        {
            if(animeNode->checkStatus==Qt::Unchecked)continue;
            for(DanmuPoolNode *epNode:*animeNode->children)
            {
                if(epNode->checkStatus==Qt::Unchecked)continue;
                Pool *pool=getPool(epNode->idInfo);
                for(DanmuPoolNode *sourceNode:*epNode->children)
                {
                    if(sourceNode->checkStatus==Qt::Unchecked)continue;
                    emit workerStateMessage(tr("Set Delay: %1 %2 %3").arg(animeNode->title, epNode->title, sourceNode->idInfo));
                    DanmuPoolSourceNode *srcNode(static_cast<DanmuPoolSourceNode *>(sourceNode));
                    pool->setDelay(srcNode->srcId, delay);
                    srcNode->delay = delay;
                }
            }
        }
        emit workerStateMessage("Done");
        return 0;
    });
}

void DanmuManager::updateSourceDelay(const QString &pid, const DanmuSource *sourceInfo)
{
    ThreadTask task(GlobalObjects::workThread);
    int delay=sourceInfo->delay,id=sourceInfo->id;
    task.RunOnce([delay,id,pid](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
        query.prepare("update source set Delay= ? where PoolID=? and ID=?");
        query.bindValue(0,delay);
        query.bindValue(1,pid);
        query.bindValue(2,id);
        query.exec();
    });
}

QVector<DanmuComment *> DanmuManager::updateSource(const DanmuSource *sourceInfo, const QSet<QString> &danmuHashSet)
{
    QVector<DanmuComment *> tmpList;
    auto ret = GlobalObjects::danmuProvider->downloadDanmu(sourceInfo, tmpList);
    if(!ret)
    {
        Logger::logger()->log(Logger::Script, QString("update source[%1] failed: %2").arg(sourceInfo->title, ret.info));
        return tmpList;
    }
    GlobalObjects::blocker->preFilter(tmpList);
    for(auto iter=tmpList.begin();iter!=tmpList.end();)
    {
        QByteArray hashData(QString("%0%1%2%3").arg((*iter)->text, QString::number((*iter)->originTime), (*iter)->sender, QString::number((*iter)->color)).toUtf8());
        if(danmuHashSet.contains(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex()) || (*iter)->text.isEmpty())
        {
            delete *iter;
            iter=tmpList.erase(iter);
        }
        else
        {
            (*iter)->source=sourceInfo->id;
            ++iter;
        }
    }
    return tmpList;
}

void DanmuManager::loadAllPool()
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
    //get all danmu pools
    query.exec("select * from pool");
    int idNo = query.record().indexOf("PoolID"),
        animeNo=query.record().indexOf("Anime"),
        epTypeNo=query.record().indexOf("EpType"),
        epIndexNo=query.record().indexOf("EpIndex"),
        epNameNo=query.record().indexOf("EpName");
    while (query.next())
    {
        QString poolId(query.value(idNo).toString());
        pools.insert(poolId,  new Pool(poolId,query.value(animeNo).toString(),
                              query.value(epNameNo).toString(),
                              EpType(query.value(epTypeNo).toInt()),
                              query.value(epIndexNo).toDouble()));
    }
    //get source info
    query.exec("select * from source");
    int s_pidNo = query.record().indexOf("PoolID"),
        s_idNo = query.record().indexOf("ID"),
        s_titleNo = query.record().indexOf("Title"),
        s_descNo = query.record().indexOf("Desc"),
        s_scriptIdNo = query.record().indexOf("ScriptId"),
        s_scriptDataNo = query.record().indexOf("ScriptData"),
        s_delayNo=query.record().indexOf("Delay"),
        s_durationNo=query.record().indexOf("Duration"),
        s_timelineNo=query.record().indexOf("TimeLine");
    while (query.next())
    {
        Pool *pool = pools.value(query.value(s_pidNo).toString(),nullptr);
        Q_ASSERT(pool);
        DanmuSource srcInfo;
        srcInfo.id=query.value(s_idNo).toInt();
        srcInfo.title = query.value(s_titleNo).toString();
        srcInfo.desc = query.value(s_descNo).toString();
        srcInfo.scriptId = query.value(s_scriptIdNo).toString();
        srcInfo.scriptData = query.value(s_scriptDataNo).toString();
        srcInfo.delay=query.value(s_delayNo).toInt();
        srcInfo.duration=query.value(s_durationNo).toInt();
        srcInfo.count=0;
        srcInfo.show=true;
        srcInfo.setTimeline(query.value(s_timelineNo).toString());
        pool->sourcesTable.insert(srcInfo.id, srcInfo);
    }
}

void DanmuManager::deletePool(const QString &pid)
{
    QMutexLocker locker(&poolsLock);
    Pool *pool=pools.value(pid,nullptr);
    if(pool)
    {
        poolCache->remove(pid);
        pools.remove(pid);
        delete pool;
    }

}

void DanmuManager::updateSourceTimeline(const QString &pid, const DanmuSource *sourceInfo)
{
    ThreadTask task(GlobalObjects::workThread);
    QString timeline(sourceInfo->timelineStr());
    int id = sourceInfo->id;
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
        query.prepare("update source set TimeLine= ? where PoolID=? and ID=?");
        query.bindValue(0,timeline);
        query.bindValue(1,pid);
        query.bindValue(2,id);
        query.exec();
    });

}

void DanmuManager::loadPool(Pool *pool)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([pool](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Comment));
        auto &sources=pool->sourcesTable;
        for(auto &src:sources)
            src.count=0;
        int tableId=DanmuPoolNode::idHash(pool->id());
        query.exec(QString("select * from danmu_%1 where PoolID='%2'").arg(tableId).arg(pool->id()));
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
            QString text=query.value(textNo).toString();
            if(text.isEmpty()) continue;
            DanmuComment *danmu=new DanmuComment();
            danmu->color=query.value(colorNo).toInt();
            danmu->date=query.value(dateNo).toLongLong();
            int fontSizeLevel(query.value(sizeNo).toInt());
            danmu->fontSizeLevel=DanmuComment::FontSizeLevel(fontSizeLevel<3 && fontSizeLevel>=0?fontSizeLevel:0);
            danmu->sender=query.value(userNo).toString();
            int type(query.value(modeNo).toInt());
            danmu->type=DanmuComment::DanmuType(type<3 && type>=0?type:0);
            danmu->source=query.value(sourceNo).toInt();
            danmu->text=text;
            danmu->originTime=query.value(timeNo).toInt();

            Q_ASSERT(sources.contains(danmu->source));
            pool->setDelay(danmu);
            sources[danmu->source].count++;
            pool->commentList.append(QSharedPointer<DanmuComment>(danmu));
        }
        return 0;
    });
}

void DanmuManager::updatePool(Pool *pool, QVector<DanmuComment *> &outList, int sourceId)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([pool,&outList,sourceId,this](){
        const auto &danmuHashSet=pool->getDanmuHashSet(sourceId);
        if(sourceId==-1)
        {
            const auto &sourceTable=pool->sources();
            for(const auto &src:sourceTable)
            {
                outList.append(updateSource(&src,danmuHashSet));
            }
        }
        else
        {
            outList.append(updateSource(&pool->sourcesTable[sourceId],danmuHashSet));
        }
        return 0;
    });
}

void DanmuManager::saveSource(const QString &pid, const DanmuSource *source, const QVector<QSharedPointer<DanmuComment> > &danmuList)
{
    ThreadTask task(GlobalObjects::workThread);
    DanmuSource src;
    if(source!=nullptr) src=*source;
    task.RunOnce([pid,src,source,danmuList](){
        QSqlDatabase db = DBManager::instance()->getDB(DBManager::Comment);
        QSqlQuery query(db);
        db.transaction();
        if(source)
        {
            query.prepare("insert into source(PoolID,ID,Title,Desc,ScriptId,ScriptData,Delay,Duration,TimeLine) values(?,?,?,?,?,?,?,?,?)");
            query.bindValue(0,pid);
            query.bindValue(1,src.id);
            query.bindValue(2,src.title);
            query.bindValue(3,src.desc);
            query.bindValue(4,src.scriptId);
            query.bindValue(5,src.scriptData);
            query.bindValue(6,src.delay);
            query.bindValue(7,src.duration);
            query.bindValue(8,src.timelineStr());
            query.exec();
        }
        int tableId=DanmuPoolNode::idHash(pid);
        query.prepare(QString("insert into danmu_%1(PoolID,Time,Date,Color,Mode,Size,Source,User,Text) values(?,?,?,?,?,?,?,?,?)").arg(tableId));
        for(const auto &danmu: danmuList)
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
        db.commit();
    });
}

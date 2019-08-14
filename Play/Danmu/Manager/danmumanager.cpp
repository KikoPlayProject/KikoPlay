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
#include "../common.h"
#include "../blocker.h"
#include "../danmupool.h"
#include "../providermanager.h"
#include "globalobjects.h"

DanmuManager *PoolStateLock::manager=nullptr;
DanmuManager::DanmuManager(QObject *parent) : QObject(parent),countInited(false)
{
    PoolStateLock::manager=this;
    loadAllPool();
}

DanmuManager::~DanmuManager()
{
    //poolWorker->deleteLater();
    for(auto pool:pools) pool->deleteLater();
}

Pool *DanmuManager::getPool(const QString &pid, bool loadDanmu)
{
    QMutexLocker locker(&removeLock);
    Pool *pool=pools.value(pid,nullptr);
    if(pool && loadDanmu)
    {
        pool->load();
        refreshCache(pool);
    }
    return pool;
}

Pool *DanmuManager::getPool(const QString &animeTitle, const QString &title, bool loadDanmu)
{
    return getPool(getPoolId(animeTitle,title),loadDanmu);
}

void DanmuManager::loadPoolInfo(QList<DanmuPoolNode *> &poolNodeList)
{
    if(!countInited)
    {
        ThreadTask task(GlobalObjects::workThread);
        task.Run([this](){
            //get danmu count
            QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
            for (int i = 0; i < DanmuTableCount; ++i)
            {
                query.exec(QString("select PoolID,Source,count(*) as DanmuCount from danmu_%1 group by PoolID,Source").arg(i));
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
        DanmuPoolNode *epNode=new DanmuPoolNode(DanmuPoolNode::EpNode,animeNode);
        epNode->title=pool->ep;
        epNode->idInfo=pool->pid;
        for(const auto &src:pool->sourcesTable)
        {
            DanmuPoolSourceNode *sourceNode=new DanmuPoolSourceNode(epNode);
            sourceNode->title=src.name;
            sourceNode->idInfo=src.url;
            sourceNode->srcId=src.id;
            sourceNode->delay=src.delay;
            sourceNode->setTimeline(src);
			sourceNode->danmuCount = src.count;
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
                animeTitle.replace(QRegExp("[\\\\/:*?\"<>|]"),"");
                for(DanmuPoolNode *epNode:*node->children)
                {
                    if(epNode->checkStatus==Qt::Unchecked)continue;
                    QString epTitle(epNode->title);
                    epTitle.replace(QRegExp("[\\\\/:*?\"<>|]"),"");
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
                animeTitle.replace(QRegExp("[\\\\/:*?\"<>|]"),"");
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
            QList<DanmuSourceInfo> srcInfoList;
            QHash<int, QPair<DanmuSourceInfo,QList<DanmuComment *> > > danmuInfo;
            int danmuConut=0;

            ds>>curAnime>>curEp>>file16MD5>>srcInfoList>>danmuConut;
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
            QStringList file16Md5List(file16MD5.split(';',QString::SkipEmptyParts));
            QString pid(this->createPool(curAnime,curEp,file16Md5List.count()==1?file16Md5List.first():""));
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

QStringList DanmuManager::getAssociatedFile16Md5(const QString &pid)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
    query.exec(QString("select MD5 from match where PoolID='%1'").arg(pid));
    QStringList md5s;
    while (query.next())
    {
        md5s<<query.value(0).toString();
    }
    return md5s;
}

MatchInfo *DanmuManager::searchMatch(DanmuManager::MatchProvider from, const QString &keyword)
{
    switch (from)
    {
    case MatchProvider::DanDan:
        return ddSearch(keyword);
    case MatchProvider::Bangumi:
        return bgmSearch(keyword);
    case MatchProvider::Local:
        return localSearch(keyword);
    }
}

MatchInfo *DanmuManager::matchFrom(DanmuManager::MatchProvider from, const QString &fileName)
{
    switch (from)
    {
    case MatchProvider::DanDan:
        return ddMatch(fileName);
    case MatchProvider::Local:
        return localMatch(fileName);
    default:
        return nullptr;
    }
}

QString DanmuManager::updateMatch(const QString &fileName, const MatchInfo *newMatchInfo)
{
    MatchInfo::DetailInfo detailInfo=newMatchInfo->matches.first();
    return createPool(detailInfo.animeTitle,detailInfo.title,getFileHash(fileName));
}

MatchInfo *DanmuManager::ddSearch(const QString &keyword)
{
    ThreadTask task(GlobalObjects::workThread);
    QVariant ret= task.Run([&keyword](){
        QString baseUrl = "https://api.acplay.net/api/v2/search/episodes";
        QUrlQuery query;
        query.addQueryItem("anime", keyword);
        MatchInfo *searchInfo=new MatchInfo;
        try
        {
            QString str(Network::httpGet(baseUrl,query));
            QJsonDocument document(Network::toJson(str));
            do
            {
                if (!document.isObject()) break;
                QJsonObject obj = document.object();
                QJsonValue success = obj.value("success");
                if (success.type() != QJsonValue::Bool)break;
                if (!success.toBool())
                {
                    searchInfo->error = false;
                    return QVariant::fromValue(static_cast<void *>(searchInfo));;
                }
                QJsonValue animes=obj.value("animes");
                if(animes.type()!=QJsonValue::Array) break;
                QJsonArray animeArray=animes.toArray();
                for(auto animeIter=animeArray.begin();animeIter!=animeArray.end();++animeIter)
                {
                    if(!(*animeIter).isObject())continue;
                    QJsonObject animeObj=(*animeIter).toObject();
                    QJsonValue animeTitle=animeObj.value("animeTitle");
                    if(animeTitle.type()!=QJsonValue::String)continue;
                    QString animeTitleStr=animeTitle.toString().trimmed();
                    QJsonValue episodes= animeObj.value("episodes");
                    if(episodes.type()!=QJsonValue::Array) continue;
                    QJsonArray episodeArray=episodes.toArray();
                    for(auto episodeIter=episodeArray.begin();episodeIter!=episodeArray.end();++episodeIter)
                    {
                        if(!(*episodeIter).isObject())continue;
                        QJsonObject episodeObj=(*episodeIter).toObject();
                        QJsonValue episodeTitle=episodeObj.value("episodeTitle");
                        if(episodeTitle.type()!=QJsonValue::String)continue;
                        MatchInfo::DetailInfo detailInfo;
                        detailInfo.animeTitle=animeTitleStr;
                        detailInfo.title=episodeTitle.toString().trimmed();
                        searchInfo->matches.append(detailInfo);
                    }
                }
                searchInfo->error = false;
                return QVariant::fromValue(static_cast<void *>(searchInfo));;
            }while(false);
            searchInfo->error=true;
            searchInfo->errorInfo=QObject::tr("Reply JSON Format Error");
        }
        catch(Network::NetworkError &error)
        {
            searchInfo->error=true;
            searchInfo->errorInfo=error.errorInfo;
        }
        return QVariant::fromValue(static_cast<void *>(searchInfo));
    });
    return static_cast<MatchInfo *>(ret.value<void *>());
}

MatchInfo *DanmuManager::bgmSearch(const QString &keyword)
{
    ThreadTask task(GlobalObjects::workThread);
    QVariant ret= task.Run([&keyword](){
        QString baseUrl("https://api.bgm.tv/search/subject/"+keyword);
        QUrlQuery query;
        query.addQueryItem("type","2");
        query.addQueryItem("responseGroup","small");
        query.addQueryItem("start","0");
        query.addQueryItem("max_results","3");
        MatchInfo *searchInfo=new MatchInfo;
        try
        {
            QJsonDocument document(Network::toJson(Network::httpGet(baseUrl,query,QStringList()<<"Accept"<<"application/json")));
            QJsonArray results=document.object().value("list").toArray();
            for(auto iter=results.begin();iter!=results.end();++iter)
            {
                QJsonObject searchObj=(*iter).toObject();
                int bgmID=searchObj.value("id").toInt();
                QString animeTitle=searchObj.value("name_cn").toString().trimmed();
                if(animeTitle.isEmpty())animeTitle=searchObj.value("name").toString().trimmed();
                animeTitle.replace("&amp;","&");
                QString epUrl(QString("https://api.bgm.tv/subject/%1/ep").arg(bgmID));
                try
                {
                    QString str(Network::httpGet(epUrl,QUrlQuery(),QStringList()<<"Accept"<<"application/json"));
                    QJsonDocument document(Network::toJson(str));
                    QJsonObject obj = document.object();
                    QJsonArray epArray=obj.value("eps").toArray();
                    for(auto epIter=epArray.begin();epIter!=epArray.end();++epIter)
                    {
                        QJsonObject epobj=(*epIter).toObject();
                        QString epTitle(epobj.value("name_cn").toString().trimmed());
                        if(epTitle.isEmpty())epTitle=epobj.value("name").toString().trimmed();
                        epTitle.replace("&amp;","&");
                        MatchInfo::DetailInfo detailInfo;
                        detailInfo.animeTitle=animeTitle;
                        detailInfo.title=tr("No.%0 %1").arg(epobj.value("sort").toInt()).arg(epTitle);
                        searchInfo->matches.append(detailInfo);
                    }
                }
                catch(Network::NetworkError &)
                {
                    continue;
                }
                searchInfo->error = false;
            }
        }
        catch(Network::NetworkError &error)
        {
            searchInfo->error=true;
            searchInfo->errorInfo=error.errorInfo;
        }
        return QVariant::fromValue(static_cast<void *>(searchInfo));
    });
    return static_cast<MatchInfo *>(ret.value<void *>());
}

MatchInfo *DanmuManager::localSearch(const QString &keyword)
{
    MatchInfo *searchInfo=new MatchInfo;
    searchInfo->error=false;
    for(Pool *pool:pools)
    {
        if(pool->anime.contains(keyword) || pool->ep.contains(keyword))
        {
            MatchInfo::DetailInfo detailInfo;
            detailInfo.animeTitle=pool->anime;
            detailInfo.title=pool->ep;
            searchInfo->matches.append(detailInfo);
        }
    }
    return searchInfo;
}

MatchInfo *DanmuManager::ddMatch(const QString &fileName)
{
    ThreadTask task(GlobalObjects::workThread);
    QVariant ret= task.Run([&fileName,this](){
        QString hashStr(getFileHash(fileName));
        if(hashStr.isEmpty()) return QVariant::fromValue(static_cast<void *>(nullptr));

        MatchInfo *localMatchInfo=searchInMatchTable(hashStr);
        if(localMatchInfo) return QVariant::fromValue(static_cast<void *>(localMatchInfo));
        QFileInfo fileInfo(fileName);
        QJsonObject json;
        json.insert("fileName", fileInfo.baseName());
        json.insert("fileHash", hashStr);

        QJsonDocument document;
        document.setObject(json);
        QByteArray dataArray = document.toJson(QJsonDocument::Compact);

        QString baseUrl = "https://api.acplay.net/api/v2/match";
        MatchInfo *matchInfo=new MatchInfo;
        matchInfo->fileHash=hashStr;
        try
        {
            QString str(Network::httpPost(baseUrl,dataArray,QStringList()<<"Content-Type"<<"application/json"<<"Accept"<<"application/json"));
            QJsonDocument document(Network::toJson(str));
            do
            {
                if (!document.isObject()) break;
                QJsonObject obj = document.object();
                QJsonValue isMatched=obj.value("isMatched");
                if(isMatched.type()!=QJsonValue::Bool) break;
                matchInfo->success=isMatched.toBool();
                QJsonValue matches=obj.value("matches");
                if(matches.type()!=QJsonValue::Array) break;
                QJsonArray detailInfoArray=matches.toArray();
                for(auto iter=detailInfoArray.begin();iter!=detailInfoArray.end();++iter)
                {
                    if(!(*iter).isObject())continue;
                    QJsonObject detailObj=(*iter).toObject();
                    QJsonValue animeTitle=detailObj.value("animeTitle");
                    if(animeTitle.type()!=QJsonValue::String)continue;
                    QJsonValue episodeTitle=detailObj.value("episodeTitle");
                    if(episodeTitle.type()!=QJsonValue::String)continue;
                    MatchInfo::DetailInfo detailInfo;
                    detailInfo.animeTitle=animeTitle.toString().trimmed();
                    detailInfo.title=episodeTitle.toString().trimmed();
                    matchInfo->matches.append(detailInfo);
                }
                matchInfo->error = false;
                if(matchInfo->success && matchInfo->matches.count()>0)
                {
                    matchInfo->poolID=createPool(matchInfo->matches.first().animeTitle,matchInfo->matches.first().title,hashStr);
                }
                return QVariant::fromValue(static_cast<void *>(matchInfo));
            }while(false);
            matchInfo->error=true;
            matchInfo->errorInfo=QObject::tr("Reply JSON Format Error");
        }
        catch(Network::NetworkError &error)
        {
            matchInfo->error=true;
            matchInfo->errorInfo=error.errorInfo;
        }
        return QVariant::fromValue(static_cast<void *>(matchInfo));
    });
    return static_cast<MatchInfo *>(ret.value<void *>());
}

MatchInfo *DanmuManager::localMatch(const QString &fileName)
{
    QString hashStr(getFileHash(fileName));
    if(hashStr.isEmpty()) return nullptr;
    return searchInMatchTable(hashStr);
}

MatchInfo *DanmuManager::searchInMatchTable(const QString &fileHash)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
    query.exec(QString("select poolID from match where MD5='%1'").arg(fileHash));
    if(!query.first())return nullptr;
    QString poolID=query.value(0).toString();
    Pool *pool=getPool(poolID,false);
    if(!pool)return nullptr;
    MatchInfo *matchInfo=new MatchInfo;
    matchInfo->error=false;
    matchInfo->fileHash=fileHash;
    matchInfo->poolID=poolID;
    matchInfo->success=true;
    MatchInfo::DetailInfo detailInfo;
    detailInfo.title=pool->ep;
    detailInfo.animeTitle=pool->anime;
    matchInfo->matches.append(detailInfo);
    return matchInfo;
}

QString DanmuManager::getFileHash(const QString &fileName)
{
    QFile mediaFile(fileName);
    bool ret=mediaFile.open(QIODevice::ReadOnly);
    if(!ret) return QString();
    QByteArray file16MB = mediaFile.read(16*1024*1024);
    return QCryptographicHash::hash(file16MB,QCryptographicHash::Md5).toHex();
}

QString DanmuManager::createPool(const QString &animeTitle, const QString &title, const QString &fileHash)
{
    QString poolId(getPoolId(animeTitle,title));
    if(!pools.contains(poolId))
    {
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
        query.prepare("insert into pool(PoolID,AnimeTitle,Title) values(?,?,?)");
        query.bindValue(0,poolId);
        query.bindValue(1,animeTitle);
        query.bindValue(2,title);
        query.exec();
        QMutexLocker locker(&removeLock);
        pools.insert(poolId,new Pool(poolId,animeTitle,title));
    }
    if(!fileHash.isEmpty())
        setMatch(fileHash,poolId);
    return poolId;
}

QString DanmuManager::renamePool(const QString &pid, const QString &nAnimeTitle, const QString &nEpTitle)
{
    Pool *pool=getPool(pid,false);
    if(!pool) return QString();
    if(nAnimeTitle==pool->anime && nEpTitle==pool->ep) return pool->pid;
    PoolStateLock lock;
    if(!lock.tryLock(pid)) return QString();
    QString npid(getPoolId(nAnimeTitle,nEpTitle));
    int oldId=DanmuPoolNode::idHash(pool->pid),newId=DanmuPoolNode::idHash(npid);
    QSqlDatabase db(GlobalObjects::getDB(GlobalObjects::Comment_DB));
    db.transaction();

    QSqlQuery query(db);
    query.prepare("update pool set PoolID=?,AnimeTitle=?,Title=? where PoolID=?");
    query.bindValue(0,npid);
    query.bindValue(1,nAnimeTitle);
    query.bindValue(2,nEpTitle);
    query.bindValue(3,pool->pid);
    query.exec();

    if(oldId!=newId)
    {
        query.prepare(QString("insert into danmu_%1 select * from danmu_%2 where PoolID=?").arg(newId).arg(oldId));
        query.bindValue(0,npid);
        query.exec();
        query.exec(QString("delete from danmu_%1 where PoolID='%2'").arg(oldId).arg(npid));
    }

    if(!db.commit()) return QString();

    QMutexLocker locker(&removeLock);
    if(poolDanmuCacheInfo.contains(pid))
    {
        QMutexLocker locker(&cacheLock);
        poolDanmuCacheInfo.remove(pid);
    }
    pools.remove(pid);
    pool->pid=npid;
    pool->anime=nAnimeTitle;
    pool->ep=nEpTitle;
    pools.insert(npid,pool);

    return npid;
}

QString DanmuManager::getPoolId(const QString &animeTitle, const QString &title)
{
    QByteArray hashData = QString("%1-%2").arg(animeTitle).arg(title).toUtf8();
    return QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex();
}

void DanmuManager::setMatch(const QString &fileHash, const QString &poolId)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
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
        QSqlDatabase db=GlobalObjects::getDB(GlobalObjects::Comment_DB);
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
        emit workerStateMessage("Done");
        return 0;
    });
}

void DanmuManager::deleteSource(const QString &pid, int srcId)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([pid,srcId](){
        QSqlDatabase db = GlobalObjects::getDB(GlobalObjects::Comment_DB);
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
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
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

void DanmuManager::updateSourceDelay(const QString &pid, const DanmuSourceInfo *sourceInfo)
{
    ThreadTask task(GlobalObjects::workThread);
    int delay=sourceInfo->delay,id=sourceInfo->id;
    task.RunOnce([delay,id,pid](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
        query.prepare("update source set Delay= ? where PoolID=? and ID=?");
        query.bindValue(0,delay);
        query.bindValue(1,pid);
        query.bindValue(2,id);
        query.exec();
    });
}

QList<DanmuComment *> DanmuManager::updateSource(const DanmuSourceInfo *sourceInfo, const QSet<QString> &danmuHashSet)
{
    QList<DanmuComment *> tmpList;
    QString errInfo = GlobalObjects::providerManager->downloadBySourceURL(sourceInfo->url,tmpList);
    if(!errInfo.isEmpty())return tmpList;
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
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
    //get all danmu pools
    query.exec("select * from pool");
    int idNo = query.record().indexOf("PoolID"),
        animeNo=query.record().indexOf("AnimeTitle"),
        epNo=query.record().indexOf("Title");
    while (query.next())
    {
        QString poolId(query.value(idNo).toString());
        pools.insert(poolId,new Pool(poolId,query.value(animeNo).toString(),query.value(epNo).toString()));
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
        Pool *pool = pools.value(query.value(s_pidNo).toString(),nullptr);
        Q_ASSERT(pool);
        DanmuSourceInfo srcInfo;
        srcInfo.id=query.value(s_idNo).toInt();
        srcInfo.url=query.value(s_urlNo).toString();
        srcInfo.name=query.value(s_nameNo).toString();
        srcInfo.count=0;
        srcInfo.delay=query.value(s_delayNo).toInt();
        srcInfo.show=true;
        srcInfo.setTimeline(query.value(s_timelineNo).toString());
        pool->sourcesTable.insert(srcInfo.id, srcInfo);
    }
}

void DanmuManager::refreshCache(Pool *newPool)
{
    QMutexLocker locker(&cacheLock);
    if(newPool)
    {
        if(poolDanmuCacheInfo.contains(newPool->pid))
        {
            if(poolDanmuCacheInfo.value(newPool->pid)<8)
                ++poolDanmuCacheInfo[newPool->pid];
        }
        else
        {
            poolDanmuCacheInfo[newPool->pid]=4;
        }
    }
    for(auto iter=poolDanmuCacheInfo.begin();iter!=poolDanmuCacheInfo.end();)
    {
        if(iter.value()==0)
        {
            Pool *pool=pools.value(iter.key(),nullptr);
            Q_ASSERT(pool);
            if(!pool->used && pool->clean()) iter=poolDanmuCacheInfo.erase(iter);
			else ++iter;
        }
        else
        {
            --iter.value();
            ++iter;
        }
    }
}

void DanmuManager::deletePool(const QString &pid)
{
    QMutexLocker locker(&removeLock);
    Pool *pool=pools.value(pid,nullptr);
    if(pool)
    {
        if(poolDanmuCacheInfo.contains(pid))
        {
            QMutexLocker locker(&cacheLock);
            poolDanmuCacheInfo.remove(pid);
        }
        pools.remove(pid);
        delete pool;
    }

}

void DanmuManager::updateSourceTimeline(const QString &pid, const DanmuSourceInfo *sourceInfo)
{
    ThreadTask task(GlobalObjects::workThread);
    DanmuSourceInfo srcInfo(*sourceInfo);
    task.RunOnce([pid, srcInfo](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
        query.prepare("update source set TimeLine= ? where PoolID=? and ID=?");
        query.bindValue(0,srcInfo.getTimelineStr());
        query.bindValue(1,pid);
        query.bindValue(2,srcInfo.id);
        query.exec();
    });

}

void DanmuManager::loadPool(Pool *pool)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([pool](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Comment_DB));
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

void DanmuManager::updatePool(Pool *pool, QList<DanmuComment *> &outList, int sourceId)
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

void DanmuManager::saveSource(const QString &pid, const DanmuSourceInfo *source, const QList<QSharedPointer<DanmuComment> > &danmuList)
{
    ThreadTask task(GlobalObjects::workThread);
    DanmuSourceInfo src;
    if(source!=nullptr) src=*source;
    task.RunOnce([pid,src,source,danmuList](){
        QSqlDatabase db = GlobalObjects::getDB(GlobalObjects::Comment_DB);
        QSqlQuery query(db);
        db.transaction();
        if(source)
        {
            query.prepare("insert into source(PoolID,ID,Name,Delay,URL,TimeLine) values(?,?,?,?,?,?)");
            query.bindValue(0,pid);
            query.bindValue(1,src.id);
            query.bindValue(2,src.name);
            query.bindValue(3,src.delay);
            query.bindValue(4,src.url);
            query.bindValue(5,src.getTimelineStr());
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

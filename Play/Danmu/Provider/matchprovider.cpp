#include "matchprovider.h"
#include "globalobjects.h"
#include "Common/network.h"
#include <QSqlQuery>
#include <QSqlRecord>

MatchWorker *MatchProvider::matchWorker=nullptr;

MatchInfo *MatchProvider::SearchFormDandan(QString keyword)
{
    if(!matchWorker)
    {
        matchWorker=new MatchWorker;
        matchWorker->moveToThread(GlobalObjects::workThread);
    }
    QMetaObject::invokeMethod(matchWorker,"beginSearch",Qt::QueuedConnection,Q_ARG(QString,keyword));
    QEventLoop eventLoop;
    MatchInfo *curSearchInfo(nullptr);
    QObject::connect(matchWorker,&MatchWorker::searchDone, &eventLoop, [&eventLoop,&curSearchInfo](MatchInfo *searchInfo){
        curSearchInfo=searchInfo;
        eventLoop.quit();
    });
    eventLoop.exec();
    return curSearchInfo;
}

MatchInfo *MatchProvider::SerchFromDB(QString keyword)
{
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare("select AnimeTitle,Title from bangumi where AnimeTitle like ? or Title like ?");
    keyword=QString("%%1%").arg(keyword);
    query.bindValue(0,keyword);
    query.bindValue(1,keyword);
    query.exec();
    MatchInfo *searchInfo=new MatchInfo;
    searchInfo->error=false;
    int animeNo=query.record().indexOf("AnimeTitle"),titleNo=query.record().indexOf("Title");
    while (query.next())
    {
        MatchInfo::DetailInfo detailInfo;
        detailInfo.animeTitle=query.value(animeNo).toString();
        detailInfo.title=query.value(titleNo).toString();
        searchInfo->matches.append(detailInfo);
    }
    return searchInfo;
}

MatchInfo *MatchProvider::MatchFromDandan(QString fileName)
{
    if(!matchWorker)
    {
        matchWorker=new MatchWorker;
        matchWorker->moveToThread(GlobalObjects::workThread);
    }
    QMetaObject::invokeMethod(matchWorker,"beginMatch",Qt::QueuedConnection,Q_ARG(QString,fileName));
    QEventLoop eventLoop;
    MatchInfo *curMatchInfo(nullptr);
    QObject::connect(matchWorker,&MatchWorker::matchDone, &eventLoop, [&eventLoop,&curMatchInfo](MatchInfo *matchInfo){
        curMatchInfo=matchInfo;
        eventLoop.quit();
    });
    eventLoop.exec();
    return curMatchInfo;
}

MatchInfo *MatchProvider::MatchFromDB(QString fileName)
{
    QFile mediaFile(fileName);
    bool ret=mediaFile.open(QIODevice::ReadOnly);
    if(!ret)return nullptr;
    QByteArray file16MB = mediaFile.read(16*1024*1024);
    QByteArray hashData = QCryptographicHash::hash(file16MB,QCryptographicHash::Md5);
    QString hashStr(hashData.toHex());
    return MatchWorker::retrieveInMatchTable(hashStr);
}

QString MatchProvider::updateMatchInfo(QString fileName, MatchInfo *newMatchInfo, const QString cName)
{
    MatchInfo::DetailInfo detailInfo=newMatchInfo->matches.first();
    QString poolID=addToBangumiTable(detailInfo.animeTitle,detailInfo.title);
    QFile mediaFile(fileName);
    bool ret=mediaFile.open(QIODevice::ReadOnly);
    if(!ret)return poolID;
    QByteArray file16MB = mediaFile.read(16*1024*1024);
    QByteArray hashData = QCryptographicHash::hash(file16MB,QCryptographicHash::Md5);
    QString hashStr(hashData.toHex());
    addToMatchTable(hashStr,poolID,true,cName);
    return poolID;
}

void MatchProvider::addToMatchTable(QString fileHash, QString poolID, bool replace, const QString cName)
{
    QSqlQuery query(QSqlDatabase::database(cName));
    query.exec(QString("select * from match where MD5='%1'").arg(fileHash));
    if(query.first())
    {
        if(replace)
            query.exec(QString("delete from match where MD5='%1'").arg(fileHash));
        else
            return;
    }
    query.prepare("insert into match(MD5,PoolID) values(?,?)");
    query.bindValue(0,fileHash);
    query.bindValue(1,poolID);
    query.exec();
}

QString MatchProvider::addToBangumiTable(QString animeTitle, QString title, const QString cName)
{
    QByteArray hashData = QString("%1-%2").arg(animeTitle).arg(title).toUtf8();
    QString poolID = QString(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex());
    QSqlQuery query(QSqlDatabase::database(cName));
    query.exec(QString("select * from bangumi where PoolID='%1'").arg(poolID));
    if(query.first())return poolID;
    query.prepare("insert into bangumi(PoolID,AnimeTitle,Title) values(?,?,?)");
    query.bindValue(0,poolID);
    query.bindValue(1,animeTitle);
    query.bindValue(2,title);
    query.exec();
    return poolID;
}

void MatchWorker::handleMatchReply(QJsonDocument &document, MatchInfo *matchInfo)
{
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
        for(auto iter=detailInfoArray.begin();iter!=detailInfoArray.end();iter++)
        {
            if(!(*iter).isObject())continue;
            QJsonObject detailObj=(*iter).toObject();
            QJsonValue animeTitle=detailObj.value("animeTitle");
            if(animeTitle.type()!=QJsonValue::String)continue;
            QJsonValue episodeTitle=detailObj.value("episodeTitle");
            if(episodeTitle.type()!=QJsonValue::String)continue;
            MatchInfo::DetailInfo detailInfo;
            detailInfo.animeTitle=animeTitle.toString();
            detailInfo.title=episodeTitle.toString();
            matchInfo->matches.append(detailInfo);
        }
        matchInfo->error = false;
        if(matchInfo->success && matchInfo->matches.count()>0)
        {
            matchInfo->poolID=MatchProvider::addToBangumiTable(matchInfo->matches.first().animeTitle,matchInfo->matches.first().title,"WT");
            MatchProvider::addToMatchTable(matchInfo->fileHash,matchInfo->poolID,"WT");
        }
        return;
    }while(false);
    matchInfo->error=true;
    matchInfo->errorInfo=QObject::tr("Reply JSON Format Error");
    return;
}

void MatchWorker::handleSearchReply(QJsonDocument &document, MatchInfo *searchInfo)
{
    do
    {
        if (!document.isObject()) break;
        QJsonObject obj = document.object();
        QJsonValue success = obj.value("success");
        if (success.type() != QJsonValue::Bool)break;
        if (!success.toBool())
        {
            searchInfo->error = false;
            return;
        }
        QJsonValue animes=obj.value("animes");
        if(animes.type()!=QJsonValue::Array) break;
        QJsonArray animeArray=animes.toArray();
        for(auto animeIter=animeArray.begin();animeIter!=animeArray.end();animeIter++)
        {
            if(!(*animeIter).isObject())continue;
            QJsonObject animeObj=(*animeIter).toObject();
            QJsonValue animeTitle=animeObj.value("animeTitle");
            if(animeTitle.type()!=QJsonValue::String)continue;
            QString animeTitleStr=animeTitle.toString();
            QJsonValue episodes= animeObj.value("episodes");
            if(episodes.type()!=QJsonValue::Array) continue;
            QJsonArray episodeArray=episodes.toArray();
            for(auto episodeIter=episodeArray.begin();episodeIter!=episodeArray.end();episodeIter++)
            {
                if(!(*episodeIter).isObject())continue;
                QJsonObject episodeObj=(*episodeIter).toObject();
                QJsonValue episodeTitle=episodeObj.value("episodeTitle");
                if(episodeTitle.type()!=QJsonValue::String)continue;
                MatchInfo::DetailInfo detailInfo;
                detailInfo.animeTitle=animeTitleStr;
                detailInfo.title=episodeTitle.toString();
                searchInfo->matches.append(detailInfo);
            }
        }
        searchInfo->error = false;
        return;
    }while(false);
    searchInfo->error=true;
    searchInfo->errorInfo=QObject::tr("Reply JSON Format Error");
}

MatchInfo *MatchWorker::retrieveInMatchTable(QString fileHash, const QString cName)
{
    QSqlQuery query(QSqlDatabase::database(cName));
    query.exec(QString("select poolID from match where MD5='%1'").arg(fileHash));
    if(!query.first())return nullptr;
    QString poolID=query.value(0).toString();
    query.exec(QString("select AnimeTitle,Title from bangumi where PoolID='%1'").arg(poolID));
    if(!query.first())return nullptr;
    QString animeTitle=query.value("AnimeTitle").toString(),
            title=query.value("Title").toString();
    MatchInfo *matchInfo=new MatchInfo;
    matchInfo->error=false;
    matchInfo->fileHash=fileHash;
    matchInfo->poolID=poolID;
    matchInfo->success=true;
    MatchInfo::DetailInfo detailInfo;
    detailInfo.title=title;
    detailInfo.animeTitle=animeTitle;
    matchInfo->matches.append(detailInfo);
    return matchInfo;
}

void MatchWorker::beginMatch(QString fileName)
{
    QFile mediaFile(fileName);
    bool ret=mediaFile.open(QIODevice::ReadOnly);
    if(!ret)
    {
        emit matchDone(nullptr);
        return;
    }
    QByteArray file16MB = mediaFile.read(16*1024*1024);
    QByteArray hashData = QCryptographicHash::hash(file16MB,QCryptographicHash::Md5);
    QString hashStr(hashData.toHex());

    MatchInfo *localMatchInfo=retrieveInMatchTable(hashStr,"WT");
    if(localMatchInfo)
    {
        emit matchDone(localMatchInfo);
        return;
    }
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
        handleMatchReply(document,matchInfo);
    }
    catch(Network::NetworkError &error)
    {
        matchInfo->error=true;
        matchInfo->errorInfo=error.errorInfo;
    }
   // request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    emit matchDone(matchInfo);
}

void MatchWorker::beginSearch(QString keyword)
{
    QString baseUrl = "https://api.acplay.net/api/v2/search/episodes";
    QUrl url(baseUrl);
    QUrlQuery query;
    query.addQueryItem("anime", keyword);
    url.setQuery(query);
    MatchInfo *searchInfo=new MatchInfo;
    try
    {
        QString str(Network::httpGet(baseUrl,query));
        QJsonDocument document(Network::toJson(str));
        handleSearchReply(document,searchInfo);
    }
    catch(Network::NetworkError &error)
    {
        searchInfo->error=true;
        searchInfo->errorInfo=error.errorInfo;
    }
    emit searchDone(searchInfo);
}

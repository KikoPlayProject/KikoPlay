#include "dandanprovider.h"
#include "Common/network.h"

DanmuAccessResult *DandanProvider::search(const QString &keyword)
{
    QString baseUrl = "https://api.acplay.net/api/v2/search/episodes";
    QUrlQuery query;
    query.addQueryItem("anime", keyword);
    DanmuAccessResult *searchResult=new DanmuAccessResult;
    searchResult->providerId=id();
    try
    {
        QString str(Network::httpGet(baseUrl,query,QStringList()<<"Accept"<<"application/json"));
        QJsonDocument document(Network::toJson(str));
        handleSearchReply(document,searchResult);
    }
    catch(Network::NetworkError &error)
    {
        searchResult->error=true;
        searchResult->errorInfo=error.errorInfo;
    }
    emit searchDone(searchResult);
    return searchResult;
}

DanmuAccessResult *DandanProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    result->error=false;
    result->list.append(*item);
    emit epInfoDone(result);
    return result;
}

QString DandanProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString baseUrl = QString("https://api.acplay.net/api/v2/comment/%1").arg(item->id);
    QString errorInfo;
    try
    {
        QString str(Network::httpGet(baseUrl,QUrlQuery(),QStringList()<<"Accept"<<"application/json"));
        QJsonDocument document(Network::toJson(str));
        handleDownloadReply(document,danmuList);

    }
    catch(Network::NetworkError &error)
    {
        errorInfo=error.errorInfo;
    }
    emit downloadDone(errorInfo);
    return errorInfo;
}

QString DandanProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    int epid=url.mid(url.lastIndexOf(':')+1).toInt();
    DanmuSourceItem item;
    item.id=epid;
    return downloadDanmu(&item,danmuList);
}

void DandanProvider::handleSearchReply(QJsonDocument &document, DanmuAccessResult *result)
{
    do
    {
        if (!document.isObject()) break;
        QJsonObject obj = document.object();
        QJsonValue success = obj.value("success");
        if (success.type() != QJsonValue::Bool)break;
        if (!success.toBool())
        {
            result->error = false;
            return;
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
            QString animeTitleStr=animeTitle.toString();
            QJsonValue episodes= animeObj.value("episodes");
            if(episodes.type()!=QJsonValue::Array) continue;
            QJsonArray episodeArray=episodes.toArray();
            for(auto episodeIter=episodeArray.begin();episodeIter!=episodeArray.end();++episodeIter)
            {
                if(!(*episodeIter).isObject())continue;
                QJsonObject episodeObj=(*episodeIter).toObject();
                QJsonValue episodeTitle=episodeObj.value("episodeTitle");
                if(episodeTitle.type()!=QJsonValue::String)continue;
                QJsonValue episodeId=episodeObj.value("episodeId");
                if(episodeId.type()!=QJsonValue::Double)continue;
                DanmuSourceItem item;
                item.danmuCount=-1;
                item.title=episodeTitle.toString();
                item.description=animeTitleStr;
                item.id=episodeId.toInt();
                //item.source=DanmuSource::Dandan;
                item.extra=0;
                item.delay=0;
                result->list.append(item);
            }
        }
        result->error = false;
        return;
    }while(false);
    result->error=true;
    result->errorInfo=tr("Reply JSON Format Error");
}

void DandanProvider::handleDownloadReply(QJsonDocument &document, QList<DanmuComment *> &danmuList)
{
    do
    {
        if (!document.isObject()) break;
        QJsonObject obj = document.object();
        QJsonValue comments=obj.value("comments");
        if(comments.type()!=QJsonValue::Array) break;
        QJsonArray comentArray=comments.toArray();
        for(auto comentIter=comentArray.begin();comentIter!=comentArray.end();++comentIter)
        {
            if(!(*comentIter).isObject())continue;
            QJsonObject comentObj=(*comentIter).toObject();
            QJsonValue p=comentObj.value("p");
            if(p.type()!=QJsonValue::String) continue;
            QStringList ps=p.toString().split(',');
            if(ps.count()<4)continue;
            QJsonValue m=comentObj.value("m");
            if(m.type()!=QJsonValue::String) continue;

            DanmuComment *danmu=new DanmuComment();
            danmu->text=m.toString();
            danmu->time =ps[0].toFloat() * 1000;
            danmu->setType(ps[1].toInt());
            danmu->color=ps[2].toInt();
            danmu->date=0;
            danmu->sender="[Dandan]"+ps[3];
            danmu->blockBy=-1;
            danmu->fontSizeLevel=DanmuComment::Normal;
            if(danmu->type!=DanmuComment::UNKNOW)danmuList.append(danmu);
            else delete danmu;
        }

    }while(false);
}


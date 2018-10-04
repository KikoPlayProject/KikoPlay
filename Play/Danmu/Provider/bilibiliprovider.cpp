#include "bilibiliprovider.h"
#include "Common/network.h"
namespace
{
    const char *supportedUrlRe[]={"(https?://)?www\\.bilibili\\.com/video/av[0-9]+/?",
                                  "av[0-9]+"};
}

QStringList BilibiliProvider::supportedURLs()
{
    return QStringList({"https://www.bilibili.com/video/av1728704",
                        "av24213033"});
}

DanmuAccessResult *BilibiliProvider::search(const QString &keyword)
{
    QString baseUrl = "https://api.bilibili.com/x/web-interface/search/all";
    QUrlQuery query;
    query.addQueryItem("keyword", keyword);
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

DanmuAccessResult *BilibiliProvider::getEpInfo(DanmuSourceItem *item)
{
    bool fromBangumi=(item->extra==1);
    QString baseUrl = fromBangumi?"https://bangumi.bilibili.com/view/web_api/season":
                                    "https://api.bilibili.com/view";
    QUrlQuery query;
    if(fromBangumi)
    {
        query.addQueryItem("media_id", QString::number(item->id));
    }
    else
    {
        query.addQueryItem("appkey", "8e9fc618fbd41e28");
        query.addQueryItem("id", QString::number(item->id));
    }
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    try
    {
        QString str(Network::httpGet(baseUrl,query,QStringList()<<"Accept"<<"application/json"));
        QJsonDocument document(Network::toJson(str));
        if(fromBangumi)
            handleBangumiReply(document,result);
        else
            handleViewReply(document,result,item->id);
    }
    catch(Network::NetworkError &error)
    {
        result->error=true;
        result->errorInfo=error.errorInfo;
    }
    emit epInfoDone(result,item);
    return result;
}

DanmuAccessResult *BilibiliProvider::getURLInfo(const QString &url)
{
    int reCount=sizeof(supportedUrlRe)/sizeof(const char *);
    int matchIndex=0;
    for(;matchIndex<reCount;++matchIndex)
    {
        QRegExp re(supportedUrlRe[matchIndex]);
        re.indexIn(url);
        if(re.matchedLength()==url.length())
            break;
    }
    if(matchIndex==reCount)
    {
        return nullptr;
    }
    QRegExp re("av([0-9]+)");
    re.indexIn(url);
    QStringList captured=re.capturedTexts();
    QString aidStr = captured[1];
    DanmuSourceItem item;
    item.id=aidStr.toInt();
    item.subId=0;
    //item.source=DanmuSource::Bilibili;
    item.extra=0;
    return getEpInfo(&item);
}

QString BilibiliProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString errInfo;
    try
    {
        QString replyStr(Network::httpGet(QString("http://comment.bilibili.com/%1.xml").arg(item->subId),QUrlQuery()));
        handleDownloadReply(replyStr,danmuList);
    }
    catch(Network::NetworkError &error)
    {
        errInfo = error.errorInfo;
    }
    emit downloadDone(errInfo);
    return errInfo;
}

QString BilibiliProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    int cid=url.mid(url.lastIndexOf(':')+1).toInt();
    DanmuSourceItem item;
    item.subId=cid;
    return downloadDanmu(&item,danmuList);
}

void BilibiliProvider::handleSearchReply(QJsonDocument &document, DanmuAccessResult *searchResult)
{
    if (!document.isObject())
    {
        searchResult->error=true;
        searchResult->errorInfo=tr("Reply JSON Format Error");
        return;
    }
    QJsonObject obj = document.object();
    QJsonValue bangumiResult(Network::getValue(obj,"data/result/media_bangumi")),
            videoResult(Network::getValue(obj,"data/result/video"));
    if(bangumiResult.type()==QJsonValue::Array)
    {
        QJsonArray bangumiArray=bangumiResult.toArray();
        for(auto bangumiIter=bangumiArray.begin();bangumiIter!=bangumiArray.end();++bangumiIter)
        {
            if(!(*bangumiIter).isObject())continue;
            QJsonObject bangumiObj=(*bangumiIter).toObject();
            QJsonValue title=bangumiObj.value("title");
            if(title.type()!=QJsonValue::String)continue;
            QJsonValue desc=bangumiObj.value("desc");
            if(desc.type()!=QJsonValue::String)continue;
            QJsonValue media_id=bangumiObj.value("media_id");
            if(media_id.type()!=QJsonValue::Double)continue;
            DanmuSourceItem item;
            item.danmuCount=-1;
            item.title=title.toString().replace(QRegExp("<([^<>]*)em([^<>]*)>"), "");
            item.description=desc.toString();
            item.id=media_id.toInt();
            //item.source=DanmuSource::Bilibili;
            item.extra=1;//from bangumi
            item.delay=0;
            searchResult->list.append(item);
        }
    }
    if(videoResult.type()==QJsonValue::Array)
    {
        QJsonArray videoArray=videoResult.toArray();
        for(auto videoIter=videoArray.begin();videoIter!=videoArray.end();++videoIter)
        {
            if(!(*videoIter).isObject())continue;
            QJsonObject bangumiObj=(*videoIter).toObject();
            QJsonValue title=bangumiObj.value("title");
            if(title.type()!=QJsonValue::String)continue;
            QJsonValue desc=bangumiObj.value("description");
            if(desc.type()!=QJsonValue::String)continue;
            QJsonValue aid=bangumiObj.value("id");
            if(aid.type()!=QJsonValue::Double)continue;
            DanmuSourceItem item;
            item.danmuCount=-1;
            item.title=title.toString().replace(QRegExp("<([^<>]*)em([^<>]*)>"),"");
            item.description=desc.toString();
            item.id=aid.toInt();
            //item.source=DanmuSource::Bilibili;
            item.extra=0;//from video
            item.delay=0;
            searchResult->list.append(item);
        }
    }
    searchResult->error = false;
}

void BilibiliProvider::handleBangumiReply(QJsonDocument &document, DanmuAccessResult *result)
{
    if (!document.isObject())
    {
		result->error=true;
		result->errorInfo=tr("Reply JSON Format Error");
        return;
    }
    QJsonObject obj = document.object();
    QJsonValue bangumiResult(Network::getValue(obj,"result/episodes"));
    if(bangumiResult.type()==QJsonValue::Array)
    {
        QJsonArray bangumiArray=bangumiResult.toArray();
        for(auto bangumiIter=bangumiArray.begin();bangumiIter!=bangumiArray.end();++bangumiIter)
        {
            if(!(*bangumiIter).isObject())continue;
            QJsonObject bangumiObj=(*bangumiIter).toObject();
            QJsonValue title=bangumiObj.value("index_title");
            if(title.type()!=QJsonValue::String)continue;
            QJsonValue index=bangumiObj.value("index");
            if(index.type()!=QJsonValue::String)continue;
            QJsonValue cid=bangumiObj.value("cid");
            if(cid.type()!=QJsonValue::Double)continue;
            QJsonValue aid=bangumiObj.value("aid");
            if(aid.type()!=QJsonValue::Double)continue;
            DanmuSourceItem item;
            item.danmuCount=-1;
            item.title=QString("%1-%2").arg(index.toString()).arg(title.toString());
            item.id=aid.toInt();
            item.subId=cid.toInt();
            //item.source=DanmuSource::Bilibili;
            item.extra=0;
            item.delay=0;
            result->list.append(item);
        }
    }
    result->error = false;
}

void BilibiliProvider::handleViewReply(QJsonDocument &document, DanmuAccessResult *result, int aid)
{
    do
    {
        if (!document.isObject()) break;
        QJsonObject obj = document.object();
        QJsonValue pages=obj.value("pages");
        if(pages.type()!=QJsonValue::Double)break;
        int pageCount=pages.toInt();
        if(pageCount==1)
        {
            QJsonValue title=obj.value("title");
            if(title.type()!=QJsonValue::String)continue;
            QJsonValue cid=obj.value("cid");
            if(cid.type()!=QJsonValue::Double)continue;
            DanmuSourceItem item;
            item.danmuCount=-1;
            item.title=title.toString();
            item.id=aid;
            item.subId=cid.toInt();
            //item.source=DanmuSource::Bilibili;
            item.extra=0;
            item.delay=0;
            result->list.append(item);
        }
        else
        {
            try
            {
                QString replyStr(Network::httpGet(QString("https://www.bilibili.com/video/av%1").arg(aid),QUrlQuery(),
					QStringList()<<"User-Agent"<<"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:62.0) Gecko/20100101 Firefox/62.0"));
                QRegExp re("(\"videoData\":)(.*)(,\"upData\")");
                int pos=re.indexIn(replyStr);
                if(pos!=-1)
                {
                    QStringList list = re.capturedTexts();
                    decodeVideoList(list.at(2).toUtf8(),result,aid);
                }
                else
                {
                    re.setPattern("(\"epList\":)(.*)(,\"newestEp\")");
                    pos=re.indexIn(replyStr);
                    if(pos!=-1)
                    {
                        QStringList list = re.capturedTexts();
                        decodeEpList(list.at(2).toUtf8(),result,aid);
                    }
                }
            }
            catch(Network::NetworkError &error)
            {
                result->error=true;
                result->errorInfo=error.errorInfo;
            }
        }
        result->error = false;
        return;
    }while(false);
    result->error=true;
    result->errorInfo=QObject::tr("Reply JSON Format Error");
}

void BilibiliProvider::decodeVideoList(QByteArray &bytes, DanmuAccessResult *result, int aid)
{
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(bytes, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) return;
    if (!document.isObject()) return;
    QJsonObject obj = document.object();
    if (obj.contains("pages") && obj["pages"].isArray())
    {
        QJsonArray pageArray = obj["pages"].toArray();
        for (auto p = pageArray.begin(); p != pageArray.end(); ++p)
        {
            if (!(*p).isObject())continue;
            QJsonObject pObj = (*p).toObject();
            QJsonValue title = pObj["part"];
            if (title.type() != QJsonValue::String)continue;
            QJsonValue duration = pObj["duration"];
            if (duration.type() != QJsonValue::Double)continue;
            QJsonValue cid = pObj["cid"];
            if (cid.type() != QJsonValue::Double)continue;
            DanmuSourceItem item;
            item.danmuCount = -1;
            item.title = title.toString();
            item.id=aid;
            item.subId = cid.toInt();
            //item.source = DanmuSource::Bilibili;
            item.extra = duration.toInt();
            result->list.append(item);
        }
    }
}

void BilibiliProvider::decodeEpList(QByteArray &bytes, DanmuAccessResult *result, int aid)
{
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(bytes, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) return;
    if (!document.isArray()) return;
    QJsonArray epArray=document.array();
    for(auto p=epArray.begin();p!=epArray.end();++p)
    {
        if(!(*p).isObject())continue;
        QJsonObject pObj=(*p).toObject();
        QJsonValue title=pObj["index_title"];
        if(title.type()!=QJsonValue::String)continue;
        QJsonValue cid=pObj["cid"];
        if(cid.type()!=QJsonValue::Double)continue;
        DanmuSourceItem item;
        item.danmuCount=-1;
        item.title=title.toString();
        item.id=aid;
        item.subId=cid.toInt();
        //item.source=DanmuSource::Bilibili;
        item.extra=0;
        result->list.append(item);
    }
}

void BilibiliProvider::handleDownloadReply(QString &reply, QList<DanmuComment *> &danmuList)
{
    QXmlStreamReader reader(reply);
    while(!reader.atEnd())
    {
        if(reader.isStartElement() && reader.name()=="d")
        {
            QXmlStreamAttributes attributes=reader.attributes();
            if(attributes.hasAttribute("p"))
            {
                QStringList attrList=attributes.value("p").toString().split(',');
                if(attrList.length()<8)continue;
                DanmuComment *danmu=new DanmuComment();
                danmu->text=reader.readElementText();
                danmu->time = attrList[0].toFloat() * 1000;
                danmu->originTime=danmu->time;
                danmu->setType(attrList[1].toInt());
                danmu->color=attrList[3].toInt();
                danmu->date=attrList[4].toLongLong();
                danmu->sender="[Bilibili]"+attrList[6];
                switch (attrList[2].toInt())
                {
                case 25:
                    danmu->fontSizeLevel=DanmuComment::Normal;
                    break;
                case 18:
                    danmu->fontSizeLevel=DanmuComment::Small;
                    break;
                case 36:
                    danmu->fontSizeLevel=DanmuComment::Large;
                    break;
                default:
                    break;
                }
                if(danmu->type!=DanmuComment::UNKNOW)danmuList.append(danmu);
                else
                {
#ifdef QT_DEBUG
                    qDebug()<<"unsupport danmu mode:"<<danmu->text;
#endif
                    delete danmu;
                }
            }
        }
        reader.readNext();
    }
}

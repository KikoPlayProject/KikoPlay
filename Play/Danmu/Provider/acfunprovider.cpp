#include "acfunprovider.h"
#include "Common/network.h"
namespace
{
    const char *supportedUrlRe[]={"(http://)?www\\.acfun\\.cn/v/ac[0-9]+(_[0-9]+)?/?",
                                  "(http://)?www\\.acfun\\.cn/bangumi/ab[0-9]+_[0-9]+_[0-9]+/?"};
}
QStringList AcfunProvider::supportedURLs()
{
    return QStringList({"http://www.acfun.cn/v/ac4471456",
                        "http://www.acfun.cn/bangumi/ab5020318_29434_234123"});
}

DanmuAccessResult *AcfunProvider::search(const QString &keyword)
{
    QString baseUrl = "http://search.aixifan.com/search";
    QUrlQuery query;
    query.addQueryItem("q", keyword);
    query.addQueryItem("isArticle", "1");
    query.addQueryItem("pageSize", "20");
    query.addQueryItem("pageNo", "1");
    query.addQueryItem("aiCount", "3");
    query.addQueryItem("spCount", "3");
    query.addQueryItem("sortField", "score");
    DanmuAccessResult *searchResult=new DanmuAccessResult;
    searchResult->providerId=id();
    try
    {
        QString str(Network::httpGet(baseUrl,query));
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

DanmuAccessResult *AcfunProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    switch (item->subId)
    {
    case 1:
        handleSp(item,result);
        break;
    case 2:
        handleAi(item,result);
        break;
    default:
        result->list.append(*item);
        result->error=false;
        break;
    }
    emit epInfoDone(result,item);
    return result;
}

DanmuAccessResult *AcfunProvider::getURLInfo(const QString &url)
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
    DanmuSourceItem item;
    QRegExp re(matchIndex==0?"ac[0-9]+(_[0-9]+)?":"ab[0-9]+_[0-9]+_[0-9]+");
    re.indexIn(url);
    item.subId=matchIndex==0?3:4;
    item.strId=re.capturedTexts()[0];
    item.title=item.strId;
    item.extra=0;

    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    result->error=false;
    result->list.append(item);
    return result;
}

QString AcfunProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString baseUrl;
    QString errInfo;
    switch (item->subId)
    {
    case 3:
        baseUrl=QString("http://www.acfun.cn/v/%1").arg(item->strId);
        break;
    case 4:
        baseUrl=QString("http://www.acfun.cn/bangumi/%1").arg(item->strId);
        break;
    default:
        errInfo = tr("Decode Failed");
        emit downloadDone(errInfo,item);
        return errInfo;
    }
	try
	{
		QString replyStr(Network::httpGet(baseUrl, QUrlQuery()));
		QRegExp re("\"videoId\":([0-9]+)");
		int pos = re.indexIn(replyStr);
		if (pos == -1)
		{
			errInfo = tr("Decode Failed");
		}
		else
		{
			QStringList captured = re.capturedTexts();
			item->id = captured[1].toInt();
			downloadAllDanmu(danmuList, item->id);
		}
	}
	catch (Network::NetworkError &error)
	{
		errInfo = error.errorInfo;
	}
    emit downloadDone(errInfo,item);
    return errInfo;
}

QString AcfunProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    QString videoId=url.mid(url.lastIndexOf(':')+1);
    downloadAllDanmu(danmuList,videoId.toInt());
    return QString();
}

void AcfunProvider::handleSearchReply(QJsonDocument &document, DanmuAccessResult *searchResult)
{
    if (!document.isObject())
    {
        searchResult->error=true;
        searchResult->errorInfo=tr("Reply JSON Format Error");
        return;
    }
    QJsonObject obj = document.object();
    QJsonValue spResult(Network::getValue(obj,"data/page/sp")),
               aiResult(Network::getValue(obj,"data/page/ai")),
               listResult(Network::getValue(obj,"data/page/list"));
    if(spResult.type()==QJsonValue::Array)
    {
        QJsonArray spArray=spResult.toArray();
        for(auto spIter=spArray.begin();spIter!=spArray.end();++spIter)
        {
            if(!(*spIter).isObject())continue;
            QJsonObject spObj=(*spIter).toObject();
            QJsonValue contentId=spObj.value("contentId");
            if(contentId.type()!=QJsonValue::String)continue;
            QJsonValue title=spObj.value("title");
            if(title.type()!=QJsonValue::String)continue;
            QJsonValue desc=spObj.value("description");
            if(desc.type()!=QJsonValue::String)continue;
            DanmuSourceItem item;
            item.danmuCount=-1;
            item.title=title.toString();
            item.description=desc.toString();
            item.strId=contentId.toString();
            item.subId=1;//sp result
            item.extra=0;
            searchResult->list.append(item);
        }
    }
    if(aiResult.type()==QJsonValue::Array)
    {
        QJsonArray aiArray=aiResult.toArray();
        for(auto aiIter=aiArray.begin();aiIter!=aiArray.end();++aiIter)
        {
            if(!(*aiIter).isObject())continue;
            QJsonObject aiObj=(*aiIter).toObject();
            QJsonValue title=aiObj.value("title");
            if(title.type()!=QJsonValue::String)continue;
            QJsonValue desc=aiObj.value("description");
            if(desc.type()!=QJsonValue::String)continue;
            DanmuSourceItem item;
            item.danmuCount=-1;
            item.title=title.toString();
            item.description=desc.toString();
            item.strId=QJsonDocument(aiObj).toJson(QJsonDocument::Compact);
            item.subId=2;//ai result
            item.extra=0;
            searchResult->list.append(item);
        }
    }
    if(listResult.type()==QJsonValue::Array)
    {
        QJsonArray listArray=listResult.toArray();
        for(auto listIter=listArray.begin();listIter!=listArray.end();++listIter)
        {
            if(!(*listIter).isObject())continue;
            QJsonObject listObj=(*listIter).toObject();
            QJsonValue title=listObj.value("title");
            if(title.type()!=QJsonValue::String)continue;
            QJsonValue desc=listObj.value("description");
            if(desc.type()!=QJsonValue::String)continue;
            QJsonValue contentId= listObj.value("contentId");
            if(contentId.type()!=QJsonValue::String)continue;
            DanmuSourceItem item;
            item.danmuCount=-1;
            item.title=title.toString();
            item.description=desc.toString();
            item.strId=contentId.toString();
            item.subId=3;//list result acNo
            item.extra=0;
            searchResult->list.append(item);
        }
    }
    searchResult->error = false;
}

void AcfunProvider::handleSp(DanmuSourceItem *item, DanmuAccessResult *result, int pageNo)
{
    //item->strId: aa[0-9]+
    QString baseUrl="http://www.acfun.cn/member/special/getSpecialContentPageBySpecial.aspx";
    QByteArray data(QString("pageNo=%1&pageSize=20&specialId=%2").arg(pageNo).arg(item->strId.mid(2)).toUtf8());
    try
    {
        QString replyStr(Network::httpPost(baseUrl,data));
        QJsonDocument document(Network::toJson(replyStr));
        QJsonObject obj = document.object();
        QJsonValue specialContents=obj.value("specialContents");
        if(specialContents.isArray())
        {
            QJsonArray contentsArray=specialContents.toArray();
            for(auto cIter=contentsArray.begin();cIter!=contentsArray.end();++cIter)
            {
                if(!(*cIter).isObject())continue;
                QJsonObject cObj=(*cIter).toObject();
                QJsonValue title=cObj.value("title");
                if(title.type()!=QJsonValue::String)continue;
                QJsonValue id=cObj.value("id"); //ac
                if(id.type()!=QJsonValue::String)continue;
                DanmuSourceItem item;
                item.subId=3;//acNo
                item.title=title.toString();
                item.strId="ac"+id.toString();
                item.extra=0;
                result->list.append(item);
            }
        }
        QJsonValue totalPage(Network::getValue(obj,"page/totalPage"));
        if(totalPage.isDouble())
        {
            int tp=totalPage.toInt();
            if(pageNo<tp)
            {
                handleSp(item,result,pageNo+1);
            }
        }
    }
    catch(Network::NetworkError &error)
    {
        result->error=true;
        result->errorInfo=error.errorInfo;
        return;
    }
    result->error=false;
}

void AcfunProvider::handleAi(DanmuSourceItem *item, DanmuAccessResult *result)
{
    try
    {
        QJsonDocument document(Network::toJson(item->strId));
        QJsonObject obj = document.object();
        QString contentId=obj.value("contentId").toString();
        QJsonArray epNames(obj.value("episodeName").toArray()),
                groupIds(obj.value("groupIds").toArray()),
                videoIds(obj.value("videoIds").toArray());
        if(epNames.count()!=groupIds.count()||epNames.count()!=videoIds.count())
        {
            throw Network::NetworkError(QObject::tr("Decode JSON Failed"));
        }
        for(int i=0;i<epNames.count();++i)
        {
            QString title=epNames.at(i).toString();
            QString groupId=groupIds.at(i).toString();
            QString videoId=videoIds.at(i).toString();
            DanmuSourceItem item;
            item.subId=4;//bgmNo ab
            item.title=title;
            item.strId=QString("ab%1_%2_%3").arg(contentId).arg(groupId).arg(videoId);
            item.extra=0;
            result->list.append(item);
        }
    }
    catch(Network::NetworkError &error)
    {
        result->error=true;
        result->errorInfo=error.errorInfo;
    }
    result->error=false;
}

void AcfunProvider::downloadAllDanmu(QList<DanmuComment *> &danmuList, int videoId)
{
    try
    {
        int downloadCount=0;
        QString timeStamp("4073558400000");
        do
        {
            QString url=QString("http://danmu.aixifan.com/V4/%1_2/%2/1000").arg(videoId).arg(timeStamp);
            QUrlQuery query;
            query.addQueryItem("order", "-1");
            QString str(Network::httpGet(url,query));
            QJsonDocument document(Network::toJson(str));
            QJsonArray array=document.array();
            if(array.size()<3)break;
            QJsonArray dmArray=array.at(2).toArray();
            downloadCount=dmArray.count();
            for(auto iter=dmArray.begin();iter!=dmArray.end();++iter)
            {
                QJsonObject dmObj=(*iter).toObject();
                QJsonValue c=dmObj.value("c");
                if(c.type()!=QJsonValue::String) continue;
                QStringList cs=c.toString().split(',');
                if(cs.count()<6)continue;
                QJsonValue m=dmObj.value("m");
                if(m.type()!=QJsonValue::String) continue;

                DanmuComment *danmu=new DanmuComment();
                danmu->text=m.toString();
                danmu->time =cs[0].toFloat() * 1000;
                danmu->originTime=danmu->time;
                danmu->color=cs[1].toInt();
                danmu->setType(cs[2].toInt());
                if(cs[3].toInt()==37)
                    danmu->fontSizeLevel=DanmuComment::Large;
                else
                    danmu->fontSizeLevel=DanmuComment::Normal;
                danmu->sender="[Acfun]"+cs[4];
                danmu->date=cs[5].toLongLong();
                if(danmu->type!=DanmuComment::UNKNOW)danmuList.append(danmu);
                else delete danmu;
            }
            if(danmuList.count()>0)
                timeStamp=QString::number(danmuList.last()->date);

        }while(downloadCount>=1000);
    }
    catch(Network::NetworkError &)
    {
        return;
    }
}

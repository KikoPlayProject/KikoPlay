#include "pptvprovider.h"
#include "Common/htmlparsersax.h"
#include "Common/network.h"
namespace
{
    const char *supportedUrlRe[]={"(https?://)?v\\.pptv\\.com/show/[a-zA-Z0-9/]+\\.html"};
}
QStringList PPTVProvider::supportedURLs()
{
    return QStringList({"http://v.pptv.com/show/iaH4enbIgGlia7OaE.html"});
}

DanmuAccessResult *PPTVProvider::search(const QString &keyword)
{
    QString baseUrl = QString("http://sou.pptv.com/s_video");
    QUrlQuery query;
    query.addQueryItem("kw",keyword);
    DanmuAccessResult *searchResult=new DanmuAccessResult;
    searchResult->providerId=id();
    try
    {
        QString str(Network::httpGet(baseUrl,query));
        handleSearchReply(str,searchResult);
    }
    catch(Network::NetworkError &error)
    {
        searchResult->error=true;
        searchResult->errorInfo=error.errorInfo;
    }
    emit searchDone(searchResult);
    return searchResult;
}

DanmuAccessResult *PPTVProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    result->error=false;
    result->list.append(*item);
    emit epInfoDone(result,item);
    return result;
}

DanmuAccessResult *PPTVProvider::getURLInfo(const QString &url)
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
    item.strId=url;
    return getEpInfo(&item);
}

QString PPTVProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString errInfo;
    try
    {
        QString replyStr(Network::httpGet(item->strId,QUrlQuery()));
        QRegExp re("webcfg.*(\\{.*);");
        re.setMinimal(true);
        bool hasError=true;
        do
        {
            int pos=re.indexIn(replyStr);
            if(pos==-1) break;
            QStringList captured=re.capturedTexts();
            QJsonObject videoInfo(Network::toJson(captured[1]).object());
            item->title=videoInfo.value("title").toString();
            item->extra=videoInfo.value("duration").toInt();
			item->subId = item->extra / 100;
            item->strId=QString::number(videoInfo.value("id").toInt());
            downloadAllDanmu(item->strId,item->subId,danmuList);
            hasError=false;
        }while(false);
        if(hasError) errInfo=tr("Decode Failed");
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadDone(errInfo,item);
    return errInfo;
}

QString PPTVProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    int s=url.indexOf(':')+1,e=url.indexOf(';');
    QString id=url.mid(s,e-s);
    int l=url.mid(url.lastIndexOf(':')+1).toInt();
    downloadAllDanmu(id,l,danmuList);
    return QString();
}

void PPTVProvider::downloadAllDanmu(const QString &id, int length, QList<DanmuComment *> &danmuList)
{
    QString baseUrl=QString("http://danmucdn.api.pptv.com/danmu/v4/pplive/ref/vod_%1/danmu").arg(id);
    QStringList urls;
    QList<QUrlQuery> querys;
    for (int i=0;i<=length;++i)
    {
        urls<<baseUrl;
        QUrlQuery query;
        query.addQueryItem("pos",QString::number(i*1000));
        querys<<query;
    }
    QList<QPair<QString, QByteArray> > results(Network::httpGetBatch(urls,querys));
    for(auto &result:results)
    {
        if(!result.first.isEmpty()) continue;
        QJsonObject obj(Network::toJson(result.second).object().value("data").toObject());
        QJsonArray danmuArray(obj.value("infos").toArray());
        for(auto iter=danmuArray.begin();iter!=danmuArray.end();++iter)
        {
            QJsonObject dmObj=(*iter).toObject();
            if (dmObj.value("id").toInt() == 0) continue;
            QJsonValue content=dmObj.value("content");
            if(!content.isString()) continue;
            QJsonValue user_name=dmObj.value("user_name");
            if(!user_name.isString()) continue;
            QJsonValue play_point=dmObj.value("play_point");
            if(!play_point.isDouble()) continue;
            QJsonValue createtime=dmObj.value("createtime");
            if(!createtime.isDouble()) continue;

            int colorVal=0xffffff;
            QJsonValue color = dmObj.value("font_color");
            if(color.isString()) colorVal=color.toString().mid(1).toInt(nullptr,16);
            int pos=1;
            int font_position=dmObj.value("font_position").toInt(),motion=dmObj.value("motion").toInt();
            if(font_position==100 && motion==1) pos=5; //top
            else if(font_position==300 && motion==1) pos=4; //bottom


            DanmuComment *danmu=new DanmuComment();
            danmu->text=content.toString();
            danmu->date=static_cast<long long>(createtime.toDouble()/1000);;
            danmu->time =play_point.toInt()*100;
            danmu->originTime=danmu->time;
            danmu->color= colorVal;
            danmu->fontSizeLevel=DanmuComment::Normal;
            danmu->setType(pos);
            danmu->sender="[PPTV]"+user_name.toString();
            danmuList.append(danmu);
        }
    }
}

void PPTVProvider::handleSearchReply(QString &reply, DanmuAccessResult *result)
{
    HTMLParserSax parser(reply);
    int pos=0;
    while((pos = reply.indexOf("<div class=\"positive-box clearfix\">",pos)) != -1)
    {
        parser.seekTo(pos);
        do
        {
            parser.readNext();
        }while(parser.currentNodeProperty("class")!="video-title");
        pos=parser.curPos();
        QString titleUrl("http:"+parser.currentNodeProperty("href"));
        if(!titleUrl.startsWith("http://v.pptv.com")) continue;
        QString title(parser.currentNodeProperty("title"));
        do
        {
            parser.readNext();
        }while(parser.currentNode()!="dl" || parser.isStartNode());
        parser.readNext();
        if(parser.currentNodeProperty("class").startsWith("episodes-box"))
        {
            while(true)
            {
                if(parser.currentNode()=="a" && parser.currentNodeProperty("class")=="child")
                {
                    QString url("http:"+parser.currentNodeProperty("href"));
                    if(url.startsWith("http://v.pptv.com"))
                    {
                        DanmuSourceItem epItem;
                        epItem.strId=url;
                        epItem.title=title + " " + parser.readContentUntil("a",false).trimmed();;
						QRegExp lre("<.*>");
						lre.setMinimal(true);
						epItem.title.replace(lre, "");
                        result->list.append(epItem);
                    }
                }
                if(parser.currentNode()=="div" && !parser.isStartNode()) break;
				parser.readNext();
            }
        }
        else if(parser.currentNodeProperty("class")=="btn-block")
        {
            DanmuSourceItem epItem;
            epItem.title=title;
            epItem.strId=titleUrl;
            result->list.append(epItem);
        }
        pos=parser.curPos();
    }
    if((pos = reply.indexOf("<div class=\"news-list\">",parser.curPos()) != -1))
    {
        parser.seekTo(pos);
        while(true)
        {
            if(parser.currentNode()=="li" && parser.isStartNode())
            {
                do
                {
                    parser.readNext();
                }while(parser.currentNodeProperty("class")!="info-block");
                parser.readNext();
                QString title(parser.currentNodeProperty("title"));
                parser.readNext();
                QString url("http:"+parser.currentNodeProperty("href"));
                if(url.startsWith("http://v.pptv.com"))
                {
                    DanmuSourceItem item;
                    item.strId=url;
                    item.title=title;
                    result->list.append(item);
                }
            }
            if(parser.currentNode()=="ul" && !parser.isStartNode()) break;
			parser.readNext();
        }
    }
    result->error = false;
}

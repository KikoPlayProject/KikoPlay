#include "tencentprovider.h"
#include "Common/htmlparsersax.h"
#include "Common/network.h"
namespace
{
    const char *supportedUrlRe[]={"(https?://)?v\\.qq\\.com/x/cover/[a-zA-Z0-9/]+\\.html"};
}
QStringList TencentProvider::supportedURLs()
{
    return QStringList({"https://v.qq.com/x/cover/gtn6ik9kapbiqm0.html",
                        "https://v.qq.com/x/cover/gtn6ik9kapbiqm0/o0029t5qpp8.html"});
}

DanmuAccessResult *TencentProvider::search(const QString &keyword)
{
    QString baseUrl = QString("https://v.qq.com/x/search/");
    QUrlQuery query;
    query.addQueryItem("q",keyword);
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

DanmuAccessResult *TencentProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    result->error=false;
    result->list.append(*item);
    emit epInfoDone(result,item);
    return result;
}

DanmuAccessResult *TencentProvider::getURLInfo(const QString &url)
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

QString TencentProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString errInfo;
    try
    {
        QString replyStr(Network::httpGet(item->strId,QUrlQuery()));
        QRegExp re("VIDEO_INFO = (.*)\\n");
        re.setMinimal(true);
        bool hasError=true;
        do
        {
            int pos=re.indexIn(replyStr);
            if(pos==-1) break;
            QStringList captured=re.capturedTexts();
            QJsonObject videoInfo(Network::toJson(captured[1]).object());
            item->title=videoInfo.value("title").toString();
            item->subId=videoInfo.value("duration").toString().toFloat()/30;
            QString vid(item->strId=videoInfo.value("vid").toString());
            QUrlQuery query;
            query.addQueryItem("vid",vid);
            QXmlStreamReader reader(Network::httpGet("http://bullet.video.qq.com/fcgi-bin/target/regist",query));
            while(!reader.atEnd())
            {
                if(reader.isStartElement())
                {
                    if(reader.name()=="targetid")
                    {
                        item->strId=reader.readElementText();
                        break;
                    }
                }
                reader.readNext();
            }
            downloadAllDanmu(item->strId,item->subId,danmuList);
            hasError=false;
        }while(false);
        if(hasError) errInfo=tr("Decode Failed");
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadDone(errInfo);
    return errInfo;
}

QString TencentProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    int s=url.indexOf(':')+1,e=url.indexOf(';');
    QString id=url.mid(s,e-s);
    int l=url.mid(url.lastIndexOf(':')+1).toInt();
    downloadAllDanmu(id,l,danmuList);
    return QString();
}

void TencentProvider::downloadAllDanmu(const QString &id, int length, QList<DanmuComment *> &danmuList)
{
    QString baseUrl("https://mfm.video.qq.com/danmu");
    QList<QPair<QString,QString> > queryItems({
        {"otype","json"},{"target_id",id}
    });

    for (int i=0;i<=length;++i)
    {
        queryItems.append(QPair<QString,QString>("timestamp",QString::number(i*30)));
        QUrlQuery query;
        query.setQueryItems(queryItems);
        queryItems.removeLast();
        try
        {
            QString str(Network::httpGet(baseUrl,query));
            QJsonObject obj(Network::toJson(str).object());
            QJsonArray danmuArray(obj.value("comments").toArray());
            for(auto iter=danmuArray.begin();iter!=danmuArray.end();++iter)
            {
                QJsonObject dmObj=(*iter).toObject();
                QJsonValue content=dmObj.value("content");
                if(!content.isString()) continue;
                QJsonValue opername=dmObj.value("opername");
                if(!opername.isString()) continue;
                QJsonValue timepoint=dmObj.value("timepoint");
                if(!timepoint.isDouble()) continue;

                int posVal=1, colorVal=0xffffff;
                try
                {
                    QJsonObject pobj(Network::toJson(dmObj.value("content_style").toString()).object());
                    QJsonValue pos = pobj.value("position");
                    if (pos.isDouble()) posVal = pos.toInt();
                    else if (pos.isString()) posVal = pos.toString().toInt();
                    else
                        posVal = 1;
                    QJsonValue color = pobj.value("color");
                    if (color.isDouble()) colorVal = color.toInt();
                    else if (color.isString()) colorVal = color.toString().toInt(nullptr,16);
                    else
                        colorVal = 0xffffff;
                }
                catch (const Network::NetworkError &)
                {

                }

                DanmuComment *danmu=new DanmuComment();
                danmu->text=content.toString();
                danmu->date=0;
                danmu->time =timepoint.toInt()*1000;
                danmu->originTime=danmu->time;
                danmu->color= colorVal;
                danmu->fontSizeLevel=DanmuComment::Normal;
                if(posVal==1) danmu->setType(1);
                else if(posVal==5) danmu->setType(5);//top
                else if(posVal==6) danmu->setType(4);//bottom
                else danmu->setType(1);

                danmu->sender="[Tencent]"+opername.toString();
                if(danmu->type!=DanmuComment::UNKNOW)danmuList.append(danmu);
                else delete danmu;
            }
        } catch (const Network::NetworkError &) {

        }
    }
}

void TencentProvider::handleSearchReply(QString &reply, DanmuAccessResult *result)
{
    QRegExp re("<div class=(\"\\s*result_item\\s*result_item_v)|(\"result_item result_item_h)");
    re.setMinimal(true);
    int pos=0;
    HTMLParserSax parser(reply);
    while ((pos = re.indexIn(reply, pos)) != -1)
    {
        parser.seekTo(pos);
        pos += re.matchedLength();
        parser.readNext();
        bool isItem_v=parser.currentNodeProperty("class").contains("result_item_v");
        do
        {
            parser.readNext();
        }while(parser.currentNodeProperty("class")!="result_title");
        parser.readNext();
        QString titleUrl(parser.currentNodeProperty("href"));
        if(!titleUrl.startsWith("http://v.qq.com") && !titleUrl.startsWith("https://v.qq.com")) continue;
        QString title(parser.readContentUntil("a",false));
        QRegExp lre("<.*>");
        lre.setMinimal(true);
        title.replace(lre,"");
        if(isItem_v)
        {
            do
            {
                parser.readNext();
            }while(parser.currentNodeProperty("class")!="item");
            while(parser.currentNodeProperty("class")=="item")
            {
                parser.readNext();
                QString url(parser.currentNodeProperty("href"));
                if(url.startsWith("http://v.qq.com") || url.startsWith("https://v.qq.com"))
                {
                    DanmuSourceItem epItem;
                    epItem.title=title + " " + parser.readContentText();
                    epItem.strId=url;
                    result->list.append(epItem);
                }
                do
                {
                    parser.readNext();
                }while(parser.currentNode()!="div");
                parser.readNext();
            }
        }
        else
        {
            DanmuSourceItem epItem;
            epItem.title=title;
            epItem.strId=titleUrl;
            result->list.append(epItem);
        }
    }
	result->error = false;
}

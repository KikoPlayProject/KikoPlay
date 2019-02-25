#include "youkuprovider.h"
#include "Common/htmlparsersax.h"
#include "Common/network.h"
namespace
{
    const char *supportedUrlRe[]={"(https?://)?v\\.youku\\.com/v_show/id_[a-zA-Z0-9]+(==)?(.html)?"};
}

QStringList YoukuProvider::supportedURLs()
{
    return QStringList({"https://v.youku.com/v_show/id_XMTI3ODI4OTU1Ng"});
}

DanmuAccessResult *YoukuProvider::search(const QString &keyword)
{
    QString baseUrl = QString("https://so.youku.com/search_video/q_%1").arg(keyword);
    DanmuAccessResult *searchResult=new DanmuAccessResult;
    searchResult->providerId=id();
    try
    {
        QString str(Network::httpGet(baseUrl,QUrlQuery()));
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

DanmuAccessResult *YoukuProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    result->error=false;
    result->list.append(*item);
    emit epInfoDone(result,item);
    return result;
}

DanmuAccessResult *YoukuProvider::getURLInfo(const QString &url)
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

QString YoukuProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString errInfo;
    try
    {
        QString replyStr(Network::httpGet(item->strId,QUrlQuery()));
        QRegExp re;
        re.setMinimal(true);
        bool hasError=true;
        do
        {
            re.setPattern("\\bvideoId: '(\\d+)'");
            int pos=re.indexIn(replyStr);
            if(pos==-1) break;
            QStringList captured=re.capturedTexts();
            item->strId=captured[1];
            re.setPattern("\\bseconds: '([\\d\\.]+)'");
            pos=re.indexIn(replyStr);
            if(pos==-1) break;
			captured = re.capturedTexts();
            item->subId=captured[1].toFloat()/60;
            if(item->title.isEmpty())
            {
                re.setPattern("<meta name=\"title\" content=\"(.*)\" />");
                pos=re.indexIn(replyStr);
                if(pos!=-1)
                {
                    item->title=re.capturedTexts()[1];
                }
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
    emit downloadDone(errInfo,item);
    return errInfo;
}

QString YoukuProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    int s=url.indexOf(':')+1,e=url.indexOf(';');
    QString id=url.mid(s,e-s);
    int l=url.mid(url.lastIndexOf(':')+1).toInt();
    downloadAllDanmu(id,l,danmuList);
    return QString();
}

void YoukuProvider::downloadAllDanmu(const QString &id, int length, QList<DanmuComment *> &danmuList)
{
    QString baseUrl("http://service.danmu.youku.com/list");
    QList<QPair<QString,QString> > queryItems({
        {"mcount","1"},{"ct","1001"},{"iid",id}
    });

    for (int i=0;i<=length;++i)
    {
        queryItems.append(QPair<QString,QString>("mat",QString::number(i)));
        QUrlQuery query;
        query.setQueryItems(queryItems);
        queryItems.removeLast();
        try
        {
            QString str(Network::httpGet(baseUrl,query));
            QJsonObject obj(Network::toJson(str).object());
            QJsonArray danmuArray(obj.value("result").toArray());
            for(auto iter=danmuArray.begin();iter!=danmuArray.end();++iter)
            {
                QJsonObject dmObj=(*iter).toObject();
                QJsonValue content=dmObj.value("content");
                if(!content.isString()) continue;
                QJsonValue date=dmObj.value("createtime");
                if(!date.isDouble()) continue;
                QJsonValue uid=dmObj.value("uid");
                if(!uid.isDouble()) continue;
                QJsonValue playat=dmObj.value("playat");
                if(!playat.isDouble()) continue;

				int posVal=3, colorVal=0xffffff;
				try
				{
					QJsonObject pobj(Network::toJson(dmObj.value("propertis").toString()).object());
					QJsonValue pos = pobj.value("pos");
					if (pos.isDouble()) posVal = pos.toInt();
					else if (pos.isString()) posVal = pos.toString().toInt();
					else
						posVal = 3;
					QJsonValue color = pobj.value("color");
					if (color.isDouble()) colorVal = color.toInt();
					else if (color.isString()) colorVal = color.toString().toInt();
					else
						colorVal = 0xffffff;
				}
				catch (const Network::NetworkError &)
				{

				}

                DanmuComment *danmu=new DanmuComment();
                danmu->text=content.toString();
                danmu->time =playat.toInt();
                danmu->originTime=danmu->time;
                danmu->color= colorVal;
                danmu->fontSizeLevel=DanmuComment::Normal;
                if(posVal==3) danmu->setType(1);
                else if(posVal==4) danmu->setType(5);
                else if(posVal==6) danmu->setType(4);
                else danmu->setType(1);

                danmu->sender="[Youku]"+QString::number((long long)uid.toDouble());
                danmu->date=static_cast<long long>(date.toDouble()/1000);
                if(danmu->type!=DanmuComment::UNKNOW)danmuList.append(danmu);
                else delete danmu;
            }
        } catch (const Network::NetworkError &) {

        }
    }
}

void YoukuProvider::handleSearchReply(QString &reply, DanmuAccessResult *result)
{
    QRegExp re("\"domid\":\"bpmodule-main\"(.*)\"html\":\"(.*)\"..</script>");
    re.setMinimal(true);
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
		QString html(list.at(2));
		html.replace("\\n", "\n");
		html.replace("\\t", "\t");
		html.replace("\\\"", "\"");
        HTMLParserSax parser(html);
        DanmuSourceItem item;
        bool epStart=false;
        bool youkuItem=false;
        while(!parser.atEnd())
        {
            if(parser.currentNodeProperty("class")=="mod-main")
            {
                while(parser.currentNode()!="a") parser.readNext();
                QString strId(parser.currentNodeProperty("href"));
                if(strId.contains("v.youku.com"))
                {
                    item.title=parser.currentNodeProperty("title");
                    if(strId.startsWith("//"))strId="http:"+strId;
                    item.strId=strId;
                    youkuItem=true;
                }
                else
                {
                    youkuItem=false;
                }
                epStart=false;
            }
            else if(youkuItem && parser.currentNodeProperty("class")=="mod-play")
            {
                parser.readNext();
                if(!parser.isStartNode())
                {
                    result->list.append(item);
                }
                else
                {
                    epStart=true;
                }
            }
            else if(epStart && parser.isStartNode() && parser.currentNode()=="li")
            {
                parser.readNext();
                if(parser.currentNode()=="a")
                {
                    DanmuSourceItem epItem;
                    epItem.title=item.title + " " + parser.currentNodeProperty("title");
                    epItem.strId=parser.currentNodeProperty("href");
                    result->list.append(epItem);
                }
            }
            parser.readNext();
        }
    }
    result->error = false;
}

#include "dililiprovider.h"
#include "Common/network.h"
#include "Common/htmlparsersax.h"

namespace
{
    const char *supportedUrlRe[]={"(https?://)?www\\.5dm\\.tv/bangumi/dv[0-9]+(\\?link=[0-9]+)?/?",
                                 "(https?://)?www\\.5dm\\.tv/end/dv[0-9]+(\\?link=[0-9]+)?/?"};
}

QStringList DililiProvider::supportedURLs()
{
    return QStringList({"https://www.5dm.tv/bangumi/dv41292",
                       "https://www.5dm.tv/end/dv17582"});
}

DanmuAccessResult *DililiProvider::search(const QString &keyword)
{
    QString baseUrl = QString("https://www.5dm.tv/search/%1").arg(keyword);
    //QUrlQuery query;
    //query.addQueryItem("s", keyword);

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

DanmuAccessResult *DililiProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    try
    {
        QString str(Network::httpGet(item->strId,QUrlQuery()));
        handleEpReply(str,result);
    }
    catch(Network::NetworkError &error)
    {
        result->error=true;
        result->errorInfo=error.errorInfo;
    }
    emit epInfoDone(result,item);
    return result;
}

DanmuAccessResult *DililiProvider::getURLInfo(const QString &url)
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
    item.id=1;//need to get ep info first
    return getEpInfo(&item);
}

QString DililiProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString videoId=item->strId;
    QString errInfo;
    if(item->id==1)
    {
        try
        {
            QString replyStr(Network::httpGet(item->strId,QUrlQuery()));
            QRegExp re("(<div id=\"player-embed\">)(.*)(</div>)");
            QRegExp idRe("cid=(.*)&");
			idRe.setMinimal(true);
            int pos=re.indexIn(replyStr);
            if(pos!=-1)
            {
                QStringList list = re.capturedTexts();
                pos=idRe.indexIn(list.at(2));
            }
            if(pos!=-1)
            {
                QStringList alist=idRe.capturedTexts();
                videoId = alist.at(1);
            }
            if(pos==-1)
            {
                errInfo=tr("Get video Info Failed");
                emit downloadDone(errInfo);
                return errInfo;
            }
            item->strId=videoId;
        }
        catch(Network::NetworkError &error)
        {
            errInfo=error.errorInfo;
            emit downloadDone(errInfo);
            return errInfo;
        }
    }
    QString baseUrl="https://www.5dm.tv/player/xml.php";
    QUrlQuery query;
    query.addQueryItem("id", videoId);
    try
    {
        QString replyStr(Network::httpGet(baseUrl,query));
        handleDownloadReply(replyStr,danmuList);
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadDone(errInfo);
    return errInfo;
}

QString DililiProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    QString id(url.mid(url.lastIndexOf(':')+1));
    DanmuSourceItem item;
    item.strId=id;
    item.id=0;
    return downloadDanmu(&item,danmuList);
}

void DililiProvider::handleSearchReply(const QString &reply, DanmuAccessResult *result)
{
    QRegExp re("(<div class=\"post_ajax_tm\">)(.*)(<div class=\"clearfix\"></div> )");
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
        HTMLParserSax parser(list.at(2));
        DanmuSourceItem item;
        bool itemStart=false;
        while(!parser.atEnd())
        {
            if(parser.currentNodeProperty("class").startsWith("video-item"))
            {
                if(!itemStart)itemStart=true;
                else result->list.append(item);
            }
            else if(parser.currentNode()=="h3" && parser.isStartNode())
            {
                parser.readNext();
                item.strId=parser.currentNodeProperty("href");
                item.title=parser.currentNodeProperty("title");
            }
            else if(parser.currentNodeProperty("class")=="item-content hidden")
            {
                parser.readNext();
                item.description=parser.readContentText();
            }
            parser.readNext();
        }
        if(itemStart)result->list.append(item);
    }
    result->error = false;
}

void DililiProvider::handleEpReply(const QString &reply, DanmuAccessResult *result)
{
    QRegExp re("(<table class=\"table table-bordered\">)(.*)(</table>)");
    re.setMinimal(true);
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
        HTMLParserSax parser(list.at(2));
        DanmuSourceItem item;
        item.danmuCount=-1;
        item.delay=0;
        item.id=1;
        item.extra=0;
        bool itemStart=false;
        while(!parser.atEnd())
        {
            if(parser.currentNode()=='a' && parser.isStartNode())
            {
                if(!itemStart)itemStart=true;
                else result->list.append(item);
                item.strId=parser.currentNodeProperty("href");
            }
            else if(parser.currentNode()=="i" && !parser.isStartNode())
            {
                item.title=parser.readContentText();
            }
            parser.readNext();
        }
        if(itemStart)result->list.append(item);
    }
    else
    {
        re.setPattern("(<div id=\"player-embed\">)(.*)(</div>)");
        pos=re.indexIn(reply);
        QRegExp idRe("cid=(.*)&");
		idRe.setMinimal(true);
        if(pos!=-1)
        {
            QStringList list = re.capturedTexts();
            pos=idRe.indexIn(list.at(2));
        }
        if(pos!=-1)
        {
            QStringList alist=idRe.capturedTexts();
            DanmuSourceItem si;
            si.danmuCount = -1;
            si.strId = alist.at(1);
            si.delay=0;
            si.id=0;
			si.extra = 0;
            QRegExp titleRe("(<h1 class=\"video-title\">)(.*)(</h1>)");
            titleRe.setMinimal(true);
            if(titleRe.indexIn(reply)!=-1)
            {
                si.title=titleRe.capturedTexts()[2];
            }
            else
            {
                si.title = alist.at(1);
            }
            result->list.append(si);
        }
    }
    result->error=false;
    return;
}

void DililiProvider::handleDownloadReply(const QString &reply, QList<DanmuComment *> &danmuList)
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
                if(attrList.length()<7)continue;
                DanmuComment *danmu=new DanmuComment();
                danmu->text=reader.readElementText();
                danmu->time = attrList[0].toFloat() * 1000;
                danmu->setType(attrList[1].toInt());
                danmu->color=attrList[3].toInt();
                danmu->date=attrList[4].toLongLong();
                danmu->sender="[5dm]"+attrList[6];
                danmu->blockBy=-1;
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

#include "bahamutprovider.h"
#include "Common/htmlparsersax.h"
#include "Common/network.h"
namespace
{
    const char *supportedUrlRe[]={"(https?://)?ani\\.gamer\\.com\\.tw/animeVideo\\.php\\?sn=[0-9]+/?"};
}

QStringList BahamutProvider::supportedURLs()
{
    return QStringList({"https://ani.gamer.com.tw/animeVideo.php?sn=9285"});
}

DanmuAccessResult *BahamutProvider::search(const QString &keyword)
{
    QString baseUrl = "https://ani.gamer.com.tw/search.php";
    QUrlQuery query;
    WORD wLanguageID = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
    LCID Locale = MAKELCID(wLanguageID, SORT_CHINESE_PRCP);
    QChar *buf=new QChar[keyword.length()];
    LCMapString(Locale,LCMAP_TRADITIONAL_CHINESE,reinterpret_cast<LPCWSTR>(keyword.constData()),keyword.length(),reinterpret_cast<LPWSTR>(buf),keyword.length());
    QString outstr(buf, keyword.length());
	delete[] buf;
    query.addQueryItem("kw", outstr);

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

DanmuAccessResult *BahamutProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    try
    {
        QString baseUrl=item->extra==1?"https://ani.gamer.com.tw/animeRef.php":"https://ani.gamer.com.tw/animeVideo.php";
        QUrlQuery query;
        query.addQueryItem("sn",QString::number(item->id));
        QString str(Network::httpGet(baseUrl,query));
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

DanmuAccessResult *BahamutProvider::getURLInfo(const QString &url)
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
    QRegExp re("sn=([0-9]+)");
    re.indexIn(url);
    QStringList captured=re.capturedTexts();
    QString snStr = captured[1];
    DanmuSourceItem item;
    item.id=snStr.toInt();
    item.extra=0;
    return getEpInfo(&item);
}

QString BahamutProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString baseUrl="https://ani.gamer.com.tw/ajax/danmuGet.php";
    QByteArray data(QString("sn=%1").arg(item->id).toUtf8());
    QString errInfo;
    try
    {
        QString replyStr(Network::httpPost(baseUrl,data));
        QJsonDocument document(Network::toJson(replyStr));
        handleDownloadReply(document,danmuList);
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadDone(errInfo);
    return errInfo;
}

QString BahamutProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    int sn=url.mid(url.lastIndexOf(':')+1).toInt();
    DanmuSourceItem item;
    item.id=sn;
    return downloadDanmu(&item,danmuList);
}

void BahamutProvider::handleSearchReply(const QString &reply, DanmuAccessResult *result)
{
    QRegExp re("(<ul class=\"anime_list\">)(.*)(</ul>)");
    re.setMinimal(true);
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
        HTMLParserSax parser(list.at(2));
        DanmuSourceItem item;
        item.extra=1;//search url need redirect
        while(!parser.atEnd())
        {
            if(parser.currentNodeProperty("class")=="info")
            {
                parser.readNext();
                item.title=parser.readContentText();
                parser.readNext();
                parser.readNext();
                item.description=parser.readContentText();
                result->list.append(item);
            }
            else if(parser.currentNode()=="a" && parser.isStartNode())
            {
                QRegExp re("animeRef\\.php\\?sn=([0-9]+)");
                if(re.indexIn(parser.currentNodeProperty("href"))!=-1)
                {
                    QString idStr = re.capturedTexts()[1];
                    item.id=idStr.toInt();
                }
            }
            parser.readNext();
        }
    }
    result->error=false;
}

void BahamutProvider::handleEpReply(QString &reply, DanmuAccessResult *result)
{
    QRegExp re("(<section class=\"season\">)(.*)(</section>)");
    re.setMinimal(true);
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
        HTMLParserSax parser(list.at(2));
        DanmuSourceItem item;
        item.extra=0;
        while(!parser.atEnd())
        {
            if(parser.currentNode()=="a" && parser.isStartNode())
            {
                QRegExp re("\\?sn=([0-9]+)");
                if(re.indexIn(parser.currentNodeProperty("href"))!=-1)
                {
                    QString idStr = re.capturedTexts()[1];
                    item.id=idStr.toInt();
                }
                item.title=parser.readContentText();
                result->list.append(item);
            }
            parser.readNext();
        }
    }
    else
    {
        QRegExp snTitleRe("animefun.videoSn.?=.?([0-9]+);.*animefun.title.?=.?'(.*)'");
        int pos=snTitleRe.indexIn(reply);
        if(pos!=-1)
        {
            QStringList list = snTitleRe.capturedTexts();
            DanmuSourceItem item;
            item.extra=0;
            item.id=list[1].toInt();
            item.title=list[2];
            result->list.append(item);
        }

    }
    result->error=false;
}

void BahamutProvider::handleDownloadReply(QJsonDocument &document, QList<DanmuComment *> &danmuList)
{
    if(!document.isArray())return;
    QJsonArray dmArray(document.array());
    for(auto dmIter=dmArray.begin();dmIter!=dmArray.end();++dmIter)
    {
        if(!(*dmIter).isObject())continue;
        QJsonObject dmObj=(*dmIter).toObject();
        QJsonValue text=dmObj.value("text");
        if(text.type()!=QJsonValue::String)continue;
        QJsonValue color=dmObj.value("color");
        if(color.type()!=QJsonValue::String)continue;
        QJsonValue size=dmObj.value("size");
        if(size.type()!=QJsonValue::Double)continue;
        QJsonValue pos=dmObj.value("position");
        if(pos.type()!=QJsonValue::Double)continue;
        QJsonValue time=dmObj.value("time");
        if(time.type()!=QJsonValue::Double)continue;
        QJsonValue user=dmObj.value("userid");
        if(user.type()!=QJsonValue::String)continue;
        DanmuComment *danmu=new DanmuComment();
        danmu->text=text.toString();
        danmu->time = time.toInt()*100;
        danmu->originTime=danmu->time;
        danmu->type=DanmuComment::DanmuType(pos.toInt());
        QString colorStr(color.toString());
        danmu->color=colorStr.rightRef(colorStr.length()-1).toInt(nullptr,16);
        danmu->date=0;
        danmu->sender = "[Gamer]"+user.toString();
        switch (size.toInt())
        {
        case 1:
            danmu->fontSizeLevel=DanmuComment::Normal;
            break;
        case 0:
            danmu->fontSizeLevel=DanmuComment::Small;
            break;
        case 2:
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

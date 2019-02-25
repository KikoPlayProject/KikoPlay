#include "tucaoprovider.h"
#include "Common/htmlparsersax.h"
#include "Common/network.h"

namespace
{
    const char *supportedUrlRe[]={"(http://)?www\\.tucao\\.one/play/h[0-9]+(#[0-9]+)?/?",
                                 "h[0-9]+(#[0-9]+)?"};
}
DanmuAccessResult *TucaoProvider::search(const QString &keyword)
{
    QString baseUrl = "http://www.tucao.one/index.php";
    QUrlQuery query;
    query.addQueryItem("m", "search");
    query.addQueryItem("a", "init2");
    query.addQueryItem("time", "all");
    query.addQueryItem("q", keyword);

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

DanmuAccessResult *TucaoProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    try
    {
        QString str(Network::httpGet(QString("http://www.tucao.one/play/h%0/").arg(item->id),QUrlQuery()));
        handleEpReply(str,item,result);
    }
    catch(Network::NetworkError &error)
    {
        result->error=true;
        result->errorInfo=error.errorInfo;
    }
    emit epInfoDone(result,item);
    return result;
}

DanmuAccessResult *TucaoProvider::getURLInfo(const QString &url)
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
    QRegExp re("h([0-9]+)(#)?([0-9]+)?");
    re.indexIn(url);
    QStringList captured=re.capturedTexts();
    QString hidStr = captured[1];
    DanmuSourceItem item;
    item.id=hidStr.toInt();
    item.subId=captured.size()==4?captured[3].toInt():0;
    //item.source=DanmuSource::Tucao;
    return getEpInfo(&item);
}

QString TucaoProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString baseUrl="http://www.tucao.one/index.php";
    QUrlQuery query;
    query.addQueryItem("m", "mukio");
    query.addQueryItem("a", "init");
    query.addQueryItem("c", "index");
    query.addQueryItem("playerID", QString("11-%1-1-%2").arg(item->id).arg(item->subId));
    QString errInfo;
    try
    {
        QString replyStr(Network::httpGet(baseUrl,query));
        handleDownloadReply(replyStr,danmuList);
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadDone(errInfo,item);
    return errInfo;
}

QString TucaoProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    int s=url.indexOf(':')+1,e=url.indexOf(';');
    int hid=url.mid(s,e-s).toInt();
    int p=url.mid(url.lastIndexOf(':')+1).toInt();
    DanmuSourceItem item;
    item.id=hid;
    item.subId=p;
    return downloadDanmu(&item,danmuList);
}

void TucaoProvider::handleSearchReply(QString &reply, DanmuAccessResult *result)
{
    QRegExp re("(<div class=\"search_list\" style=\"border-top:1px solid #eee;\">)(.*)(<div id=\"float_show\">)");
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
        HTMLParserSax parser(list.at(2));
        DanmuSourceItem item;
        //item.source=DanmuSource::Tucao;
        bool itemStart=false;
        while(!parser.atEnd())
        {
            if(parser.currentNodeProperty("class")=="info")
            {
                if(!itemStart)itemStart=true;
                else result->list.append(item);
            }
            else if(parser.currentNodeProperty("class")=="blue")
            {
                QRegExp re("h([0-9]+)");
                re.indexIn(parser.currentNodeProperty("href"));
                QString hidStr = re.capturedTexts()[1];
                item.id=hidStr.toInt();
                item.title=parser.readContentText();
            }
            else if(parser.currentNodeProperty("class")=="d")
            {
                item.description=parser.readContentText();
            }
            parser.readNext();
        }
        if(itemStart)result->list.append(item);
    }
    result->error = false;
}

void TucaoProvider::handleEpReply(QString &reply,DanmuSourceItem *item, DanmuAccessResult *result)
{
    QRegExp re("(<ul id=\"player_code\" mid=\".*\"><li>)(.*)(</li><li>.*</li></ul>)");
    re.setMinimal(true);
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
        QString epInfo(list.at(2));
        QStringList eps(epInfo.split('|'));
        eps.pop_front();
        re.setPattern("(<h1 class=\"show_title\">)(.*)(<span style=\"color:#F40;\">)");
        QString pageTitle;
        if(re.indexIn(reply)!=-1)
        {
            pageTitle=re.capturedTexts()[2];
        }
        int index=0;
        for(QString &ep:eps)
        {
            QString title(ep.split('*').at(0));
            DanmuSourceItem si;
            si.danmuCount = -1;
            si.title = title.trimmed().isEmpty()?(pageTitle.isEmpty()?item->title:pageTitle):title;
            si.id = item->id;
            //si.source = DanmuSource::Tucao;
            si.subId = index++;
            si.extra=0;
            si.delay=0;
            result->list.append(si);
        }
    }
    result->error=false;
}

void TucaoProvider::handleDownloadReply(QString &reply, QList<DanmuComment *> &danmuList)
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
                if(attrList.length()<5)continue;
                DanmuComment *danmu=new DanmuComment();
                danmu->text=reader.readElementText();
                danmu->time = attrList[0].toFloat() * 1000;
                danmu->originTime=danmu->time;
                danmu->setType(attrList[1].toInt());
                danmu->color=attrList[3].toInt();
                danmu->date=attrList[4].toLongLong();
                danmu->blockBy=-1;
                danmu->sender="[Tucao]";
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

QStringList TucaoProvider::supportedURLs()
{
    return QStringList({"http://www.tucao.one/play/h4077044/",
                        "h4066128"});
}

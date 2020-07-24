#include "iqiyiprovider.h"
#include "Common/htmlparsersax.h"
#include "Common/network.h"
#include "Common/zlib.h"

namespace
{
    const char *supportedUrlRe[]={"(https?://)?www\\.iqiyi\\.com/(v|w)_.+\\.html"};
}

QStringList IqiyiProvider::supportedURLs()
{
    return QStringList({"https://www.iqiyi.com/v_19rr1jer2o.html",
                        "https://www.iqiyi.com/w_19rsjq2cbh.html"});
}

DanmuAccessResult *IqiyiProvider::search(const QString &keyword)
{
    QString baseUrl = QString("https://so.iqiyi.com/so/q_%1").arg(keyword);
    DanmuAccessResult *searchResult=new DanmuAccessResult;
    searchResult->providerId=id();
    try
    {
        QString str(Network::httpGet(baseUrl,QUrlQuery(), {"Accept","*/*"}));
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

DanmuAccessResult *IqiyiProvider::getEpInfo(DanmuSourceItem *item)
{
    DanmuAccessResult *result=new DanmuAccessResult;
    result->providerId=id();
    result->error=false;
    result->list.append(*item);
    emit epInfoDone(result,item);
    return result;
}

DanmuAccessResult *IqiyiProvider::getURLInfo(const QString &url)
{
    int reCount=sizeof(supportedUrlRe)/sizeof(const char *);
    int matchIndex=0;
    for(;matchIndex<reCount;++matchIndex)
    {
        QRegExp re(supportedUrlRe[matchIndex]);
        re.setMinimal(true);
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

QString IqiyiProvider::downloadDanmu(DanmuSourceItem *item, QList<DanmuComment *> &danmuList)
{
    QString errInfo;
    try
    {
        QString replyStr(Network::httpGet(item->strId,QUrlQuery()));
        QRegExp re("playPageInfo\\s*(=|\\|\\|)\\s*(\\{\".*\\})(;|\\s)");
        re.setMinimal(true);
        int pos=re.indexIn(replyStr);
        if(pos==-1)
        {
            errInfo=tr("Decode Failed");
        }
        else
        {
            QStringList captured=re.capturedTexts();
            QJsonObject videoInfo(Network::toJson(captured[2]).object());
            item->title=videoInfo.value("tvName").toString();
            item->strId=QString::number(static_cast<qint64>(videoInfo.value("tvId").toDouble()));
            QStringList durations(videoInfo.value("duration").toString().split(':',QString::SkipEmptyParts));
            int duration=0,base=1;
            for(int i=durations.size()-1;i>=0;--i)
            {
                duration+=durations.at(i).toInt()*base;
                base*=60;
            }
            item->extra=duration;
            item->subId=duration / (60 * 5) + 1;
            downloadAllDanmu(item->strId,item->subId,danmuList);
        }
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadDone(errInfo,item);
    return errInfo;
}

QString IqiyiProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    QStringList info(url.split(';',QString::SkipEmptyParts));
    QString tvId=info[0].mid(info[0].indexOf(':')+1);
    int length=-1;
    if(info.count()>1) length=info[1].mid(info[1].indexOf(':')+1).toInt();
    downloadAllDanmu(tvId,length,danmuList);
    return QString();
}

void IqiyiProvider::handleSearchReply(QString &reply, DanmuAccessResult *result)
{
    QRegExp re("<div class=\"layout-main\">(.*)<div class=\"layout-side\">");
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
        HTMLParserSax parser(list.at(1));
        DanmuSourceItem item;
        item.extra=0;
        bool itemStart=false;
        while(!parser.atEnd())
        {
            if(parser.currentNodeProperty("class")=="qy-search-result-tit")
            {
                do{
                    parser.readNext();
                }while(parser.currentNodeProperty("class")!="main-tit");
                if(itemStart)
				{
                    result->list.append(item);
				}
                itemStart=true;
                item.title=parser.currentNodeProperty("title");
                item.strId=parser.currentNodeProperty("href");
                if(item.strId.startsWith("//")) item.strId.push_front("http:");
            }
            else if(parser.currentNodeProperty("class")=="qy-search-result-album")
            {
                while(parser.currentNode()!="ul" || parser.isStartNode())
                {
                    parser.readNext();
                    if(parser.currentNode()=="li" && parser.currentNodeProperty("class")=="album-item")
                    {
                        parser.readNext();
                        DanmuSourceItem epItem;
                        epItem.title=item.title + " " + parser.currentNodeProperty("title");
                        epItem.strId=parser.currentNodeProperty("href");
                        if(epItem.strId.startsWith("//")) epItem.strId.push_front("http:");
                        epItem.extra=0;
                        result->list.append(epItem);
                    }
                }
                itemStart = false;
            }
            else if(parser.currentNodeProperty("class")=="qy-search-result-album-half")
            {
                while(parser.currentNode()!="ul" || parser.isStartNode())
                {
                    parser.readNext();
                    if(parser.currentNode()=="li" && parser.currentNodeProperty("class")=="album-item")
                    {
                        parser.readNext();
                        DanmuSourceItem epItem;
                        epItem.title=parser.currentNodeProperty("title");
                        epItem.strId=parser.currentNodeProperty("href");
                        if(epItem.strId.startsWith("//")) epItem.strId.push_front("http:");
                        epItem.extra=0;
                        result->list.append(epItem);
                    }
                }
                itemStart = false;
            }
            parser.readNext();
        }
        if(itemStart && !item.title.trimmed().isEmpty())result->list.append(item);
    }
    result->error = false;
}

void IqiyiProvider::downloadAllDanmu(const QString &id, int length, QList<DanmuComment *> &danmuList)
{
    QString tvId = "0000" + id;
    QString s1(tvId.mid(tvId.length()-4,2)),s2(tvId.mid(tvId.length()-2));
    QString baseUrl=QString("http://cmts.iqiyi.com/bullet/%1/%2/%3_300_%4.z").arg(s1).arg(s2).arg(id);
    if(length==-1)
    {
        for(int i=1;;i++)
        {
            try
            {
                QString dmUrl=baseUrl.arg(i++);
                QByteArray replyBytes(Network::httpGet(dmUrl,QUrlQuery()));
                decodeDanmu(replyBytes,danmuList);
            }
            catch(Network::NetworkError &)
            {
                return;
            }
        }
    }
    else
    {
        QStringList urls;
        for (int i=1;i<=length;++i)
        {
            urls<<baseUrl.arg(i);
        }
        QList<QPair<QString, QByteArray> > results(Network::httpGetBatch(urls,QList<QUrlQuery>()));
        for(auto &result:results)
        {
            if(!result.first.isEmpty()) continue;
            decodeDanmu(result.second,danmuList);
        }
    }
}

void IqiyiProvider::decodeDanmu(const QByteArray &replyBytes, QList<DanmuComment *> &danmuList)
{
    QByteArray decompressResult;
    if(decompress(replyBytes,decompressResult)!=Z_OK)return;
    QXmlStreamReader reader(decompressResult);
    bool dmStart=false;
    DanmuComment tmpDanmu;
    tmpDanmu.setType(1);
    tmpDanmu.fontSizeLevel=DanmuComment::Normal;
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            if(reader.name()=="bulletInfo")
            {
                dmStart=true;
            }
            else if(reader.name()=="contentId")
            {
                QString idText=reader.readElementText();
                tmpDanmu.date=idText.mid(0,10).toLongLong();
            }
            else if(reader.name()=="content")
            {
                tmpDanmu.text=reader.readElementText();
            }
            else if(reader.name()=="showTime")
            {
                tmpDanmu.time = reader.readElementText().toFloat() * 1000;
                tmpDanmu.originTime= tmpDanmu.time;
            }
            else if(reader.name()=="color")
            {
                tmpDanmu.color = reader.readElementText().toInt(nullptr,16);
            }
            else if(reader.name()=="uid")
            {
                tmpDanmu.sender="[iqiyi]"+reader.readElementText();
            }
        }
        else if(reader.isEndElement())
        {
            if(reader.name()=="bulletInfo")
            {
                dmStart=false;
                danmuList.append(new DanmuComment(tmpDanmu));
            }
        }
        reader.readNext();
    }
}

int IqiyiProvider::decompress(const QByteArray &input, QByteArray &output)
{
    int ret;
    unsigned have;
    const int chunkSize=16384;
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;
    ret = inflateInit(&stream);
    if (ret != Z_OK) return ret;
    unsigned char inBuf[chunkSize];
    unsigned char outBuf[chunkSize];
    QDataStream inStream(input);
    while(!inStream.atEnd())
    {
        stream.avail_in=inStream.readRawData((char *)&inBuf,chunkSize);
        if (stream.avail_in == 0)
            break;
        stream.next_in = inBuf;
        do
        {
            stream.avail_out = chunkSize;
            stream.next_out = outBuf;
            ret = inflate(&stream, Z_NO_FLUSH);
            switch (ret)
            {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&stream);
                return ret;
            }
            have = chunkSize - stream.avail_out;
            output.append((const char *)outBuf,have);
        } while (stream.avail_out == 0);
    }
    (void)inflateEnd(&stream);
    return Z_OK ;
}


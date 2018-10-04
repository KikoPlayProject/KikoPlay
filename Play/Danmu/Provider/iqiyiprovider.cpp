#include "iqiyiprovider.h"
#include "Common/htmlparsersax.h"
#include "Common/network.h"
#include "Common/zlib.h"

namespace
{
    const char *supportedUrlRe[]={"(https?://)?www\\.iqiyi\\.com/v_.+\\.html"};
}

QStringList IqiyiProvider::supportedURLs()
{
    return QStringList({"https://www.iqiyi.com/v_19rr1jer2o.html"});
}

DanmuAccessResult *IqiyiProvider::search(const QString &keyword)
{
    QString baseUrl = QString("https://so.iqiyi.com/so/q_%1_site_iqiyi_m_").arg(keyword);
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
        QRegExp re("\"?tvId\"?:(.+),");
        re.setMinimal(true);
        int pos=re.indexIn(replyStr);
        if(pos==-1)
        {
            errInfo=tr("Decode Failed");
        }
        else
        {
            QStringList captured=re.capturedTexts();
            item->strId=captured[1];
            if(item->title.isEmpty())
            {
                QRegExp tRe("\"?tvName\"?:\"(.+)\",");
                tRe.setMinimal(true);
                int pos=tRe.indexIn(replyStr);
                if(pos!=-1)
                {
                    item->title=tRe.capturedTexts()[1];
                }
            }
            downloadAllDanmu(item->strId,danmuList);
        }
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadDone(errInfo);
    return errInfo;
}

QString IqiyiProvider::downloadBySourceURL(const QString &url, QList<DanmuComment *> &danmuList)
{
    QString tvId=url.mid(url.lastIndexOf(':')+1);
    downloadAllDanmu(tvId,danmuList);
    return QString();
}

void IqiyiProvider::handleSearchReply(QString &reply, DanmuAccessResult *result)
{
    QRegExp re("(<ul class=\"mod_result_list\">)(.*)(<div class=\"mod-page\")");
    int pos=re.indexIn(reply);
    if(pos!=-1)
    {
        QStringList list = re.capturedTexts();
        HTMLParserSax parser(list.at(2));
        DanmuSourceItem item;
        bool itemStart=false;
        bool epStart=false;
        while(!parser.atEnd())
        {
            if(parser.currentNodeProperty("class")=="list_item")
            {
                if(!itemStart)itemStart=true;
				else
				{
                    if (!epStart && !item.title.trimmed().isEmpty())result->list.append(item);
				}
                epStart=false;
                item.title=parser.currentNodeProperty("data-widget-searchlist-tvname");
            }
            else if(parser.currentNodeProperty("class")=="result_title")
            {
                parser.readNext();
                item.strId=parser.currentNodeProperty("href");
            }
            else if(parser.currentNodeProperty("class")=="info_play_btn")
            {
                item.strId=parser.currentNodeProperty("href");
            }
            else if(parser.currentNodeProperty("class")=="result_info_txt")
            {
                item.description=parser.readContentText();
            }
            else if(parser.currentNodeProperty("class")=="result_album clearfix" &&
                    parser.currentNodeProperty("data-tvlist-elem")=="list")
            {
                epStart=true;
            }
            else if(epStart && parser.currentNodeProperty("class")=="album_link")
            {
                if(!parser.currentNodeProperty("href").isEmpty())
                {
                    DanmuSourceItem epItem;
                    epItem.title=item.title + " " + parser.currentNodeProperty("title");
                    epItem.strId=parser.currentNodeProperty("href");
                    result->list.append(epItem);
                }
            }
            parser.readNext();
        }
        if(itemStart && !item.title.trimmed().isEmpty())result->list.append(item);
    }
    result->error = false;
}

void IqiyiProvider::downloadAllDanmu(const QString &id, QList<DanmuComment *> &danmuList)
{
    QString tvId = "0000" + id;
    QString s1(tvId.mid(tvId.length()-4,2)),s2(tvId.mid(tvId.length()-2));
    QString baseUrl=QString("http://cmts.iqiyi.com/bullet/%1/%2/%3_300_%4.z").arg(s1).arg(s2).arg(id);
    int i = 1;
    while(true)
    {
        try
        {
            QString dmUrl=baseUrl.arg(i++);
            QByteArray replyBytes(Network::httpGet(dmUrl,QUrlQuery()));
            QByteArray decompressResult;
            if(decompress(replyBytes,decompressResult)!=Z_OK)continue;
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
        catch(Network::NetworkError &)
        {
            return;
        }
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


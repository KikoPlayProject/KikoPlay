#include "network.h"
#include "Common/zlib.h"
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QThreadStorage>
#include <QSharedPointer>
namespace
{
    static QThreadStorage<QSharedPointer<QNetworkAccessManager>> managers;
    static QThreadStorage<QSharedPointer<Network::ReqAbortFlagObj>> abortFlagObjs;
}

QNetworkAccessManager *Network::getManager()
{
    if(!managers.hasLocalData())
    {
        managers.setLocalData(QSharedPointer<QNetworkAccessManager>::create());
    }
    return managers.localData().get();
}

Network::Reply Network::httpGet(const QString &url, const QUrlQuery &query, const QStringList &header, bool redirect)
{
    QUrl queryUrl(url);
    if(!query.isEmpty())  queryUrl.setQuery(query);
    QNetworkRequest request;
    request.setUrl(queryUrl);
    if(header.size()>=2)
    {
        Q_ASSERT((header.size() & 1) ==0);
        for(int i=0;i<header.size();i+=2)
        {
            request.setRawHeader(header[i].toUtf8(),header[i+1].toUtf8());
        }
    }
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, redirect);
    request.setMaximumRedirectsAllowed(maxRedirectTimes);

    QNetworkAccessManager *manager = getManager();
    manager->setCookieJar(nullptr);

    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);
    QNetworkReply *reply = manager->get(request);

    QEventLoop eventLoop;
    QObject::connect(getAbortFlag(), &ReqAbortFlagObj::abort, &eventLoop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    timer.start();
    eventLoop.exec();
    Reply replyObj;
    if (timer.isActive())
    {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError)
        {
            replyObj.hasError = false;
            replyObj.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            replyObj.content = reply->readAll();
            replyObj.headers = reply->rawHeaderPairs();
        }
        else
        {
            replyObj.hasError = true;
            replyObj.errInfo=reply->errorString();
        }
    }
    else
    {
        QObject::disconnect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        reply->abort();
        replyObj.hasError = true;
        replyObj.errInfo=QObject::tr("Replay Timeout");
    }
    reply->deleteLater();
    return replyObj;
}

Network::Reply Network::httpPost(const QString &url, const QByteArray &data, const QStringList &header, const QUrlQuery &query)
{
    QUrl queryUrl(url);
    if(!query.isEmpty())  queryUrl.setQuery(query);
    QNetworkRequest request;
    if(header.size()>=2)
    {
        for(int i=0;i<header.size();i+=2)
        {
            request.setRawHeader(header[i].toUtf8(),header[i+1].toUtf8());
        }
    }
    request.setUrl(queryUrl);

    QNetworkAccessManager *manager = getManager();
    manager->setCookieJar(nullptr);

    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);
    QNetworkReply *reply = manager->post(request, data);

    QEventLoop eventLoop;
    QObject::connect(getAbortFlag(), &ReqAbortFlagObj::abort, &eventLoop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    timer.start();
    eventLoop.exec();

    Reply replyObj;

    if (timer.isActive())
    {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError)
        {
            replyObj.hasError = false;
            replyObj.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            replyObj.content = reply->readAll();
            replyObj.headers = reply->rawHeaderPairs();
        }
        else
        {
            replyObj.hasError = true;
            replyObj.errInfo=reply->errorString();
        }
    }
    else
    {
        QObject::disconnect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        reply->abort();
        replyObj.hasError = true;
        replyObj.errInfo=QObject::tr("Replay Timeout");
    }
    reply->deleteLater();
    return replyObj;
}

QJsonDocument Network::toJson(const QString &str)
{
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(str.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        throw NetworkError(QObject::tr("Decode JSON Failed"));
    }
    return document;
}

QJsonValue Network::getValue(QJsonObject &obj, const QString &path)
{
    QStringList pathList(path.split('/'));
    QJsonValue value;
    QJsonValue cur(obj);
    for(const QString &key:pathList)
    {
        if(cur.isObject())
        {
            value=cur.toObject().value(key);
            cur=value;
        }
        else if(cur.isArray())
        {
            value=cur.toArray().at(key.toInt());
            cur=value;
        }
    }
    return value;
}

QList<Network::Reply> Network::httpGetBatch(const QStringList &urls, const QList<QUrlQuery> &queries, const QList<QStringList> &headers, bool redirect)
{
    Q_ASSERT(urls.size()==queries.size() || queries.size()==0);
    Q_ASSERT(urls.size()==headers.size() || headers.size()==0);
    QList<Reply> results;
	if (urls.isEmpty()) return results;
    int finishCount=0;
    QEventLoop eventLoop;
    QNetworkAccessManager *manager(getManager());
    manager->setCookieJar(nullptr);
    for(int i=0;i<urls.size();++i)
    {
        results.append(Reply());
        QUrl queryUrl(urls.at(i));
        if(!queries.isEmpty()) queryUrl.setQuery(queries.at(i));
        QNetworkRequest request;
        request.setUrl(queryUrl);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, redirect);
        request.setMaximumRedirectsAllowed(maxRedirectTimes);

        if(!headers.isEmpty())
        {
            for(int j=0;j<headers[i].size();j+=2)
            {
                request.setRawHeader(headers[i][j].toUtf8(),headers[i][j+1].toUtf8());
            }
        }
        QNetworkReply *reply = manager->get(request);
        QTimer *timer=new QTimer;
        timer->setInterval(timeout*2);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout,reply,&QNetworkReply::abort);
        QObject::connect(getAbortFlag(), &ReqAbortFlagObj::abort, reply,&QNetworkReply::abort);
        timer->start();
        QObject::connect(reply, &QNetworkReply::finished, reply, [i,&results,&finishCount,&eventLoop,reply,timer]()
        {
            if(timer->isActive())
            {
                timer->stop();
                if(reply->error() == QNetworkReply::NoError)
                {
                    results[i].hasError = false;
                    results[i].statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    results[i].content = reply->readAll();
                    results[i].headers = reply->rawHeaderPairs();
                }
                else
                {
                    results[i].hasError = true;
                    results[i].errInfo=reply->errorString();
                }
            }
            else
            {
                results[i].hasError = true;
                results[i].errInfo=QObject::tr("Replay Timeout");
            }
            reply->deleteLater();
            timer->deleteLater();
            finishCount++;
            if(finishCount==results.count()) eventLoop.quit();
        });
    }
    eventLoop.exec();
    return results;
}

int Network::gzipCompress(const QByteArray &input, QByteArray &output)
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
    ret = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                            MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) return ret;
	static QVector<unsigned char> inBuf(chunkSize), outBuf(chunkSize);
    QDataStream inStream(input);
    int flush;
    while(!inStream.atEnd())
    {
        stream.avail_in=inStream.readRawData((char *)inBuf.data(),chunkSize);
        flush = stream.avail_in<chunkSize ? Z_FINISH : Z_NO_FLUSH;
        stream.next_in = inBuf.data();
        do
        {
            stream.avail_out = chunkSize;
            stream.next_out = outBuf.data();
            ret = deflate(&stream, flush);
            assert(ret != Z_STREAM_ERROR);
            have = chunkSize - stream.avail_out;
            output.append((const char *)outBuf.constData(),have);
        } while (stream.avail_out == 0);
    }
    (void)deflateEnd(&stream);
    return Z_OK ;
}

int Network::gzipDecompress(const QByteArray &input, QByteArray &output)
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
    ret = inflateInit2(&stream, MAX_WBITS + 16);
    if (ret != Z_OK) return ret;
	static QVector<unsigned char> inBuf(chunkSize), outBuf(chunkSize);
    static char dummy_head[2] = {
            0x8 + 0x7 * 0x10,
            (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
        };
    QDataStream inStream(input);
    while(!inStream.atEnd())
    {
        stream.avail_in=inStream.readRawData((char *)inBuf.data(),chunkSize);
        if (stream.avail_in == 0)
            break;
        stream.next_in = inBuf.data();
        do
        {
            stream.avail_out = chunkSize;
            stream.next_out = outBuf.data();
            ret = inflate(&stream, Z_NO_FLUSH);
            switch (ret)
            {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
                (void)inflateEnd(&stream);
                return ret;
            case Z_DATA_ERROR:
                stream.next_in = (Bytef*) dummy_head;
                stream.avail_in = sizeof(dummy_head);
                if((ret = inflate(&stream, Z_NO_FLUSH)) == Z_OK)
                    break;
            case Z_MEM_ERROR:
                (void)inflateEnd(&stream);
                return ret;
            }
            have = chunkSize - stream.avail_out;
            output.append((const char *)outBuf.constData(),have);
        } while (stream.avail_out == 0);
    }
    (void)inflateEnd(&stream);
    return Z_OK ;
}

int Network::decompress(const QByteArray &input, QByteArray &output)
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
	static QVector<unsigned char> inBuf(chunkSize), outBuf(chunkSize);
    QDataStream inStream(input);
    while(!inStream.atEnd())
    {
        stream.avail_in=inStream.readRawData((char *)inBuf.data(),chunkSize);
        if (stream.avail_in == 0)
            break;
        stream.next_in = inBuf.data();
        do
        {
            stream.avail_out = chunkSize;
            stream.next_out = outBuf.data();
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
            output.append((const char *)outBuf.constData(),have);
        } while (stream.avail_out == 0);
    }
    (void)inflateEnd(&stream);
    return Z_OK ;
}


Network::Reply Network::httpHead(const QString &url, const QUrlQuery &query, const QStringList &header, bool redirect)
{
    QUrl queryUrl(url);
    if(!query.isEmpty())  queryUrl.setQuery(query);
    QNetworkRequest request;
    request.setUrl(queryUrl);
    if(header.size()>=2)
    {
        Q_ASSERT((header.size() & 1) ==0);
        for(int i=0;i<header.size();i+=2)
        {
            request.setRawHeader(header[i].toUtf8(),header[i+1].toUtf8());
        }
    }
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, redirect);
    request.setMaximumRedirectsAllowed(maxRedirectTimes);

    QNetworkAccessManager *manager = getManager();
    manager->setCookieJar(nullptr);

    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);
    QNetworkReply *reply = manager->head(request);

    QEventLoop eventLoop;
    QObject::connect(getAbortFlag(), &ReqAbortFlagObj::abort, &eventLoop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    timer.start();
    eventLoop.exec();
    Reply replyObj;
    if (timer.isActive())
    {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError)
        {
            replyObj.hasError = false;
            replyObj.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            replyObj.headers = reply->rawHeaderPairs();
        }
        else
        {
            replyObj.hasError = true;
            replyObj.errInfo=reply->errorString();
        }
    }
    else
    {
        QObject::disconnect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        reply->abort();
        replyObj.hasError = true;
        replyObj.errInfo=QObject::tr("Replay Timeout");
    }
    reply->deleteLater();
    return replyObj;
}

Network::ReqAbortFlagObj *Network::getAbortFlag()
{
    if(!abortFlagObjs.hasLocalData())
    {
        abortFlagObjs.setLocalData(QSharedPointer<ReqAbortFlagObj>::create());
    }
    return abortFlagObjs.localData().get();
}

void Network::ReqAbortFlagObj::abortRequest()
{
    emit abort();
}

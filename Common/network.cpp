#include "network.h"
#include "Common/zlib.h"
#include <QNetworkCookie>
#include <QNetworkCookieJar>
namespace
{
    QHash<QThread *,QNetworkAccessManager *> managerMap;
    QMutex locker;
    QNetworkAccessManager *getManager()
    {
        QMutexLocker lock(&locker);
        QNetworkAccessManager *manager=managerMap.value(QThread::currentThread());
        if(!manager)
        {
            manager=new QNetworkAccessManager();
            managerMap.insert(QThread::currentThread(),manager);
        }
        return manager;
    }
}
QByteArray Network::httpGet(const QString &url, const QUrlQuery &query, const QStringList &header, int ttl)
{
    QUrl queryUrl(url);
    if(!query.isEmpty())
        queryUrl.setQuery(query);
    QNetworkRequest request;
    request.setUrl(queryUrl);
    if(header.size()>=2)
    {
        Q_ASSERT((header.size() & 1) ==0);
        for(int i=0;i<header.size();i+=2)
            request.setRawHeader(header[i].toUtf8(),header[i+1].toUtf8());
    }
    QNetworkAccessManager *manager = getManager();

    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);
    QNetworkReply *reply = manager->get(request);

    QEventLoop eventLoop;
	QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    timer.start();
    eventLoop.exec();
    bool hasError=false;
    QString errorInfo;
    QByteArray replyBytes;
    if (timer.isActive())
    {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError)
        {
            int nStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if(nStatusCode==301 || nStatusCode==302)
            {
                if(ttl<=0)
                {
                    throw NetworkError(QObject::tr("Too many redirects"));
                }
                try
                {
                    QString location(reply->header(QNetworkRequest::LocationHeader).toString());
                    if (location.isEmpty())
                        location = reply->rawHeader("Location");
                    QUrl redirectUrl(location);
                    if (redirectUrl.isRelative())
                    {
						QString host(redirectUrl.host());
						if (host.isEmpty()) host = queryUrl.host();
                        QString scheme(queryUrl.scheme());
						int i = 0;
						while (i < location.length() && location[i] == '/') ++i;
						location = location.mid(i);
						if (!location.startsWith(host)) location = QString("%1/%2").arg(host, location);
						if(!scheme.isEmpty()) location = QString("%1://%2").arg(scheme,location);
                    }
                    replyBytes=httpGet(location,QUrlQuery(),header,ttl-1);
                }
                catch(NetworkError &error)
                {
                    hasError=true;
                    errorInfo=error.errorInfo;
                }
            }
            else if(nStatusCode==200)
            {
                replyBytes = reply->readAll();
            }
            else
            {
                hasError=true;
                errorInfo=QObject::tr("Error,Status Code:%1").arg(nStatusCode);
            }
        }
        else
        {
            hasError=true;
            errorInfo=reply->errorString();
        }
    }
    else
    {
        QObject::disconnect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        reply->abort();
        hasError=true;
        errorInfo=QObject::tr("Replay Timeout");
    }
    reply->deleteLater();
    if(hasError)
    {
        throw NetworkError(errorInfo);
    }
    else
    {
        return replyBytes;
    }
}

QByteArray Network::httpPost(const QString &url, const QByteArray &data, const QStringList &header)
{
    QUrl queryUrl(url);
    QNetworkRequest request;
    if(header.size()>=2)
    {
        for(int i=0;i<header.size();i+=2)
            request.setRawHeader(header[i].toUtf8(),header[i+1].toUtf8());
    }
    request.setUrl(queryUrl);
    QNetworkAccessManager *manager = getManager();

    bool hasError=false;
    QString errorInfo;
    QByteArray replyData;

    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);
    QNetworkReply *reply = manager->post(request, data);

    QEventLoop eventLoop;
	QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    timer.start();
    eventLoop.exec();

    if (timer.isActive())
    {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError)
        {
            int nStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if(nStatusCode==200)
            {
                replyData = reply->readAll();
            }
            else
            {
                hasError=true;
                errorInfo=QObject::tr("Error,Status Code:%1").arg(nStatusCode);
            }
        }
        else
        {
            hasError=true;
            errorInfo=reply->errorString();
        }
    }
    else
    {
        QObject::disconnect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        reply->abort();
        hasError=true;
        errorInfo=QObject::tr("Replay Timeout");
    }
    reply->deleteLater();
    if(hasError)
    {
        throw NetworkError(errorInfo);
    }
    else
    {
        return replyData;
    }
}

QList<QPair<QByteArray, QByteArray> > Network::httpHead(const QString &url, const QUrlQuery &query, const QStringList &header)
{
    QUrl queryUrl(url);
    if(!query.isEmpty()) queryUrl.setQuery(query);
    QNetworkRequest request;
    request.setUrl(queryUrl);
    if(header.size()>=2)
    {
        Q_ASSERT((header.size() & 1) ==0);
        for(int i=0;i<header.size();i+=2)
            request.setRawHeader(header[i].toUtf8(),header[i+1].toUtf8());
    }
    QList<QNetworkCookie> cookies;
    QNetworkAccessManager *manager = getManager();
    if(request.hasRawHeader("Cookie"))
    {
        auto cookieBytes = request.rawHeader("Cookie");
        auto rawList = cookieBytes.split(';');
        for (auto &bytes:rawList)
        {
            int pos =bytes.indexOf('=');
            if(pos<=0) continue;
            auto name = bytes.left(pos);
            auto value = bytes.mid(pos+1);
            QNetworkCookie cookie(name, value);
            cookie.setDomain(queryUrl.host());
            cookie.setPath(queryUrl.path());
            cookies<<cookie;
            manager->cookieJar()->insertCookie(cookie);
        }
    }
    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);
    QNetworkReply *reply = manager->head(request);
    for(auto &cookie:cookies)
    {
        manager->cookieJar()->deleteCookie(cookie);
    }
    QEventLoop eventLoop;
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    timer.start();
    eventLoop.exec();
    bool hasError=false;
    QString errorInfo;
    QByteArray replyBytes;
    if (timer.isActive())
    {
        timer.stop();
        if (reply->error() == QNetworkReply::NoError)
        {
            return reply->rawHeaderPairs();
        }
        else
        {
            hasError=true;
            errorInfo=reply->errorString();
        }
    }
    else
    {
        QObject::disconnect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        reply->abort();
        hasError=true;
        errorInfo=QObject::tr("Replay Timeout");
    }
    reply->deleteLater();
    if(hasError)
    {
        throw NetworkError(errorInfo);
    }
    return QList<QNetworkReply::RawHeaderPair>();
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

QList<QPair<QString, QByteArray>> Network::httpGetBatch(const QStringList &urls, const QList<QUrlQuery> &querys, const QList<QStringList> &headers)
{
    Q_ASSERT(urls.size()==querys.size() || querys.size()==0);
    Q_ASSERT(urls.size()==headers.size() || headers.size()==0);
    QList<QPair<QString, QByteArray> > results;
    int finishCount=0;
    QEventLoop eventLoop;
    QNetworkAccessManager *manager(getManager());
    for(int i=0;i<urls.size();++i)
    {
        results.append(QPair<QString,QByteArray>());
        QUrl queryUrl(urls.at(i));
        if(!querys.isEmpty()) queryUrl.setQuery(querys.at(i));
        QNetworkRequest request;
        request.setUrl(queryUrl);
        if(!headers.isEmpty())
        {
            for(int j=0;j<headers[i].size();j+=2)
                request.setRawHeader(headers[i][j].toUtf8(),headers[i][j+1].toUtf8());
        }
        QNetworkReply *reply = manager->get(request);
        QTimer *timer=new QTimer;
        timer->setInterval(timeout*2);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout,reply,&QNetworkReply::abort);
        timer->start();
        QObject::connect(reply, &QNetworkReply::finished, reply, [i,&results,&finishCount,&eventLoop,reply,timer]()
        {
            if(timer->isActive())
            {
                timer->stop();
                int nStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if(reply->error() == QNetworkReply::NoError)
                {
                    if(nStatusCode==200)
                        results[i].second = reply->readAll();
                    else
                        results[i].first = QObject::tr("Error,Status Code:%1").arg(nStatusCode);
                }
                else
                {
                    results[i].first = reply->errorString();
                }
            }
            else
            {
                results[i].first = QObject::tr("Replay Timeout");
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
    unsigned char inBuf[chunkSize];
    unsigned char outBuf[chunkSize];
    QDataStream inStream(input);
    int flush;
    while(!inStream.atEnd())
    {
        stream.avail_in=inStream.readRawData((char *)&inBuf,chunkSize);
        flush = stream.avail_in<chunkSize ? Z_FINISH : Z_NO_FLUSH;
        stream.next_in = inBuf;
        do
        {
            stream.avail_out = chunkSize;
            stream.next_out = outBuf;
            ret = deflate(&stream, flush);
            assert(ret != Z_STREAM_ERROR);
            have = chunkSize - stream.avail_out;
            output.append((const char *)outBuf,have);
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
    unsigned char inBuf[chunkSize];
    unsigned char outBuf[chunkSize];
    static char dummy_head[2] = {
            0x8 + 0x7 * 0x10,
            (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
        };
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
            output.append((const char *)outBuf,have);
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

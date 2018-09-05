#include "network.h"

QByteArray Network::httpGet(const QString &url, QUrlQuery &query, QStringList &header)
{
    QUrl queryUrl(url);
    if(!query.isEmpty())
        queryUrl.setQuery(query);
    QNetworkRequest request;
    request.setUrl(queryUrl);
    if(header.size()>=2)
    {
        for(int i=0;i<header.size();i+=2)
            request.setRawHeader(header[i].toUtf8(),header[i+1].toUtf8());
    }

    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(request);
    QEventLoop eventLoop;
	QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(&manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);
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
                try
                {
                    QString location(reply->header(QNetworkRequest::LocationHeader).toString());
                    if (location.isEmpty())
                        location = reply->rawHeader("Location");
                    QUrl redirectUrl(location);
                    if (redirectUrl.isRelative())
                    {
                        QString host(queryUrl.host());
                        QString scheme(queryUrl.scheme());
                        if (!location.startsWith('/'))
                            location.push_front('/');
                        location = QString("%1://%2%3").arg(scheme).arg(host).arg(location);
                    }
                    replyBytes=httpGet(location,QUrlQuery(),header);
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

QByteArray Network::httpPost(const QString &url, QByteArray &data, QStringList &header)
{
    QUrl queryUrl(url);
    QNetworkRequest request;
    if(header.size()>=2)
    {
        for(int i=0;i<header.size();i+=2)
            request.setRawHeader(header[i].toUtf8(),header[i+1].toUtf8());
    }
    request.setUrl(queryUrl);

    bool hasError=false;
    QString errorInfo;
    QByteArray replyData;

    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, data);
    QEventLoop eventLoop;
	QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(&manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);
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
    for(QString key:pathList)
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

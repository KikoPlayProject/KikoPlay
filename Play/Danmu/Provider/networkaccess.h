#ifndef NETWORKACCESS_H
#define NETWORKACCESS_H
#include <QtCore>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
class NetworkError
{
public:
    NetworkError(QString info):errorInfo(info){}
    QString errorInfo;
};
class NetworkAccess
{
    static const int timeout=10000;
public:
    static QString HttpGetStr(QString url,QUrlQuery query)
    {
        QUrl queryUrl(url);
        queryUrl.setQuery(query);

        QNetworkRequest request;
        request.setUrl(queryUrl);

        QTimer timer;
        timer.setInterval(timeout);
        timer.setSingleShot(true);
        QNetworkAccessManager manager;
        QNetworkReply *reply = manager.get(request);
        QEventLoop eventLoop;
        QObject::connect(&manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);
        timer.start();
        eventLoop.exec();
        bool hasError=false;
        QString errorInfo;
        QString replyStr;
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
                        replyStr=HttpGetStr(reply->header(QNetworkRequest::LocationHeader).toString(),QUrlQuery());
                    }
                    catch(NetworkError &error)
                    {
                        hasError=true;
                        errorInfo=error.errorInfo;
                    }
                }
                else if(nStatusCode==200)
                {
                    replyStr = reply->readAll();
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
            return replyStr;
        }
    }
    static QJsonDocument HttpGetJson(QString url,QUrlQuery query)
    {
        QUrl queryUrl(url);
        queryUrl.setQuery(query);

        QNetworkRequest request;
        request.setRawHeader("Accept", "application/json");
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        request.setUrl(queryUrl);

        QTimer timer;
        timer.setInterval(timeout);
        timer.setSingleShot(true);
        QNetworkAccessManager manager;
        QNetworkReply *reply = manager.get(request);
        QEventLoop eventLoop;
        QObject::connect(&manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);
        timer.start();
        eventLoop.exec();
        bool hasError=false;
        QString errorInfo;
        QJsonDocument doucment;
        if (timer.isActive())
        {
            timer.stop();
            if (reply->error() == QNetworkReply::NoError)
            {
                int nStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if(nStatusCode!=200)
                {
                    hasError=true;
                    errorInfo=QObject::tr("Error,Status Code:%1").arg(nStatusCode);
                }
                else
                {
                    QByteArray bytes = reply->readAll();
                    QJsonParseError jsonError;
                    doucment = QJsonDocument::fromJson(bytes, &jsonError);
                    if (jsonError.error != QJsonParseError::NoError)
                    {
                        hasError=true;
                        errorInfo=QObject::tr("Decode Reply JSON Failed");
                    }
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
            return doucment;
        }
    }
};


#endif // NETWORKACCESS_H

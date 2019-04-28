#include "aria2jsonrpc.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtCore>

Aria2JsonRPC::Aria2JsonRPC(QObject *parent) : QObject(parent)
{
    QProcess *process = new QProcess(this);
#ifdef Q_OS_WIN
    process->start("taskkill /im aria2c.exe /f");
#else
    process->start("pkill -f aria2c");
#endif
    process->waitForFinished(-1);
    QStringList args;
    args << "--enable-rpc=true";
    args << "--rpc-listen-port=6800";
#ifdef Q_OS_WIN
    process->start(QCoreApplication::applicationDirPath()+"\\aria2c.exe", args);
#else
    process->start(QCoreApplication::applicationDirPath()+"/aria2c", args);

#endif
    process->waitForStarted(-1);
}

QJsonObject Aria2JsonRPC::rpcCall(const QString &method, const QJsonArray &params, const QString &id, bool async)
{
    QJsonObject object;
    object.insert("jsonrpc", "2.0");
    object.insert("id", id);
    object.insert("method", method);
    if (!params.isEmpty())
    {
        object.insert("params", params);
    }

    QNetworkRequest request;
    request.setUrl(QUrl("http://localhost:6800/jsonrpc"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray content(QJsonDocument(object).toJson(QJsonDocument::Compact));
    QNetworkReply *reply = manager.post(request, content);

    if(async)
    {
        QObject::connect(reply, &QNetworkReply::finished, [this,reply,method](){
            if (reply->error() == QNetworkReply::NoError)
            {
                QByteArray buffer = reply->readAll();
                QJsonDocument document = QJsonDocument::fromJson(buffer);
                handleRPCReply(method,document.object());
            }
            else
            {
                emit showLog(tr("RPC Reply Error: %1").arg(method));
            }
            reply->deleteLater();
        });
        return QJsonObject();
    }
    else
    {
        QEventLoop eventLoop;
        QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray buffer = reply->readAll();
            reply->deleteLater();
            QJsonDocument document = QJsonDocument::fromJson(buffer);
            QJsonObject replyObj(document.object());
            if(replyObj.contains("error")) throw RPCError(replyObj.value("error").toString());
            return replyObj;
        }
        else
        {
            reply->deleteLater();
            throw RPCError(tr("RPC Reply Error: %1").arg(method));
        }
    }
}

void Aria2JsonRPC::handleRPCReply(const QString &method, const QJsonObject &replyObj)
{
    if(replyObj.contains("error")) return;
    if(method=="aria2.addUri" || method=="aria2.addTorrent")
    {
        emit addNewTask(replyObj.value("id").toString(),replyObj.value("result").toString());
    }
    else if(method=="aria2.tellStatus")
    {
        QJsonObject statusObj(replyObj.value("result").toObject());
        QString errMsg(statusObj.value("errorMessage").toString());
        if(!errMsg.isEmpty())
            emit showLog(errMsg);
        emit refreshStatus(statusObj);
    }
    else if(method=="aria2.getGlobalStat")
    {
        QJsonObject resultObj(replyObj.value("result").toObject());
        emit refreshGlobalStatus(resultObj.value("downloadSpeed").toString().toInt(),
                                 resultObj.value("uploadSpeed").toString().toInt(),
                                 resultObj.value("numActive").toString().toInt());
    }
}

QString Aria2JsonRPC::addUri(const QString &uri, const QJsonObject &options)
{
    QJsonArray params,uris;
    uris.append(uri);
    params.append(uris);
    params.append(options);
    QJsonObject gidObj(rpcCall("aria2.addUri",params,"",false));
    return gidObj.value("result").toString();
}

QString Aria2JsonRPC::addTorrent(const QString &base64Str, const QJsonObject &options)
{
    QJsonArray params;
    params.append(base64Str);
    params.append(QJsonArray());
    params.append(options);
    QJsonObject gidObj(rpcCall("aria2.addTorrent",params,"",false));
    return gidObj.value("result").toString();
}

void Aria2JsonRPC::tellStatus(const QString &gid)
{
    QJsonArray params;
    params.append(gid);
    rpcCall("aria2.tellStatus",params,"");
}

void Aria2JsonRPC::switchPauseStatus(const QString &gid, bool pause)
{
    QJsonArray params;
    params.append(gid);
    rpcCall(pause?"aria2.forcePause":"aria2.unpause",params,"",true);
}

void Aria2JsonRPC::tellGlobalStatus()
{
    QJsonArray params;
    rpcCall("aria2.getGlobalStat",params,"");
}

void Aria2JsonRPC::removeTask(const QString &gid)
{
    QJsonArray params;
    params.append(gid);
    rpcCall("aria2.forceRemove",params,"");
}

void Aria2JsonRPC::switchAllPauseStatus(bool pause)
{
    QJsonArray params;
    rpcCall(pause?"aria2.forcePauseAll":"aria2.unpauseAll",params,"");
}

void Aria2JsonRPC::changeOption(const QString &gid, const QJsonObject &options)
{
    QJsonArray params;
    params.append(gid);
    params.append(options);
    rpcCall("aria2.changeOption",params,"");
}

void Aria2JsonRPC::changeGlobalOption(const QJsonObject &options)
{
    QJsonArray params;
    params.append(options);
    rpcCall("aria2.changeGlobalOption",params,"");
}

#include "aria2jsonrpc.h"
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtCore>
#include "Common/logger.h"
#include "globalobjects.h"

Aria2JsonRPC::Aria2JsonRPC(QObject *parent) : QObject(parent)
{
    aria2Process = new QProcess(this);
#ifdef Q_OS_WIN
    aria2Process->start("taskkill /im aria2c.exe /f");
#else
    aria2Process->start("pkill -f aria2c");
#endif
    aria2Process->waitForFinished(-1);
    QStringList opts=GlobalObjects::appSetting->value("Download/Aria2Args","").toString().split('\n');
    QStringList args;
    for(const QString &option:opts)
    {
        QString opt(option.trimmed());
		if (opt.isEmpty()) continue;
        if(opt.startsWith('#'))continue;
        if(!opt.startsWith("--")) opt = "--"+opt;
        args<<opt;
    }
    args << "--enable-rpc=true";
    args << "--rpc-listen-port=6800";
    //args << "rpc-save-upload-metadata","false");


#ifdef Q_OS_WIN
    aria2Process->start(QCoreApplication::applicationDirPath()+"\\aria2c.exe", args);
#else
    QFileInfo check_file(QCoreApplication::applicationDirPath()+"/aria2c");
    /* check if file exists and if yes: Is it really a file and no directory? */
    if (check_file.exists() && check_file.isFile()) {
        aria2Process->start(QCoreApplication::applicationDirPath()+"/aria2c", args);
    } else {
        aria2Process->start("aria2c", args);
    }

#endif
    aria2Process->waitForStarted(-1);
}

void Aria2JsonRPC::exit()
{
    QJsonArray params;
    rpcCall("aria2.shutdown",params,"");
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
                Logger::logger()->log(Logger::LogType::Aria2, QString("[%1]RPC Network Error: %2").arg(method, reply->errorString()));
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
            throw RPCError(QString("RPC Reply Error: %1").arg(method));
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
            Logger::logger()->log(Logger::LogType::Aria2, QString("[%1]%2").arg(method, errMsg));
        emit refreshStatus(statusObj);
    }
    else if(method=="aria2.getGlobalStat")
    {
        QJsonObject resultObj(replyObj.value("result").toObject());
        emit refreshGlobalStatus(resultObj.value("downloadSpeed").toString().toInt(),
                                 resultObj.value("uploadSpeed").toString().toInt(),
                                 resultObj.value("numActive").toString().toInt());
    }
    else if(method=="aria2.getPeers")
    {
        QJsonArray resultArray(replyObj.value("result").toArray());
        emit refreshPeerStatus(resultArray);
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

void Aria2JsonRPC::getPeers(const QString &gid)
{
    QJsonArray params;
    params.append(gid);
    rpcCall("aria2.getPeers",params,"");
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

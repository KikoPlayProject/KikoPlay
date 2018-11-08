#include "httpserver.h"
#include "qhttpengine/filesystemhandler.h"
#include "qhttpengine/qobjecthandler.h"
#include "qhttpengine/handler.h"
#include "qhttpengine/qiodevicecopier.h"
#include "qhttpengine/range.h"

#include "Play/Playlist/playlist.h"
#include "Play/Danmu/common.h"
#include "Play/Danmu/blocker.h"
#include "globalobjects.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QCoreApplication>
#include <QMimeDatabase>
namespace
{
    class MediaFileHandler : public QHttpEngine::FilesystemHandler
    {
    public:
        explicit MediaFileHandler(const QHash<QString,QString> *mediaHash,QObject *parent = 0):
            FilesystemHandler(parent),mediaHashTable(mediaHash){}

        // FilesystemHandler interface
    protected:
        virtual void process(QHttpEngine::Socket *socket, const QString &path)
        {
            if(path.startsWith("media/"))
            {
                QString mediaId(path.mid(6).trimmed());
                QString mediaPath(mediaHashTable->value(mediaId,""));
                if(mediaPath.isEmpty())
                {
                    socket->writeError(QHttpEngine::Socket::NotFound);
                }
                else
                {
                    processFile(socket, mediaPath);
                }
            }
            else
            {
                FilesystemHandler::process(socket,path);
            }
        }
    private:
        const QHash<QString,QString> *mediaHashTable;
        QMimeDatabase database;
        void processFile(QHttpEngine::Socket *socket, const QString &absolutePath)
        {
            // Attempt to open the file for reading
            QFile *file = new QFile(absolutePath);
            if (!file->open(QIODevice::ReadOnly)) {
                delete file;

                socket->writeError(QHttpEngine::Socket::Forbidden);
                return;
            }

            // Create a QIODeviceCopier to copy the file contents to the socket
            QHttpEngine::QIODeviceCopier *copier = new QHttpEngine::QIODeviceCopier(file, socket);
            connect(copier, &QHttpEngine::QIODeviceCopier::finished, copier, &QHttpEngine::QIODeviceCopier::deleteLater);
            connect(copier, &QHttpEngine::QIODeviceCopier::finished, file, &QFile::deleteLater);
            connect(copier, &QHttpEngine::QIODeviceCopier::finished, [socket]() {
                socket->close();
            });

            // Stop the copier if the socket is disconnected
            connect(socket, &QHttpEngine::Socket::disconnected, copier, &QHttpEngine::QIODeviceCopier::stop);

            qint64 fileSize = file->size();

            // Checking for partial content request
            QByteArray rangeHeader = socket->headers().value("Range");
            QHttpEngine::Range range;

            if (!rangeHeader.isEmpty() && rangeHeader.startsWith("bytes=")) {
                // Skiping 'bytes=' - first 6 chars and spliting ranges by comma
                QList<QByteArray> rangeList = rangeHeader.mid(6).split(',');

                // Taking only first range, as multiple ranges require multipart
                // reply support
                range = QHttpEngine::Range(QString(rangeList.at(0)), fileSize);
            }

            // If range is valid, send partial content
            if (range.isValid()) {
                socket->setStatusCode(QHttpEngine::Socket::PartialContent);
                socket->setHeader("Content-Length", QByteArray::number(range.length()));
                socket->setHeader("Content-Range", QByteArray("bytes ") + range.contentRange().toLatin1());
                copier->setRange(range.from(), range.to());
            } else {
                // If range is invalid or if it is not a partial content request,
                // send full file
                socket->setHeader("Content-Length", QByteArray::number(fileSize));
            }

            // Set the mimetype and content length
            socket->setHeader("Content-Type", mimeType(absolutePath));
            socket->writeHeaders();

            // Start the copy
            copier->start();
        }

        QByteArray mimeType(const QString &absolutePath)
        {
            // Query the MIME database based on the filename and its contents
            return database.mimeTypeForFile(absolutePath).name().toUtf8();
        }
    };
}
HttpServer::HttpServer(QObject *parent) : QObject(parent)
{
    MediaFileHandler *handler=new MediaFileHandler(&mediaHash, this);
    handler->setDocumentRoot(QCoreApplication::applicationDirPath()+"/web");
    handler->addRedirect(QRegExp("^$"), "/index.html");

    QHttpEngine::QObjectHandler *apiHandler=new QHttpEngine::QObjectHandler(this);
    apiHandler->registerMethod("playlist", this, &HttpServer::api_Playlist);
    apiHandler->registerMethod("updateTime", this, &HttpServer::api_UpdateTime);
    apiHandler->registerMethod("danmu/v3/", this, &HttpServer::api_Danmu);
    handler->addSubHandler(QRegExp("api/"), apiHandler);

    server = new QHttpEngine::Server(handler);
}

HttpServer::~HttpServer()
{
    server->close();
}

QString HttpServer::startServer(qint64 port)
{

   if(server->isListening())return QString();
   server->listen(QHostAddress::AnyIPv4,port);
   genLog(server->errorString().isEmpty()?"Server start":server->errorString());
   return server->errorString();
}

void HttpServer::stopServer()
{
    server->close();
    genLog("Server close");
}

void HttpServer::genLog(const QString &logInfo)
{
    emit showLog(QString("%1%2").arg(QTime::currentTime().toString("[hh:mm:ss]")).arg(logInfo));
}

void HttpServer::api_Playlist(QHttpEngine::Socket *socket)
{
    QMetaObject::invokeMethod(GlobalObjects::playlist,[this](){
        GlobalObjects::playlist->dumpJsonPlaylist(playlistDoc,mediaHash);
    },Qt::BlockingQueuedConnection);
    genLog(QString("[%1]Request:Playlist").arg(socket->peerAddress().toString()));
    QByteArray data = playlistDoc.toJson();
    socket->setHeader("Content-Length", QByteArray::number(data.length()));
    socket->setHeader("Content-Type", "application/json");
    socket->writeHeaders();
    socket->write(data);
    socket->close();
}

void HttpServer::api_UpdateTime(QHttpEngine::Socket *socket)
{
    bool syncPlayTime=GlobalObjects::appSetting->value("Server/SyncPlayTime",true).toBool();
    if(syncPlayTime)
    {
        QJsonDocument document;
        if (socket->readJson(document))
        {
            genLog(QString("[%1]Request:UpdateTime").arg(socket->peerAddress().toString()));
            QVariantMap data = document.object().toVariantMap();
            QString mediaPath=mediaHash.value(data.value("mediaId").toString());
            int playTime=data.value("playTime").toInt();
            int playTimeState=data.value("playTimeState").toInt();
            QMetaObject::invokeMethod(GlobalObjects::playlist,[mediaPath,playTime,playTimeState](){
                GlobalObjects::playlist->updatePlayTime(mediaPath,playTime,playTimeState);
            },Qt::QueuedConnection);
        }
    }
    socket->close();
}

void HttpServer::api_Danmu(QHttpEngine::Socket *socket)
{ 
    QString poolId=socket->queryString().value("id");
    genLog(QString("[%1]Request:Danmu").arg(socket->peerAddress().toString()));
    QSqlQuery query(QSqlDatabase::database("WT"));
    QHash<int,DanmuSourceInfo> sourcesTable;
    query.exec(QString("select ID,Delay,TimeLine from source where PoolID='%1'").arg(poolId));
    int idNo = query.record().indexOf("ID"),
            delayNo=query.record().indexOf("Delay"),
            timelineNo=query.record().indexOf("TimeLine");
    while (query.next())
    {
        DanmuSourceInfo sourceInfo;
        sourceInfo.delay=query.value(delayNo).toInt();
        sourceInfo.id=query.value(idNo).toInt();
        QStringList timelineList(query.value(timelineNo).toString().split(';',QString::SkipEmptyParts));
        QTextStream ts;
        for(QString &spaceInfo:timelineList)
        {
            ts.setString(&spaceInfo,QIODevice::ReadOnly);
            int start,duration;
            ts>>start>>duration;
            sourceInfo.timelineInfo.append(QPair<int,int>(start,duration));
        }
        sourcesTable.insert(sourceInfo.id,sourceInfo);
    }
    DanmuComment tmpComment;
    QJsonArray danmuArray;
    query.exec(QString("select * from danmu where PoolID='%1'").arg(poolId));
    int timeNo = query.record().indexOf("Time"),
            colorNo=query.record().indexOf("Color"),
            modeNo=query.record().indexOf("Mode"),
            sourceNo=query.record().indexOf("Source"),
            userNo=query.record().indexOf("User"),
            textNo=query.record().indexOf("Text");
    while (query.next())
    {
        tmpComment.color=query.value(colorNo).toInt();
        tmpComment.sender=query.value(userNo).toString();
        tmpComment.type=DanmuComment::DanmuType(query.value(modeNo).toInt());
        tmpComment.source=query.value(sourceNo).toInt();
        tmpComment.text=query.value(textNo).toString();
        tmpComment.originTime=query.value(timeNo).toInt();
        if(GlobalObjects::blocker->isBlocked(&tmpComment))continue;
        int delay=0;
        if(sourcesTable.contains(tmpComment.source))
        {
            for(auto &spaceItem:sourcesTable[tmpComment.source].timelineInfo)
            {
                if(tmpComment.originTime>spaceItem.first)delay+=spaceItem.second;
                else break;
            }
            delay+=sourcesTable[tmpComment.source].delay;
        }
        tmpComment.time=tmpComment.originTime+delay<0?tmpComment.originTime:tmpComment.originTime+delay;
        QJsonArray danmuObj={tmpComment.time/1000.f,tmpComment.type,tmpComment.color,tmpComment.sender,tmpComment.text};
        danmuArray.append(danmuObj);
    }
    QJsonObject resposeObj
    {
        {"code", 0},
        {"data", danmuArray}
    };
    QByteArray data = QJsonDocument(resposeObj).toJson();
    socket->setHeader("Content-Length", QByteArray::number(data.length()));
    socket->setHeader("Content-Type", "application/json");
    socket->writeHeaders();
    socket->write(data);

    socket->close();
}

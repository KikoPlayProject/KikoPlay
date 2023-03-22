#include "apihandler.h"
#include "globalobjects.h"
#include "Play/Playlist/playlist.h"
#include "Common/network.h"
#include "Common/logger.h"
#include "Play/Danmu/common.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/Provider/localprovider.h"
#include "Play/Danmu/blocker.h"
#include "Script/scriptmanager.h"
#include "Script/danmuscript.h"
#include "MediaLibrary/animeworker.h"

namespace
{
    const QString duration2Str(int duration, bool hour = false)
    {
        int cmin=duration/60;
        int cls=duration-cmin*60;
        if(hour)
        {
            int ch = cmin/60;
            cmin = cmin - ch*60;
            return QString("%1:%2:%3").arg(ch, 2, 10, QChar('0')).arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0'));
        }
        else
        {
            return QString("%1:%2").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0'));
        }
    }
}

APIHandler::APIHandler(QObject *parent) : stefanfrings::HttpRequestHandler(parent)
{

}

void APIHandler::service(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QByteArray path = request.getPath();
    if(!path.startsWith("/api/"))
    {
        response.setStatus(stefanfrings::HttpResponse::BadRequest);
        return;
    }
    path = path.mid(5);
    using Func = void(APIHandler::*)(stefanfrings::HttpRequest &, stefanfrings::HttpResponse &);
    static QMap<QString, Func> routeTable = {
        {"playlist", &APIHandler::apiPlaylist},
        {"updateTime", &APIHandler::apiUpdateTime},
        {"subtitle", &APIHandler::apiSubtitle},
        {"updateDelay", &APIHandler::apiUpdateDelay},
        {"updateTimeline", &APIHandler::apiUpdateTimeline},
        {"screenshot", &APIHandler::apiScreenshot},
        {"danmu/v3/", &APIHandler::apiDanmu},
        {"danmu/full/", &APIHandler::apiDanmuFull},
        {"danmu/local/", &APIHandler::apiLocalDanmu},
        {"danmu/launch", &APIHandler::apiLaunch}
    };
    auto api = routeTable.value(path, nullptr);
    if(!api)
    {
        response.setStatus(stefanfrings::HttpResponse::BadRequest);
        return;
    }
    (this->*api)(request, response);
}

void APIHandler::apiPlaylist(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QMetaObject::invokeMethod(GlobalObjects::playlist,[this](){
        GlobalObjects::playlist->dumpJsonPlaylist(playlistDoc);
    },Qt::BlockingQueuedConnection);
    Logger::logger()->log(Logger::LANServer, QString("[%1]Playlist").arg(request.getPeerAddress().toString()));

    QByteArray data = playlistDoc.toJson();
    QByteArray compressedBytes;
    Network::gzipCompress(data,compressedBytes);
    response.setHeader("Content-Type", "application/json");
    response.setHeader("Content-Encoding", "gzip");
    response.write(compressedBytes, true);
}

void APIHandler::apiUpdateTime(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(response)
    bool syncPlayTime=GlobalObjects::appSetting->value("Server/SyncPlayTime",true).toBool();
    if(syncPlayTime)
    {
        QJsonDocument document;
        if(readJson(request.getBody(), document))
        {
            Logger::logger()->log(Logger::LANServer, QString("[%1]UpdateTime").arg(request.getPeerAddress().toString()));
            QVariantMap data = document.object().toVariantMap();
            QString mediaPath = GlobalObjects::playlist->getPathByHash(data.value("mediaId").toString());
            if(!mediaPath.isEmpty())
            {
                int playTime=data.value("playTime").toInt();
                PlayListItem::PlayState playTimeState=PlayListItem::PlayState(data.value("playTimeState").toInt());
                QMetaObject::invokeMethod(GlobalObjects::playlist,[mediaPath,playTime,playTimeState](){
                    GlobalObjects::playlist->updatePlayTime(mediaPath,playTime,playTimeState);
                },Qt::QueuedConnection);
            }
        }
    }
}

void APIHandler::apiDanmu(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QString poolId = request.getParameter("id");
    bool update = (request.getParameter("update").toLower()=="true");
    Pool *pool = GlobalObjects::danmuManager->getPool(poolId);
    Logger::logger()->log(Logger::LANServer,
                          QString("[%1]Danmu %2%3").arg(request.getPeerAddress().toString(),
                          pool?pool->epTitle():"",
                          update?", update=true":""));
    QJsonArray danmuArray;
    if(pool)
    {
        if(update)
        {
            QVector<QSharedPointer<DanmuComment> > incList;
            pool->update(-1,&incList);
            danmuArray=Pool::exportJson(incList);
        }
        else
        {
            danmuArray=pool->exportJson();
        }
    }
    QJsonObject resposeObj
    {
        {"code", 0},
        {"data", danmuArray},
        {"update",update}
    };
    QByteArray data = QJsonDocument(resposeObj).toJson();
    QByteArray compressedBytes;
    Network::gzipCompress(data,compressedBytes);
    response.setHeader("Content-Type", "application/json");
    response.setHeader("Content-Encoding", "gzip");
    response.write(compressedBytes, true);
}

void APIHandler::apiDanmuFull(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QString poolId = request.getParameter("id");
    bool update = (request.getParameter("update").toLower()=="true");
    Pool *pool = GlobalObjects::danmuManager->getPool(poolId);
    Logger::logger()->log(Logger::LANServer,
                          QString("[%1]Danmu(Full) %2%3").arg(request.getPeerAddress().toString(),
                          pool?pool->epTitle():"",
                          update?", update=true":""));
    QJsonObject resposeObj;
    if(pool)
    {
        if(update)
        {
            QVector<QSharedPointer<DanmuComment> > incList;
            pool->update(-1,&incList);
            resposeObj=
            {
                {"comment", Pool::exportJson(incList, true)},
                {"update", true}
            };
        }
        else
        {
            resposeObj=pool->exportFullJson();
            resposeObj.insert("update", false);
            if(pool->sources().size()>0)
            {
                QJsonArray supportedScripts;
                QList<DanmuSource> sources;
                for(auto &src : pool->sources())
                    sources.append(src);
                for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::DANMU))
                {
                    DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
                    bool ret = false;
                    dmScript->hasSourceToLaunch(sources, ret);
                    if(ret) supportedScripts.append(dmScript->id());
                }
                if(supportedScripts.size()>0)
                    resposeObj.insert("launchScripts", supportedScripts);
            }
        }
    }
    QByteArray data = QJsonDocument(resposeObj).toJson();
    QByteArray compressedBytes;
    Network::gzipCompress(data,compressedBytes);
    response.setHeader("Content-Type", "application/json");
    response.setHeader("Content-Encoding", "gzip");
    response.write(compressedBytes, true);
}

void APIHandler::apiLocalDanmu(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QString mediaId = request.getParameter("mediaId");
    QString mediaPath(GlobalObjects::playlist->getPathByHash(mediaId));
    Logger::logger()->log(Logger::LANServer,
                          QString("[%1]Danmu(Local) %2").arg(request.getPeerAddress().toString(),
                          mediaPath));
    if(mediaPath.isEmpty())
    {
        response.setStatus(stefanfrings::HttpResponse::NotFound);
        return;
    }
    QString danmuFile(mediaPath.mid(0, mediaPath.lastIndexOf('.'))+".xml");
    QFileInfo fi(danmuFile);
    QJsonObject resposeObj;
    if(fi.exists())
    {
        QVector<DanmuComment *> tmplist;
        LocalProvider::LoadXmlDanmuFile(danmuFile, tmplist);
        GlobalObjects::blocker->checkDanmu(tmplist.begin(), tmplist.end(), false);
        resposeObj=
        {
            {"comment", Pool::exportJson(tmplist, false)},
            {"local", danmuFile}
        };
    }
    QByteArray data = QJsonDocument(resposeObj).toJson();
    QByteArray compressedBytes;
    Network::gzipCompress(data,compressedBytes);
    response.setHeader("Content-Type", "application/json");
    response.setHeader("Content-Encoding", "gzip");
    response.write(compressedBytes, true);

}

void APIHandler::apiUpdateDelay(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(response)
    QJsonDocument document;
    if (readJson(request.getBody(), document))
    {
        QVariantMap data = document.object().toVariantMap();
        QString poolId=data.value("danmuPool").toString();
        int delay=data.value("delay").toInt();  //ms
        int sourceId=data.value("source").toInt();
        Logger::logger()->log(Logger::LANServer, QString("[%1]UpdateDelay, SourceId: %2").arg(request.getPeerAddress().toString(),QString::number(sourceId)));
        Pool *pool=GlobalObjects::danmuManager->getPool(poolId,false);
        if(pool) pool->setDelay(sourceId, delay);
    }
}

void APIHandler::apiUpdateTimeline(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(response)
    QJsonDocument document;
    if (readJson(request.getBody(), document))
    {
        QVariantMap data = document.object().toVariantMap();
        QString poolId=data.value("danmuPool").toString();
        QString timelineStr=data.value("timeline").toString();
        int sourceId=data.value("source").toInt();
        Logger::logger()->log(Logger::LANServer, QString("[%1]UpdateTimeline, SourceId: %2").arg(request.getPeerAddress().toString(),QString::number(sourceId)));
        Pool *pool=GlobalObjects::danmuManager->getPool(poolId,false);
        DanmuSource srcInfo;
        srcInfo.setTimeline(timelineStr);
        if(pool) pool->setTimeline(sourceId, srcInfo.timelineInfo);
    }
}

void APIHandler::apiSubtitle(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QString mediaId = request.getParameter("id");
    QString mediaPath = GlobalObjects::playlist->getPathByHash(mediaId);
    if(mediaPath.isEmpty()) return;
    QFileInfo fi(mediaPath);
    QString dir=fi.absolutePath(), name=fi.fileName();
    name = name.mid(0, name.lastIndexOf('.'));
    static QStringList supportedSubFormats={"","ass","ssa","srt"};
    Logger::logger()->log(Logger::LANServer, QString("[%1]Subtitle - %2").arg(request.getPeerAddress().toString(),name));
    int formatIndex = 0;
    for(int i = 1; i < supportedSubFormats.size(); ++i)
    {
        QFileInfo subInfo(dir, name+"."+supportedSubFormats[i]);
        if(subInfo.exists())
        {
            formatIndex=i;
            break;
        }
    }
    QJsonObject resposeObj
    {
        {"type", supportedSubFormats[formatIndex]}
    };
    QByteArray data = QJsonDocument(resposeObj).toJson();
    response.setHeader("Content-Type", "application/json");
    response.write(data, true);
}

void APIHandler::apiScreenshot(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(response)
    QJsonDocument document;
    if(readJson(request.getBody(), document))
    {
        QVariantMap data = document.object().toVariantMap();
        QString animeName=data.value("animeName").toString();
        double pos = data.value("pos").toDouble();  //s
        QString mediaId = data.value("mediaId").toString();
        QString mediaPath = GlobalObjects::playlist->getPathByHash(mediaId);
        QFileInfo fi(mediaPath);
        if(fi.exists() && !animeName.isEmpty())
        {
            QTemporaryFile tmpImg("XXXXXX.jpg");
            if(tmpImg.open())
            {
                int cmin=pos/60;
                int cls=pos-cmin*60;
                QString posStr(QString("%1:%2").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')));

                QString ffmpegPath = GlobalObjects::appSetting->value("Play/FFmpeg", "ffmpeg").toString();
                QStringList arguments;
                arguments << "-ss" << QString::number(pos);
                arguments << "-i" << mediaPath;
                arguments << "-vframes" << "1";
                arguments << "-y";
                arguments << tmpImg.fileName();

                QProcess ffmpegProcess;
                bool success = true;
                QString errorInfo;
                QObject::connect(&ffmpegProcess, &QProcess::errorOccurred, this, [&errorInfo, &success](QProcess::ProcessError error){
                    if(error == QProcess::FailedToStart)
                    {
                        errorInfo = tr("Start FFmpeg Failed");
                    }
                   success = false;
                });
                QObject::connect(&ffmpegProcess, (void (QProcess:: *)(int, QProcess::ExitStatus))&QProcess::finished, this,
                                 [&errorInfo, &success](int exitCode, QProcess::ExitStatus exitStatus){
                   success = (exitStatus == QProcess::NormalExit && exitCode == 0);
                   if(!success)
                   {
                       errorInfo = tr("Generate Failed, FFmpeg exit code: %1").arg(exitCode);
                   }
                });
                QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardOutput, this, [&](){
                   qInfo()<<ffmpegProcess.readAllStandardOutput();
                });
                QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardError, this, [&]() {
                    QString content(ffmpegProcess.readAllStandardError());
                    qInfo() << content.replace("\\n", "\n");
                });

				ffmpegProcess.start(ffmpegPath, arguments);
				ffmpegProcess.waitForFinished(-1);
                if(success)
                {
                    QImage captureImage(tmpImg.fileName());
                    if(data.contains("duration")) //snippet task
                    {
                        qint64 timeId = QDateTime::currentDateTime().toMSecsSinceEpoch();
                        QString snippetPath(GlobalObjects::appSetting->value("Play/SnippetPath", GlobalObjects::dataPath + "/snippet").toString());
                        QDir dir;
                        if(!dir.exists(snippetPath)) dir.mkpath(snippetPath);
                        QString fileName(QString("%1/%2.%3").arg(snippetPath, QString::number(timeId), fi.suffix()));
                        QString duration = QString::number(qBound<int>(1, data.value("duration", 1).toInt(), 15));
                        QStringList arguments;
                        arguments << "-ss" << QString::number(pos);
                        arguments << "-i" << mediaPath;
                        arguments << "-t" << duration;
                        if(!data.value("retainAudio", true).toBool())
                            arguments << "-an";
                        arguments << "-y";
                        arguments << fileName;
                        ffmpegProcess.start(ffmpegPath, arguments);
                        ffmpegProcess.waitForFinished(-1);
                        if(success)
                        {
                            Logger::logger()->log(Logger::LANServer, QString("[%1]Snippet,[%2]%3").arg(request.getPeerAddress().toString(),posStr, fi.filePath()));
                            QString info = data.value("info").toString();
                            if(info.isEmpty())  info = QString("%1,%2s - %3").arg(duration2Str(pos), duration, fi.fileName());
                            AnimeWorker::instance()->saveSnippet(animeName, info, timeId, captureImage);
                        }
                    }
                    else
                    {
                        Logger::logger()->log(Logger::LANServer, QString("[%1]Screenshot,[%2]%3").arg(request.getPeerAddress().toString(),posStr, fi.filePath()));
                        QString info = data.value("info").toString();
                        if(info.isEmpty())  info = QString("%1 - %2").arg(duration2Str(pos), fi.fileName());
                        AnimeWorker::instance()->saveCapture(animeName, info, captureImage);
                    }
                }
                else
                {
                    Logger::logger()->log(Logger::LANServer, QString("[%1]Screenshot, [%2]%3, %4").arg(request.getPeerAddress().toString(),posStr, fi.filePath(), errorInfo));
                }
            }
        }
    }
}

void APIHandler::apiLaunch(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(response)
    QJsonDocument document;
    if(readJson(request.getBody(), document))
    {
        QVariantMap data = document.object().toVariantMap();
        Pool *pool=GlobalObjects::danmuManager->getPool(data.value("danmuPool").toString());
        QString text = data.value("text").toString();

        if(pool && !text.isEmpty())
        {

            int time = data.value("time").toInt();  //ms
            int color = data.value("color", 0xffffff).toInt();
            int fontsize = data.value("fontsize", int(DanmuComment::FontSizeLevel::Normal)).toInt();
            QString dateStr = data.value("date").toString();
            long long date;
            if(dateStr.isEmpty()) date = QDateTime::currentDateTime().toSecsSinceEpoch();
            else date = dateStr.toLongLong();
            int type = data.value("type", int(DanmuComment::DanmuType::Rolling)).toInt();

            DanmuComment comment;
            comment.text = text;
            comment.originTime = comment.time = time;
            comment.color = color;
            comment.fontSizeLevel = (DanmuComment::FontSizeLevel)fontsize;
            comment.date = date;
            comment.type = (DanmuComment::DanmuType)type;

            if(GlobalObjects::danmuPool->getPool()==pool)
            {
                int poolTime = GlobalObjects::danmuPool->getCurrentTime();
                if(time < 0)  // launch to current
                {
                    time = poolTime;
                    comment.originTime = comment.time = time;
                }
                if(qAbs(poolTime - time)<3000)
                {
                    GlobalObjects::danmuPool->launch({QSharedPointer<DanmuComment>(new DanmuComment(comment))});
                }
            }
            int cmin=time/1000/60;
            int cls=time/1000-cmin*60;
            QString commentInfo(QString("[%1:%2]%3").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')).arg(text));
            QString poolInfo(QString("%1 %2").arg(pool->animeTitle(), pool->toEp().toString()));
            Logger::logger()->log(Logger::LANServer, QString("[%1]Launch, [%2] %3").arg(request.getPeerAddress().toString(), poolInfo, commentInfo));
            QStringList scriptIds = data.value("launchScripts").toStringList();
            if(time >= 0 && !scriptIds.isEmpty() && !pool->sources().isEmpty())
            {
                QList<DanmuSource> sources;
                for(auto &src : pool->sources())
                    sources.append(src);

                QStringList results;
                for(auto &id : scriptIds)
                {
                    auto script =  GlobalObjects::scriptManager->getScript(id);
                    DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
                    ScriptState state = dmScript->launch(sources, &comment);
                    results.append(QString("[%1]: %2").arg(id, state?tr("Success"):tr("Faild, %1").arg(state.info)));
                }
                QString msg(QString("%1\n%2\n%3").arg(poolInfo, commentInfo, results.join('\n')));
                Logger::logger()->log(Logger::Script, msg);
            }
        }
    }
}

bool APIHandler::readJson(const QByteArray &bytes, QJsonDocument &doc)
{
    QJsonParseError error;
    doc = QJsonDocument::fromJson(bytes, &error);
    return error.error == QJsonParseError::NoError;
}

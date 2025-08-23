#include "kservice.h"
#include <QThread>
#include <chrono>
#include <QSettings>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkInterface>
#include "Download/downloadmodel.h"
#include "Play/Danmu/Manager/pool.h"
#include "globalobjects.h"
#include "Common/logger.h"
#include "Common/network.h"
#include "Common/eventbus.h"
#include "Common/lrucache.h"
#include "Common/network.h"
#include "Play/Video/mpvmediainfo.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/playcontext.h"
#include "pb/service.pb.h"

#define SERVICE_KEY_DAY_FIRST_START_TIME "Stats/DayFirstStartTime"
#define SERVICE_KEY_DAY_FIRST_APP_START_TIME "Stats/DayFirstUseAppTime"
#define SERVICE_KEY_USER_ID "Profile/UID"
#define SERVICE_KEY_DEVICE_ID "Profile/DID"
#define SERVICE_KEY_USER_NAME "Profile/UserName"
#define SERVICE_KEY_USER_EMAIL "Profile/Email"
#define SERVICE_KEY_ACCESS_TOKEN "Profile/AccessToken"
#define SERVICE_KEY_REFRESH_TOKEN "Profile/RefreshToken"
#define SERVICE_KEY_ENABLE_K_MATCH "KService/EnableMatch"
#define SERVICE_KEY_LIBRARAY_SOURCE_INDEX "KService/LibrarySourceIndex"
#define SERVICE_KEY_LIBRARAY_SELECT_SOURCE "KService/LibrarySourceSelected"

namespace
{
    static LRUCache<QString, kservice::KFileInfo> fileInfoCache{"KServiceFileInfo", 128, true};
}

KService::KService() : QObject{nullptr}, baseURL("https://www.kstat.top")
{
    serviceThread.reset(new QThread());
    serviceThread->setObjectName(QStringLiteral("serviceThread"));
    serviceThread->start(QThread::NormalPriority);
    serviceData.reset(new QSettings(GlobalObjects::context()->dataPath + "service.ini", QSettings::IniFormat));
    profile.loadProfile(serviceData.data());
    new EventListener(EventBus::getEventBus(), EventBus::EVENT_FILE_MATCH_DOWN, std::bind(&KService::listenMatchDown, this, std::placeholders::_1), this);
    new EventListener(EventBus::getEventBus(), EventBus::EVENT_DANMU_SRC_ADDED, std::bind(&KService::listenDanmuAdded, this, std::placeholders::_1), this);
    new EventListener(EventBus::getEventBus(), EventBus::EVENT_KAPP_LOADED, std::bind(&KService::listenCommonEvents, this, std::placeholders::_1), this);
    moveToThread(serviceThread.data());
    QMetaObject::invokeMethod(this, [=](){
        eventTimer.start(std::chrono::milliseconds(5 * 60 * 1000), this);
        this->kStatsUV(true);
    }, Qt::QueuedConnection);
}

KService *KService::instance()
{
    static QScopedPointer<KService, QScopedPointerDeleteLater> serviceObj;
    if (!serviceObj)
    {
        serviceObj.reset(new KService);
    }
    return serviceObj.data();
}

KService::~KService()
{
    if (mediaInfo) mediaInfo->deleteLater();
    serviceThread->quit();
}

void KService::statsUV()
{
    QMetaObject::invokeMethod(this, [=](){
        this->kStatsUV(true);
    }, Qt::QueuedConnection);
}

void KService::fileRecognize(const QString &path)
{
    QMetaObject::invokeMethod(this, [=](){
        this->kFileReco(path);
    });
}

void KService::launch(QSharedPointer<DanmuComment> comment, const QString &poolId, const QString &file)
{
    QMetaObject::invokeMethod(this, [=](){
        this->kLaunch(comment, poolId, file);
    });
}

void KService::login(const QString &email, const QString &password)
{
    QMetaObject::invokeMethod(this, [=](){
        this->kLogin(email, password);
    });
}

void KService::sendVerification(const QString &email)
{
    QMetaObject::invokeMethod(this, [=](){
        this->kSendVerification(email);
    });
}

void KService::registerU(const QString &email, const QString &password, const QString &userName, const QString &verificationCode)
{
    QMetaObject::invokeMethod(this, [=](){
        this->kRegister(email, password, userName, verificationCode);
    });
}

void KService::getDanmu(const QString &poolId, int duration)
{
    QMetaObject::invokeMethod(this, [=](){
        this->kGetDanmu(poolId, duration);
    });
}

void KService::getDanmu(const DanmuSource &kSrc)
{
    QJsonDocument document = QJsonDocument::fromJson(kSrc.scriptData.toUtf8());
    QJsonObject obj = document.object();
    QString poolId = obj.value("poolid").toString();
    int duration = -1;
    if (obj.contains("duration")) duration = obj.value("duration").toInt();
    getDanmu(poolId, duration);
}

void KService::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == eventTimer.timerId())
    {
        kStatsUV(false);
    }
}

void KService::setReqHeader(QNetworkRequest &request, const QString &path)
{
    qint64 ts = QDateTime::currentSecsSinceEpoch();
    request.setRawHeader("X-AppId", Network::kKikoAppId);
    request.setRawHeader("X-Timestamp", QString::number(ts).toUtf8());
    QString signature = QString("%1-%2-%3-%4").arg(Network::kKikoAppId).arg(ts).arg(Network::kKikoAppSecret).arg(path);
    request.setRawHeader("X-Signature", QCryptographicHash::hash(signature.toUtf8(), QCryptographicHash::Md5).toHex());
}

void KService::setEventHeader(kservice::EventHeader &header, const QString &ev, qint64 ts)
{
    header.set_timestamp(ts <= 0 ? QDateTime::currentSecsSinceEpoch() : ts);
    header.set_event(ev.toStdString());
    header.set_os(QString("%1 %2").arg(QSysInfo::productType(), QSysInfo::productVersion()).toStdString());
    header.set_version(GlobalObjects::kikoVersionNum);
    header.set_did(profile.deviceId.toStdString());
    if (profile.login && !profile.userId.isEmpty())
    {
        header.set_uid(profile.userId.toStdString());
    }
}

void KService::setFileInfo(kservice::KFileInfo &fileInfo, const QString &path)
{
    QFileInfo fi(path);
    if (!fi.isFile() || !fi.exists())
    {
        if (path == PlayContext::context()->path && PlayContext::context()->duration > 0)
        {
            fileInfo.set_url(path.toStdString());
            fileInfo.set_durationms(PlayContext::context()->duration * 1000);
            fileInfo.set_islocal(false);
        }
        return;
    }

    if (fileInfoCache.contains(fi.absoluteFilePath()))
    {
        fileInfo = fileInfoCache.get(fi.absoluteFilePath());
        return;
    }

    fileInfo.set_filename(fi.baseName().toStdString());
    fileInfo.set_fullpath(fi.absoluteFilePath().toStdString());
    fileInfo.set_islocal(true);

    if (!mediaInfo)
    {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [&](){
            mediaInfo = new MPVMediaInfo();
        }, Qt::BlockingQueuedConnection);
    }
    if (mediaInfo->loadFile(path))
    {
        fileInfo.set_durationms(mediaInfo->getDuration());
        mediaInfo->reset();
    }

    QFile file(path);
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray fileData(file.read(32*1024*1024));
        fileInfo.set_hash32(QCryptographicHash::hash(fileData, QCryptographicHash::Md5).toHex().toStdString());
        fileInfo.set_filesize(file.size());
    }

    QString downloadURL = GlobalObjects::downloadModel->findFileUri(path);
    if (!downloadURL.isEmpty())
    {
        fileInfo.set_url(downloadURL.toStdString());
        static const QRegularExpression re("magnet:\\?xt=urn:btih:([0-9a-fA-F]{40})(?=&|$)");
        auto match = re.match(downloadURL);
        if (match.hasMatch())
        {
            fileInfo.set_torrenthash(match.capturedTexts()[1].toStdString());
        }
    }
    fileInfoCache.put(fi.absoluteFilePath(), fileInfo);
}

void KService::post(const QString &path, const QByteArray &content, PostCallBack cb, bool loginRequired)
{
    QNetworkRequest request(QUrl(baseURL + path));
    setReqHeader(request, path);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    if (loginRequired && profile.accessTokenValid())
    {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(profile.accessToken).toUtf8());
    }
    request.setTransferTimeout(15000);
    QNetworkReply *reply = Network::getManager()->post(request, content);
    if (cb)
    {
        QObject::connect(reply, &QNetworkReply::finished, this, [reply, cb](){
            if (reply->error() != QNetworkReply::NoError)
            {
                Logger::logger()->log(Logger::APP, "[KService]reply error: " + reply->errorString());
            }
            cb(reply);
        });
    }
    QObject::connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void KService::listenMatchDown(const EventParam *p)
{
    if (!p || p->eventType != EventBus::EVENT_FILE_MATCH_DOWN) return;
    if (!KService::instance()->enableKServiceMatch()) return;

    kservice::MatchEvent matchEvent;
    setEventHeader(*matchEvent.mutable_header(), "match");

    const QVariantMap params = p->param.toMap();
    const QStringList files = params["files"].toStringList();
    const QVariantList eps = params["eps"].toList();
    if (files.empty() || eps.empty() || files.size() != eps.size()) return;

    const QString anime = params["anime"].toString();
    const QString scriptId = params["scriptId"].toString();
    const QString scriptData = params["scriptData"].toString();
    if (anime.isEmpty() || scriptId.isEmpty() || scriptData.isEmpty()) return;

    static QHash<QString, int> fileMatchTimes;

    static const QMap<QString, int> scriptTypeMap = {
        {"Kikyou.l.Bangumi", kservice::InfoSourceType::BGM},
        {"Kikyou.l.Douban", kservice::InfoSourceType::DOUBAN},
    };
    if (!scriptTypeMap.contains(scriptId)) return;

    for (int i = 0; i < files.size(); ++i)
    {
        if (!QFile::exists(files[i])) continue;
        if (fileMatchTimes.value(files[i], 0) > 1) continue;
        ++fileMatchTimes[files[i]];

        kservice::MatchEvent::Match *match = matchEvent.add_matches();
        setFileInfo(*match->mutable_fileinfo(), files[i]);

        kservice::Pool *pool = match->mutable_poolinfo();
        QVariantMap epInfo = eps[i].toMap();
        pool->set_name(anime.toStdString());
        pool->set_eptype(kservice::EpType(epInfo["type"].toInt()));
        pool->set_epindex(epInfo["index"].toDouble());
        pool->set_epname(epInfo["name"].toString().toStdString());

        kservice::InfoSource *src = match->add_infosources();
        src->set_type(kservice::InfoSourceType(scriptTypeMap.value(scriptId)));
        src->set_scriptid(scriptId.toStdString());
        src->set_scriptdata(scriptData.toStdString());
    }
    std::string msgContent;
    matchEvent.SerializeToString(&msgContent);
    post(pathKMatchEvent, QByteArray(msgContent.c_str(), msgContent.size()));
    Logger::logger()->log(Logger::APP, QString("[KService]match event: %1,%2,%3,%4").arg(anime, scriptId, scriptData, files.join("|")));
}

void KService::listenDanmuAdded(const EventParam *p)
{
    if (!p || p->eventType != EventBus::EVENT_DANMU_SRC_ADDED) return;
    kservice::AddDanmuSourceEvent dmSrcEvent;
    setEventHeader(*dmSrcEvent.mutable_header(), "dm_src");

    const QVariantList poolSrcs = p->param.toList();
    if (poolSrcs.empty()) return;

    static const QMap<QString, int> scriptTypeMap = {
        {kSrc, kservice::DanmuSourceType::KIKO},
        {"Kikyou.d.Bilibili", kservice::DanmuSourceType::BILIBILI},
        {"Kikyou.d.Gamer", kservice::DanmuSourceType::GAMER},
        {"Kikyou.d.Iqiyi", kservice::DanmuSourceType::IQIYI},
        {"Kikyou.d.Tencent", kservice::DanmuSourceType::TENCENT},
        {"Kikyou.d.youku", kservice::DanmuSourceType::YOUKU},
        {"Kikyou.d.mgtv", kservice::DanmuSourceType::MGTV},
        {"Kikyou.d.Tucao", kservice::DanmuSourceType::TUCAO},
        {"Kikyou.d.AcFun", kservice::DanmuSourceType::ACFUN},
        {"Kikyou.d.ysjdm", kservice::DanmuSourceType::YSJ},
    };

    QHash<QString, kservice::AddDanmuSourceEvent::DanmuPoolSource *> kSrcMap;
    QStringList pSrc;
    static QHash<QString, int> poolSrcTimes;
    for (auto &p : poolSrcs)
    {
        const QVariantMap srcMap = p.toMap();
        if (!scriptTypeMap.contains(srcMap.value("scriptId").toString())) continue;

        const QString poolId = srcMap.value("id").toString();
        kservice::AddDanmuSourceEvent::DanmuPoolSource *kSrc = kSrcMap.value(poolId, nullptr);
        if (!kSrc)
        {
            kSrc = dmSrcEvent.add_danmupoolsources();
            kSrc->mutable_poolinfo()->set_name(srcMap.value("name").toString().toStdString());
            kSrc->mutable_poolinfo()->set_eptype(kservice::EpType(srcMap.value("epType").toInt()));
            kSrc->mutable_poolinfo()->set_epindex(srcMap.value("epIndex").toDouble());
            kSrc->mutable_poolinfo()->set_epname(srcMap.value("epName").toString().toStdString());
            kSrc->mutable_poolinfo()->set_poolid(poolId.toStdString());
            kSrcMap[poolId] = kSrc;
        }

        if (poolSrcTimes.value(poolId, 0) > 6) continue;
        ++poolSrcTimes[poolId];

        kservice::DanmuSource *kdSrc = kSrc->add_danmusources();
        kdSrc->set_title(srcMap.value("srcTitle").toString().toStdString());
        kdSrc->set_scriptid(srcMap.value("scriptId").toString().toStdString());
        kdSrc->set_scriptdata(srcMap.value("scriptData").toString().toStdString());
        kdSrc->set_durationseconds(srcMap.value("duration").toInt());
        kdSrc->set_type(kservice::DanmuSourceType(scriptTypeMap.value(srcMap.value("scriptId").toString())));

        pSrc.append(QString("%1:%2").arg(srcMap.value("name").toString()).arg(srcMap.value("epIndex").toDouble()));
    }
    if (pSrc.isEmpty()) return;
    std::string msgContent;
    dmSrcEvent.SerializeToString(&msgContent);
    post(pathKDanmuSrcEvent, QByteArray(msgContent.c_str(), msgContent.size()));
    Logger::logger()->log(Logger::APP, QString("[KService]danmu src event: %1").arg(pSrc.join("|")));
}

void KService::listenCommonEvents(const EventParam *p)
{
    if (!p) return;
    if (p->eventType == EventBus::EVENT_KAPP_LOADED)
    {
        const qint64 lastTs = serviceData->value(SERVICE_KEY_DAY_FIRST_APP_START_TIME, 0).toLongLong();
        const QDate lastDate = QDateTime::fromSecsSinceEpoch(lastTs).date();
        const QDate curDate = QDate::currentDate();
        const qint64 curTs = QDateTime::currentSecsSinceEpoch();
        if (lastTs > 0 && lastDate.daysTo(curDate) <= 0) return;
        serviceData->setValue(SERVICE_KEY_DAY_FIRST_APP_START_TIME, curTs);

        const QVariantMap params = p->param.toMap();
        kservice::CommonEvent event;
        setEventHeader(*event.mutable_header(), "a_uv");
        QJsonDocument doc = QJsonDocument::fromVariant(params);
        const QByteArray data = doc.toJson(QJsonDocument::Compact);
        event.set_extra(data.toStdString());

        std::string msgContent;
        event.SerializeToString(&msgContent);
        post(pathCommonEvent, QByteArray(msgContent.c_str(), msgContent.size()));
        Logger::logger()->log(Logger::APP, QString("[KService]common event: %1").arg(QString::fromStdString(event.header().event())));
    }
}

void KService::refreshToken()
{
    if (!profile.refreshTokenValid())
    {
        Logger::logger()->log(Logger::APP, "[KService]refresh token invalid, login");
        QVariantMap param;
        EventBus::getEventBus()->pushEvent(EventParam{EventBus::EVENT_REQUIRE_LOGIN, param});
        return;
    }
    kservice::RefreshRequest req;
    setEventHeader(*req.mutable_header(), "refresh");

    std::string msgContent;
    req.SerializeToString(&msgContent);

    QNetworkRequest request(QUrl(baseURL + pathRefreshToken));
    setReqHeader(request, pathRefreshToken);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(profile.refreshToken).toUtf8());

    QNetworkReply *reply = Network::getManager()->post(request, QByteArray(msgContent.c_str(), msgContent.size()));
    QObject::connect(reply, &QNetworkReply::finished, this, [=](){
        if (reply->error() != QNetworkReply::NoError)
        {
            Logger::logger()->log(Logger::APP, "[KService]refresh failed but token valid: " + reply->errorString());
            return;
        }
        handleRefreshToken(reply);
    });
    QObject::connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void KService::resendStashedComments()
{
    QList<KServiceProfile::StashCommentInfo> stashComments;
    stashComments.swap(profile.stashComments);
    for (const auto &c : stashComments)
    {
        kLaunch(c.comment, c.poolId, c.file);
    }
}

QString KService::isValidUserName(const QString &userName) const
{
    static const QRegularExpression regex("^[\u4e00-\u9fa5a-zA-Z0-9_]+$");

    if (!regex.match(userName).hasMatch())
    {
        return tr("Invalid UserName: Only Chinese characters, English letters, numbers, and underscores are allowed");
    }

    if (userName.length() > 64)
    {
        return tr("Length invalid");
    }

    return "";
}

QString KService::isValidEmail(const QString &email) const
{
    if (email.contains(".."))
    {
        return tr("Invalid email format");
    }
    static const QRegularExpression pattern(R"(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = pattern.match(email);
    if (!match.hasMatch())
    {
        return tr("Invalid email format");
    }
    if (email.length() > 254 || email.split('@')[0].length() > 64)
    {
        return tr("Email length lnvalid");
    }
    return "";
}

QString KService::isValidPassword(const QString &password) const
{
    if (password.length() < 6)
    {
        return tr("Password length invalid, must be greater than 6");
    }
    return "";
}

bool KService::enableKServiceMatch() const
{
    return serviceData->value(SERVICE_KEY_ENABLE_K_MATCH, true).toBool();
}

void KService::setEnableKServiceMatch(bool on)
{
    serviceData->setValue(SERVICE_KEY_ENABLE_K_MATCH, on);
}

QList<QPair<QString, QPair<int, bool>> > KService::getLibrarySource() const
{
    static const QMap<int, QString> srcTypeMap = {
        {kservice::InfoSourceType::BGM, tr("Bangumi")},
        {kservice::InfoSourceType::DOUBAN, tr("Douban")},
    };
    QList<int> indexs = serviceData->value(SERVICE_KEY_LIBRARAY_SOURCE_INDEX).value<QList<int>>();
    if (indexs.empty())
    {
        indexs = {kservice::InfoSourceType::BGM, kservice::InfoSourceType::DOUBAN};
    }
    QSet<int> selected = serviceData->value(SERVICE_KEY_LIBRARAY_SELECT_SOURCE).value<QSet<int>>();
    if (selected.empty())
    {
        selected = QSet<int>(indexs.begin(), indexs.end());
    }
    QList<QPair<QString, QPair<int, bool>>> ret;
    for (int index : indexs)
    {
        if (srcTypeMap.contains(index))
        {
            ret.append({srcTypeMap[index], {index, selected.contains(index)}});
        }
    }
    return ret;
}

void KService::setLibrarySourceIndex(const QList<QPair<int, bool>> &indexSelected)
{
    QList<int> indexs;
    QSet<int> selected;
    for (auto &i : indexSelected)
    {
        indexs.append(i.first);
        if (i.second) selected.insert(i.first);
    }
    serviceData->setValue(SERVICE_KEY_LIBRARAY_SOURCE_INDEX, QVariant::fromValue<QList<int>>(indexs));
    if (!selected.empty()) serviceData->setValue(SERVICE_KEY_LIBRARAY_SELECT_SOURCE, QVariant::fromValue<QSet<int>>(selected));
}

void KService::kStatsUV(bool isStartup)
{
    const qint64 lastTs = serviceData->value(SERVICE_KEY_DAY_FIRST_START_TIME, 0).toLongLong();
    const QDate lastDate = QDateTime::fromSecsSinceEpoch(lastTs).date();
    const QDate curDate = QDate::currentDate();
    const qint64 curTs = QDateTime::currentSecsSinceEpoch();
    if (lastTs > 0 && lastDate.daysTo(curDate) <= 0) return;

    kservice::UVEvent uvEvent;
    serviceData->setValue(SERVICE_KEY_DAY_FIRST_START_TIME, curTs);
    setEventHeader(*uvEvent.mutable_header(), "uv", curTs);

    if (isStartup)
    {
        uvEvent.set_isstartup(true);
        uvEvent.set_startupts(GlobalObjects::context()->startupTime);

        for (auto &p : GlobalObjects::context()->stepTime)
        {
            kservice::UVEvent::StepTime *step = uvEvent.add_steptimes();
            step->set_step(p.first.toStdString());
            step->set_time(p.second);
        }
    }
    std::string msgContent;
    uvEvent.SerializeToString(&msgContent);
    post(pathKStatsUV, QByteArray(msgContent.c_str(), msgContent.size()), std::bind(&KService::handleUV, this, std::placeholders::_1));
    Logger::logger()->log(Logger::APP, "[KService]stats uv: " + curDate.toString());
}

void KService::kFileReco(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.isFile() || !fi.exists())
    {
        return;
    }
    kservice::RecoRequest req;
    setEventHeader(*req.mutable_header(), "reco");
    setFileInfo(*req.mutable_fileinfo(), path);

    std::string msgContent;
    req.SerializeToString(&msgContent);
    post(pathFileReco, QByteArray(msgContent.c_str(), msgContent.size()), std::bind(&KService::handleFileReco, this, path, std::placeholders::_1));
    Logger::logger()->log(Logger::APP, "[KService]file reco: " + path);
}

void KService::kLaunch(QSharedPointer<DanmuComment> comment, const QString &poolId, const QString &launchFile)
{
    if (!profile.login)
    {
        profile.stashComments.append({poolId, launchFile, comment});
        Logger::logger()->log(Logger::APP, "[KService]launch not login: " + poolId);
        QVariantMap param;
        EventBus::getEventBus()->pushEvent(EventParam{EventBus::EVENT_REQUIRE_LOGIN, param});
        return;
    }
    if (!profile.accessTokenValid())
    {
        profile.stashComments.append({poolId, launchFile, comment});
        Logger::logger()->log(Logger::APP, "[KService]launch access token invalid, refresh: " + poolId);
        refreshToken();
        return;
    }
    Pool *pool = GlobalObjects::danmuManager->getPool(poolId, false);
    if (!pool)
    {
        Logger::logger()->log(Logger::APP, "[KService]launch failed, get pool failed: " + poolId);
        return;
    }

    kservice::LaunchRequest req;
    setEventHeader(*req.mutable_header(), "launch");
    setFileInfo(*req.mutable_fileinfo(), launchFile);

    kservice::Pool *servicePool = req.mutable_poolinfo();
    servicePool->set_poolid(poolId.toStdString());
    servicePool->set_name(pool->animeTitle().toStdString());
    EpInfo ep = pool->toEp();
    servicePool->set_epname(ep.name.toStdString());
    servicePool->set_eptype(kservice::EpType((int)ep.type));
    servicePool->set_epindex(ep.index);

    kservice::DanmuComment *serviceComment = req.mutable_comment();
    serviceComment->set_text(comment->text.toStdString());
    serviceComment->set_senderid(profile.userId.toStdString());
    serviceComment->set_color(comment->color);
    serviceComment->set_time(comment->time);
    serviceComment->set_mode((int)comment->type);
    serviceComment->set_size((int)comment->fontSizeLevel);
    serviceComment->set_timestamp(QDateTime::currentSecsSinceEpoch());

    std::string msgContent;
    req.SerializeToString(&msgContent);
    post(pathLaunch, QByteArray(msgContent.c_str(), msgContent.size()), std::bind(&KService::handleLaunch, this, std::placeholders::_1), true);
    Logger::logger()->log(Logger::APP, "[KService]launch: " + poolId);

}

void KService::kLogin(const QString &email, const QString &password)
{
    const QString pwHash{QCryptographicHash::hash(password.toUtf8(),QCryptographicHash::Sha256).toHex()};
    kservice::LoginRequest req;
    setEventHeader(*req.mutable_header(), "login");
    req.set_email(email.toStdString());
    req.set_password(pwHash.toStdString());

    std::string msgContent;
    req.SerializeToString(&msgContent);
    post(pathLogin, QByteArray(msgContent.c_str(), msgContent.size()), std::bind(&KService::handleLogin, this, std::placeholders::_1));
    Logger::logger()->log(Logger::APP, "[KService]login: " + email);
}

void KService::kRegister(const QString &email, const QString &password, const QString &userName, const QString &verificationCode)
{
    QString errInfo = isValidEmail(email);
    if (!errInfo.isEmpty())
    {
        emit registerFinished(0, errInfo);
        return;
    }
    errInfo = isValidPassword(password);
    if (!errInfo.isEmpty())
    {
        emit registerFinished(0, errInfo);
        return;
    }
    errInfo = isValidUserName(userName);
    if (!errInfo.isEmpty())
    {
        emit registerFinished(0, errInfo);
        return;
    }
    kservice::RegisterRequest req;
    setEventHeader(*req.mutable_header(), "register");

    QString passHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
    req.set_username(userName.toStdString());
    req.set_email(email.toStdString());
    req.set_password(passHash.toStdString());
    req.set_verification_code(verificationCode.toStdString());

    std::string msgContent;
    req.SerializeToString(&msgContent);
    post(pathRegister, QByteArray(msgContent.c_str(), msgContent.size()), std::bind(&KService::handleRegister, this, std::placeholders::_1));
    Logger::logger()->log(Logger::APP, "[KService]register: " + email);
}

void KService::kSendVerification(const QString &email)
{
    QString errInfo = isValidEmail(email);
    if (!errInfo.isEmpty())
    {
        Logger::logger()->log(Logger::APP, "[KService]send verification, invalid email: " + email);
        return;
    }
    kservice::SendVerificationRequest req;
    setEventHeader(*req.mutable_header(), "send_verify");
    req.set_email(email.toStdString());

    std::string msgContent;
    req.SerializeToString(&msgContent);
    post(pathSendVerification, QByteArray(msgContent.c_str(), msgContent.size()));
    Logger::logger()->log(Logger::APP, "[KService]send verification: " + email);
}

void KService::kGetDanmu(const QString &poolId, int duration)
{
    Pool *pool = GlobalObjects::danmuManager->getPool(poolId, false);
    if (!pool)
    {
        Logger::logger()->log(Logger::APP, "[KService]get danmu failed, get pool failed: " + poolId);
        return;
    }

    kservice::KikoDanmuRequest req;
    setEventHeader(*req.mutable_header(), "get_danmu");
    if (duration > 0) req.set_duration(duration);

    kservice::Pool *servicePool = req.mutable_poolinfo();
    servicePool->set_poolid(poolId.toStdString());
    servicePool->set_name(pool->animeTitle().toStdString());
    EpInfo ep = pool->toEp();
    servicePool->set_epname(ep.name.toStdString());
    servicePool->set_eptype(kservice::EpType((int)ep.type));
    servicePool->set_epindex(ep.index);

    std::string msgContent;
    req.SerializeToString(&msgContent);
    post(pathGetDanmu, QByteArray(msgContent.c_str(), msgContent.size()), std::bind(&KService::handleGetDanmu, this, std::placeholders::_1));
    Logger::logger()->log(Logger::APP, QString("[KService]get_danmu: %1 %2").arg(pool->animeTitle(), ep.toString()));
}

void KService::handleUV(QNetworkReply *reply)
{
    QByteArray pbData = reply->readAll();
    kservice::UVEventResponse rsp;
    if (!rsp.ParseFromArray(pbData.constData(), pbData.size()))
    {
        Logger::logger()->log(Logger::APP, "[KService]uv rsp parse error");
        return;
    }
}

void KService::handleFileReco(const QString &path, QNetworkReply *reply)
{
    QByteArray pbData = reply->readAll();
    kservice::RecoResponse rsp;
    if (!rsp.ParseFromArray(pbData.constData(), pbData.size()))
    {
        Logger::logger()->log(Logger::APP, "[KService]reco rsp parse error: " + path);
        emit recognized(0, "reco rsp parse error", path, MatchResult());
        return;
    }
    if (rsp.header().status() != 1)
    {
        Logger::logger()->log(Logger::APP, "[KService]reco failed: " + QString::fromStdString(rsp.header().err_msg()));
        emit recognized(rsp.header().status(), QString::fromStdString(rsp.header().err_msg()), path, MatchResult());
        return;
    }

    QMap<int, const kservice::InfoSource *> infoSrcMap;
    for (int i = 0; i < rsp.infosources_size(); ++i)
    {
        auto &info_src = rsp.infosources(i);
        infoSrcMap[info_src.type()] = &info_src;
    }

    const kservice::InfoSource *selectedInfoSource{nullptr};
    auto libSrcs = getLibrarySource();
    for (auto &s : libSrcs)
    {
        if (s.second.second && infoSrcMap.contains(s.second.first))
        {
            selectedInfoSource = infoSrcMap[s.second.first];
            break;
        }
    }
    if (!selectedInfoSource)
    {
        Logger::logger()->log(Logger::APP, "[KService]reco rsp has no selected source");
        emit recognized(0, "reco rsp has no selected source", path, MatchResult());
        return;
    }
    MatchResult match;
    match.kServiceMatch = true;
    match.name = QString::fromStdString(rsp.matchresult().name());
    match.ep.name = QString::fromStdString(rsp.matchresult().epname());
    match.ep.type = EpType(rsp.matchresult().eptype());
    match.ep.index = rsp.matchresult().epindex();
    match.ep.localFile = path;
    match.scriptId = QString::fromStdString(selectedInfoSource->scriptid());
    match.scriptData = QString::fromStdString(selectedInfoSource->scriptdata());
    match.infoSrcType = selectedInfoSource->type();
    for (int i = 0; i < rsp.danmusources_size(); ++i)
    {
        auto &dm_src = rsp.danmusources(i);
        MatchResult::MatchDanmuSource src;
        src.name = QString::fromStdString(dm_src.title());
        src.type = dm_src.type();
        src.durationSeconds = dm_src.durationseconds();
        src.scriptId = QString::fromStdString(dm_src.scriptid());
        src.scriptData = QString::fromStdString(dm_src.scriptdata());
        match.danmuSources.append(src);
    }
    match.success = true;
    emit recognized(1, "", path, match);
}

void KService::handleLaunch(QNetworkReply *reply)
{
    QByteArray pbData = reply->readAll();
    kservice::LaunchResponse rsp;
    if (!rsp.ParseFromArray(pbData.constData(), pbData.size()))
    {
        Logger::logger()->log(Logger::APP, "[KService]launch rsp parse error");
        return;
    }
    if (rsp.header().status() != 1)
    {
        Logger::logger()->log(Logger::APP, "[KService]launch failed: " + QString::fromStdString(rsp.header().err_msg()));
        return;
    }
    const QString poolId{QString::fromStdString(rsp.poolid())};
    Pool *pool = GlobalObjects::danmuManager->getPool(poolId, false);
    if (!pool)
    {
        Logger::logger()->log(Logger::APP, "[KService]launch pool id not found: " + poolId);
        return;
    }
    DanmuSource kSrc;
    kSrc.scriptId = QString::fromStdString(rsp.danmusource().scriptid());
    kSrc.scriptData = QString::fromStdString(rsp.danmusource().scriptdata());
    kSrc.title = QString::fromStdString(rsp.danmusource().title());
    kSrc.duration = rsp.danmusource().durationseconds();

    DanmuComment *comment = new DanmuComment;
    comment->text = QString::fromStdString(rsp.comment().text());
    comment->sender = QString::fromStdString(rsp.comment().senderid());
    comment->color = rsp.comment().color();
    comment->time = comment->originTime = rsp.comment().time();
    comment->type = DanmuComment::DanmuType(rsp.comment().mode());
    comment->fontSizeLevel = DanmuComment::FontSizeLevel(rsp.comment().size());
    comment->date = rsp.comment().timestamp();

    QList<DanmuComment *> tmpComments{comment};
    pool->addSource(kSrc, tmpComments, true);
}

void KService::handleLogin(QNetworkReply *reply)
{
    QByteArray pbData = reply->readAll();
    kservice::LoginResponse rsp;
    if (!rsp.ParseFromArray(pbData.constData(), pbData.size()))
    {
        Logger::logger()->log(Logger::APP, "[KService]login rsp parse error");
        emit loginFinished(0, tr("Login Response Parse Error"));
        return;
    }
    if (rsp.header().status() != 1)
    {
        Logger::logger()->log(Logger::APP, "[KService]login failed: " + QString::fromStdString(rsp.header().err_msg()));
        emit loginFinished(0, QString::fromStdString(rsp.header().err_msg()));
        return;
    }
    profile.userId = QString::fromStdString(rsp.uid());
    profile.userName = QString::fromStdString(rsp.username());
    profile.email = QString::fromStdString(rsp.email());
    profile.accessToken = QString::fromStdString(rsp.access_token());
    profile.accessTokenExpiration = profile.parseTokenExpiration(profile.accessToken);
    profile.refreshToken = QString::fromStdString(rsp.refresh_token());
    profile.refreshTokenExpiration = profile.parseTokenExpiration(profile.refreshToken);
    Logger::logger()->log(Logger::APP, "[KService]login access token expiration: " + profile.accessTokenExpiration.toString());
    Logger::logger()->log(Logger::APP, "[KService]login refresh token expiration: " + profile.refreshTokenExpiration.toString());
    profile.login = true;
    profile.saveProfile(serviceData.data());
    emit loginFinished(1, "");
    resendStashedComments();
    Logger::logger()->log(Logger::APP, "[KService]login success: " + profile.email);
}

void KService::handleRefreshToken(QNetworkReply *reply)
{
    QByteArray pbData = reply->readAll();
    kservice::RefreshResponse rsp;
    if (!rsp.ParseFromArray(pbData.constData(), pbData.size()))
    {
        Logger::logger()->log(Logger::APP, "[KService]refresh rsp parse error");
        return;
    }
    if (rsp.header().status() != 1)
    {
        Logger::logger()->log(Logger::APP, "[KService]refresh failed, login: " + QString::fromStdString(rsp.header().err_msg()));
        QVariantMap param;
        EventBus::getEventBus()->pushEvent(EventParam{EventBus::EVENT_REQUIRE_LOGIN, param});
        return;
    }
    profile.accessToken = QString::fromStdString(rsp.access_token());
    profile.accessTokenExpiration = profile.parseTokenExpiration(profile.accessToken);
    profile.saveProfile(serviceData.data());
    Logger::logger()->log(Logger::APP, "[KService]refresh success, token expiration: " + profile.accessTokenExpiration.toString());
    resendStashedComments();
}

void KService::handleRegister(QNetworkReply *reply)
{
    QByteArray pbData = reply->readAll();
    kservice::RegisterResponse rsp;
    if (!rsp.ParseFromArray(pbData.constData(), pbData.size()))
    {
        Logger::logger()->log(Logger::APP, "[KService]register rsp parse error");
        emit registerFinished(0, tr("Register rsp parse error"));
        return;
    }
    if (rsp.header().status() != 1)
    {
        Logger::logger()->log(Logger::APP, "[KService]register failed: " + QString::fromStdString(rsp.header().err_msg()));
        emit registerFinished(0, QString::fromStdString(rsp.header().err_msg()));
        return;
    }
    QString rEmail = QString::fromStdString(rsp.email());
    QString rUserName = QString::fromStdString(rsp.username());
    QString rUid = QString::fromStdString(rsp.uid());
    Logger::logger()->log(Logger::APP, QString("[KService]register successs, uid: %1, username: %2, email: %3").arg(rUid, rUserName, rEmail));
    emit registerFinished(1, "");
}

void KService::handleGetDanmu(QNetworkReply *reply)
{
    QByteArray pbData = reply->readAll();
    kservice::KikoDanmuResponse rsp;
    if (!rsp.ParseFromArray(pbData.constData(), pbData.size()))
    {
        Logger::logger()->log(Logger::APP, "[KService]get danmu rsp parse error");
        return;
    }
    const QString poolId{QString::fromStdString(rsp.poolinfo().poolid())};
    Pool *pool = GlobalObjects::danmuManager->getPool(poolId, false);
    if (!pool)
    {
        Logger::logger()->log(Logger::APP, "[KService]get danmu pool id not found: " + poolId);
        return;
    }
    for (int i = 0; i < rsp.danmusources_size(); ++i)
    {
        auto &dm_src = rsp.danmusources(i);
        DanmuSource kSrc;
        kSrc.scriptId = QString::fromStdString(dm_src.source().scriptid());
        kSrc.scriptData = QString::fromStdString(dm_src.source().scriptdata());
        kSrc.title = QString::fromStdString(dm_src.source().title());
        kSrc.duration = dm_src.source().durationseconds();

        QList<DanmuComment *> comments;
        comments.reserve(dm_src.danmucomments_size());
        for (int j = 0; j < dm_src.danmucomments_size(); ++j)
        {
            const kservice::DanmuComment &c = dm_src.danmucomments(j);
            DanmuComment *comment = new DanmuComment;
            comment->text = QString::fromStdString(c.text());
            comment->sender = QString::fromStdString(c.senderid());
            comment->color = c.color();
            comment->time = comment->originTime = c.time();
            comment->type = DanmuComment::DanmuType(c.mode());
            comment->fontSizeLevel = DanmuComment::FontSizeLevel(c.size());
            comment->date = c.timestamp();
            comments.append(comment);
        }
        pool->addSource(kSrc, comments, true);
        Logger::logger()->log(Logger::APP, QString("[KService]pool[%1] add source: %2").arg(pool->epTitle(), kSrc.title));
    }
}

bool KServiceProfile::accessTokenValid() const
{
    qint64 secs = QDateTime::currentDateTime().secsTo(accessTokenExpiration);
    return !accessToken.isEmpty() && secs > 100;
}

bool KServiceProfile::refreshTokenValid() const
{
    qint64 secs = QDateTime::currentDateTime().secsTo(refreshTokenExpiration);
    return !refreshToken.isEmpty() && secs > 100;
}

QDateTime KServiceProfile::parseTokenExpiration(const QString &token)
{
    auto defaultExpiration = QDateTime::currentDateTime().addSecs(3600 * 6);
    QStringList parts = token.split('.');
    if (parts.size() != 3) return defaultExpiration;


    QString padded = parts[1];
    while (padded.length() % 4 != 0)
    {
        padded.append('=');
    }
    padded = padded.replace('-', '+').replace('_', '/');
    QByteArray payloadData = QByteArray::fromBase64(padded.toUtf8());
    QJsonParseError parseError;
    QJsonDocument payloadDoc = QJsonDocument::fromJson(payloadData, &parseError);
    if (parseError.error != QJsonParseError::NoError) return defaultExpiration;

    QJsonObject payloadObj = payloadDoc.object();
    if (!payloadObj.contains("exp"))  return defaultExpiration;

    qint64 exp = payloadObj["exp"].toInteger(-1);
    if (exp <= 0) return defaultExpiration;

    return QDateTime::fromSecsSinceEpoch(exp);
}

void KServiceProfile::loadProfile(QSettings *settings)
{
    deviceId = GlobalObjects::appSetting->value(SERVICE_KEY_DEVICE_ID).toString();
    if (deviceId.isEmpty())
    {
        deviceId = settings->value(SERVICE_KEY_DEVICE_ID).toString();
    }
    if (deviceId.isEmpty())
    {
        deviceId = generateDeviceId();
    }
    userId = settings->value(SERVICE_KEY_USER_ID).toString();
    userName = settings->value(SERVICE_KEY_USER_NAME).toString();
    email = settings->value(SERVICE_KEY_USER_EMAIL).toString();
    accessToken = settings->value(SERVICE_KEY_ACCESS_TOKEN).toString();
    refreshToken = settings->value(SERVICE_KEY_REFRESH_TOKEN).toString();
    if (!accessToken.isEmpty()) accessTokenExpiration = parseTokenExpiration(accessToken);
    if (!refreshToken.isEmpty()) refreshTokenExpiration = parseTokenExpiration(refreshToken);
    Logger::logger()->log(Logger::APP, "[KService]load access token expiration: " + accessTokenExpiration.toString());
    Logger::logger()->log(Logger::APP, "[KService]load refresh token expiration: " + refreshTokenExpiration.toString());
    if (!userId.isEmpty() && refreshTokenValid())
    {
        login = true;
        Logger::logger()->log(Logger::APP, "[KService]current user: " + userName);
    }
}

void KServiceProfile::saveProfile(QSettings *settings)
{
    GlobalObjects::appSetting->setValue(SERVICE_KEY_DEVICE_ID, deviceId);
    settings->setValue(SERVICE_KEY_DEVICE_ID, deviceId);
    if (!login) return;
    settings->setValue(SERVICE_KEY_USER_ID, userId);
    settings->setValue(SERVICE_KEY_USER_NAME, userName);
    settings->setValue(SERVICE_KEY_USER_EMAIL, email);
    settings->setValue(SERVICE_KEY_ACCESS_TOKEN, accessToken);
    settings->setValue(SERVICE_KEY_REFRESH_TOKEN, refreshToken);
}

QString KServiceProfile::generateDeviceId() const
{
    QString hardwareInfo;

    for (const QNetworkInterface &i : QNetworkInterface::allInterfaces())
    {
        if (i.type() == QNetworkInterface::Wifi || i.type() == QNetworkInterface::Ethernet)
        {
            hardwareInfo += i.hardwareAddress();
        }
    }
    hardwareInfo += QDateTime::currentDateTime().toString("yyyyMMddHHmmss");

    QByteArray hash = QCryptographicHash::hash(hardwareInfo.toUtf8(), QCryptographicHash::Sha256);
    return hash.left(16).toHex();
}

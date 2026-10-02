#include "kanimeprofile.h"
#include "kservice.h"
#include "pb/service.pb.h"
#include "Common/logger.h"
#include "Common/network.h"
#include "globalobjects.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QRegularExpression>
#include <QSettings>
#include <QSharedPointer>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>
#include <utility>

#define SERVICE_KEY_ENABLE_ANIME_PROFILE "KService/EnableAnimeProfile"

KServiceAux::KAnimeProfileTask::KAnimeProfileTask(KService *service, const QString &settingsPath)
    : QObject(service), service(service), serviceSettingsPath(settingsPath)
{
    QSettings settings(serviceSettingsPath, QSettings::IniFormat);
    animeProfileSettingInitialized = settings.contains(SERVICE_KEY_ENABLE_ANIME_PROFILE);
    animeProfileEnabled.store(settings.value(SERVICE_KEY_ENABLE_ANIME_PROFILE, false).toBool());

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        QMutexLocker locker(&animeProfileSettingMutex);
        animeImageRequestsStopping.store(true);
    }, Qt::DirectConnection);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        coverImageRequests.clear();
        characterImageRequests.clear();
    }, Qt::QueuedConnection);
}

bool KServiceAux::KAnimeProfileTask::getAnimeProfileSync(int srcType, const QString &scriptData, Anime *anime, QStringList &tags)
{
    if (!anime || srcType != kservice::BGM) return false;

    QString srcId;
    if (srcType == kservice::BGM)
    {
        srcId = scriptData.trimmed();
    }
    static const QRegularExpression validId("^[1-9][0-9]{0,63}$");
    if (!validId.match(srcId).hasMatch()) return false;

    struct ServiceRequest
    {
        ServiceRequest(const QString &path, int imageType = -1, const QString &characterName = QString())
            : path(path), imageType(imageType), characterName(characterName) {}
        QString path;
        int imageType;
        QString characterName;
    };
    struct ServiceResponse
    {
        bool received = false;
        bool networkOk = false;
        int httpStatus = 0;
        QByteArray contentType;
        QByteArray body;
    };

    // Keep network replies on the service thread; only copy their data back to the caller.
    auto postSync = [this, srcType, srcId](const QVector<ServiceRequest> &requests, const QString &animeName) {
        QVector<ServiceResponse> emptyResult(requests.size());
        if (requests.isEmpty()) return emptyResult;

        QEventLoop eventLoop;
        QObject::connect(service->serviceThread.data(), &QThread::finished, &eventLoop, &QEventLoop::quit, Qt::QueuedConnection);
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &eventLoop, &QEventLoop::quit, Qt::QueuedConnection);
        if (animeImageRequestsStopping.load() || QCoreApplication::closingDown() ||
            !service->serviceThread->isRunning()) return emptyResult;

        struct PendingRequest
        {
            QMutex mutex;
            QEventLoop *eventLoop = nullptr;
            QVector<ServiceResponse> responses;
            int remaining = 0;
        };
        auto pending = QSharedPointer<PendingRequest>::create();
        pending->eventLoop = &eventLoop;
        pending->responses = emptyResult;
        pending->remaining = requests.size();

        const bool queued = QMetaObject::invokeMethod(this, [this, requests, srcType, srcId, animeName, pending]() {
            for (int i = 0; i < requests.size(); ++i)
            {
                auto sendRequest = [this, requests, srcType, srcId, animeName, pending, i](PostCallBack finished) {
                    {
                        QMutexLocker locker(&pending->mutex);
                        if (!pending->eventLoop) return false;
                    }
                    std::string content;
                    if (requests[i].imageType < 0)
                    {
                        kservice::AnimeProfileGetRequest req;
                        service->setEventHeader(*req.mutable_header(), "profile_get");
                        req.set_srctype(kservice::InfoSourceType(srcType));
                        req.set_srcid(srcId.toStdString());
                        req.SerializeToString(&content);
                    }
                    else
                    {
                        kservice::AnimeImageRequest req;
                        service->setEventHeader(*req.mutable_header(), "fetch_anime_image");
                        req.set_srctype(kservice::InfoSourceType(srcType));
                        req.set_srcid(srcId.toStdString());
                        req.set_imgtype(kservice::AnimeImageType(requests[i].imageType));
                        req.set_animename(animeName.toStdString());
                        req.set_charactername(requests[i].characterName.toStdString());
                        req.SerializeToString(&content);
                    }
                    if (animeImageRequestsStopping.load() || QCoreApplication::closingDown()) return false;
                    service->post(requests[i].path, QByteArray(content.data(), content.size()), [pending, i, finished](QNetworkReply *reply) {
                        if (finished) finished(reply);
                        ServiceResponse response;
                        response.received = true;
                        response.networkOk = reply->error() == QNetworkReply::NoError;
                        response.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                        response.contentType = reply->rawHeader("Content-Type").split(';').first().trimmed().toLower();
                        response.body = reply->readAll();

                        QMutexLocker locker(&pending->mutex);
                        if (!pending->eventLoop) return;
                        pending->responses[i] = std::move(response);
                        if (--pending->remaining == 0)
                            QMetaObject::invokeMethod(pending->eventLoop, "quit", Qt::QueuedConnection);
                    });
                    return true;
                };
                if (requests[i].imageType < 0)
                    sendRequest({});
                else
                    queueAnimeImageRequest(requests[i].imageType == kservice::ANIME_COVER, std::move(sendRequest));
            }
        }, Qt::QueuedConnection);
        if (!queued) return emptyResult;

        eventLoop.exec();
        QMutexLocker locker(&pending->mutex);
        pending->eventLoop = nullptr;
        return pending->responses;
    };

    const ServiceResponse profileReply = postSync({ServiceRequest(pathAnimeProfileGet)}, QString()).first();
    if (animeImageRequestsStopping.load() || QCoreApplication::closingDown()) return false;
    kservice::AnimeProfileGetResponse rsp;
    if (!profileReply.received || !profileReply.networkOk || profileReply.httpStatus != 200 ||
        profileReply.contentType != "application/octet-stream" ||
        !rsp.ParseFromArray(profileReply.body.constData(), profileReply.body.size()) ||
        !rsp.has_header() || rsp.header().status() != 1 || !rsp.has_animeinfo())
    {
        Logger::logger()->log(Logger::APP, QString("[KService]anime profile get failed: %1 %2").arg(srcType).arg(srcId));
        return false;
    }

    const kservice::Anime &info = rsp.animeinfo();
    const QString name = QString::fromStdString(info.name());
    const QString airDate = QString::fromStdString(info.airdate());
    if (name.isEmpty() || QString::fromStdString(info.id()) != srcId ||
        info.source().type() != srcType)
    {
        Logger::logger()->log(Logger::APP, QString("[KService]anime profile get invalid info: %1 %2").arg(srcType).arg(srcId));
        return false;
    }

    anime->_name = name;
    anime->_airDate = airDate;
    anime->_desc = QString::fromStdString(info.desc());
    anime->_url = QString::fromStdString(info.url());
    anime->_coverURL = QString::fromStdString(info.coverurl());
    anime->_coverData.clear();
    anime->_epCount = info.epnum();
    anime->_scriptId = srcType == kservice::BGM ? "Kikyou.l.Bangumi" : "Kikyou.l.Douban";
    anime->_scriptData = QString::fromStdString(info.source().scriptdata());
    if (anime->_scriptData.isEmpty()) anime->_scriptData = scriptData;

    anime->staff.clear();
    for (const auto &entry : info.staff())
        anime->staff.append({QString::fromStdString(entry.first), QString::fromStdString(entry.second)});

    anime->characters.clear();
    for (const auto &item : info.characters())
    {
        Character character;
        character.name = QString::fromStdString(item.name());
        character.actor = QString::fromStdString(item.actor());
        character.link = QString::fromStdString(item.link());
        character.imgURL = QString::fromStdString(item.imgurl());
        if (!character.name.isEmpty()) anime->characters.append(character);
    }
    tags.clear();
    for (const auto &tag : info.tags()) tags.append(QString::fromStdString(tag));

    QVector<ServiceRequest> imageRequests;
    const bool hasCoverImage = rsp.hascoverimage();
    QVector<ServiceResponse> imageReplies;
    if (hasCoverImage)
    {
        imageRequests.append(ServiceRequest(pathAnimeImage, kservice::ANIME_COVER));
        for (const auto &character : std::as_const(anime->characters))
            imageRequests.append(ServiceRequest(pathAnimeImage, kservice::ANIME_CHARACTER, character.name));
        imageReplies = postSync(imageRequests, name);
    }
    if (animeImageRequestsStopping.load() || QCoreApplication::closingDown()) return false;

    auto validImage = [](const ServiceResponse &reply) {
        QImage image;
        return reply.received && reply.networkOk && reply.httpStatus == 200 &&
               reply.contentType == "image/jpeg" && image.loadFromData(reply.body);
    };
    if (hasCoverImage && validImage(imageReplies[0])) anime->_coverData = imageReplies[0].body;

    QStringList fallbackUrls;
    QList<QUrlQuery> fallbackQueries;
    QVector<int> fallbackIndexes;
    for (int i = 0; i < anime->characters.size(); ++i)
    {
        QPixmap image;
        bool loadedFromService = false;
        if (hasCoverImage)
        {
            const ServiceResponse &reply = imageReplies[i + 1];
            loadedFromService = validImage(reply) && image.loadFromData(reply.body);
            if (loadedFromService)
            {
                Character::scale(image);
                anime->characters[i].image = image;
            }
        }
        if (!loadedFromService && !anime->characters[i].imgURL.isEmpty())
        {
            fallbackUrls.append(anime->characters[i].imgURL);
            fallbackQueries.append(QUrlQuery());
            fallbackIndexes.append(i);
        }
    }

    if (animeImageRequestsStopping.load() || QCoreApplication::closingDown()) return false;
    if (anime->_coverData.isEmpty() && !anime->_coverURL.isEmpty())
    {
        const Network::Reply coverReply = Network::httpGet(anime->_coverURL, QUrlQuery());
        QImage image;
        if (!coverReply.hasError && coverReply.statusCode == 200 && image.loadFromData(coverReply.content))
            anime->_coverData = coverReply.content;
    }
    if (animeImageRequestsStopping.load() || QCoreApplication::closingDown()) return false;
    if (!fallbackUrls.isEmpty())
    {
        const QList<Network::Reply> fallbackReplies = Network::httpGetBatch(fallbackUrls, fallbackQueries);
        for (int i = 0; i < fallbackReplies.size(); ++i)
        {
            if (fallbackReplies[i].hasError || fallbackReplies[i].statusCode != 200) continue;
            QPixmap image;
            if (!image.loadFromData(fallbackReplies[i].content)) continue;
            Character::scale(image);
            anime->characters[fallbackIndexes[i]].image = image;
        }
    }
    return !animeImageRequestsStopping.load() && !QCoreApplication::closingDown();
}

void KServiceAux::KAnimeProfileTask::queueAnimeImageRequest(bool isCover, AnimeImageRequest request)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (animeImageRequestsStopping.load() || QCoreApplication::closingDown()) return;
    (isCover ? coverImageRequests : characterImageRequests).enqueue(std::move(request));
    scheduleAnimeImageRequests();
}

void KServiceAux::KAnimeProfileTask::scheduleAnimeImageRequests()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (animeImagePumpScheduled || animeImageRequestsStopping.load() || QCoreApplication::closingDown()) return;
    animeImagePumpScheduled = true;
    QMetaObject::invokeMethod(this, [this]() { startAnimeImageRequests(); }, Qt::QueuedConnection);
}

void KServiceAux::KAnimeProfileTask::startAnimeImageRequests()
{
    Q_ASSERT(QThread::currentThread() == thread());
    animeImagePumpScheduled = false;
    if (animeImageRequestsStopping.load() || QCoreApplication::closingDown())
    {
        coverImageRequests.clear();
        characterImageRequests.clear();
        return;
    }

    constexpr int maxConcurrentImages = 2;
    while (activeAnimeImageRequests < maxConcurrentImages &&
           !animeImageRequestsStopping.load() && !QCoreApplication::closingDown() &&
           (!coverImageRequests.isEmpty() || !characterImageRequests.isEmpty()))
    {
        auto request = !coverImageRequests.isEmpty() ? coverImageRequests.dequeue() : characterImageRequests.dequeue();
        ++activeAnimeImageRequests;
        const bool started = request([this](QNetworkReply *) {
            Q_ASSERT(activeAnimeImageRequests > 0);
            --activeAnimeImageRequests;
            // Schedule before the caller callback, which may enter another event loop.
            scheduleAnimeImageRequests();
        });
        if (!started) --activeAnimeImageRequests;
    }
    if (animeImageRequestsStopping.load() || QCoreApplication::closingDown())
    {
        coverImageRequests.clear();
        characterImageRequests.clear();
    }
}

bool KServiceAux::KAnimeProfileTask::enabled() const
{
    return animeProfileEnabled.load();
}

void KServiceAux::KAnimeProfileTask::setEnabled(bool on)
{
    bool changed = false;
    {
        QMutexLocker locker(&animeProfileSettingMutex);
        animeProfileSettingInitialized = true;
        changed = animeProfileEnabled.exchange(on) != on;
        QSettings settings(serviceSettingsPath, QSettings::IniFormat);
        settings.setValue(SERVICE_KEY_ENABLE_ANIME_PROFILE, on);
        settings.sync();
        if (settings.status() != QSettings::NoError)
            Logger::logger()->log(Logger::APP, "[KService]save anime profile setting failed");
    }
    if (changed) emit service->animeProfileSettingChanged(on);
}

void KServiceAux::KAnimeProfileTask::initSetting()
{
    {
        QMutexLocker locker(&animeProfileSettingMutex);
        if (animeProfileSettingInitialized || animeProfileProbeInFlight ||
            animeImageRequestsStopping.load() || QCoreApplication::closingDown()) return;
        animeProfileProbeInFlight = true;
    }

    QNetworkRequest request(QUrl("https://api.bgm.tv/v0/subjects/12"));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", QString("Kikyou/KikoPlay/%1 (https://github.com/KikoPlayProject/KikoPlay)")
                         .arg(GlobalObjects::kikoVersion).toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setTransferTimeout(5000);
    QNetworkReply *reply = Network::getManager()->get(request);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, reply, [reply]() {
        if (!reply->isFinished()) reply->abort();
    }, Qt::QueuedConnection);
    QTimer *deadline = new QTimer(reply);
    deadline->setSingleShot(true);

    struct ProbeState
    {
        bool timedOut = false;
    };
    auto state = QSharedPointer<ProbeState>::create();
    QObject::connect(deadline, &QTimer::timeout, reply, [reply, state]() {
        state->timedOut = true;
        reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, deadline, state]() {
        deadline->stop();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        const QJsonObject subject = document.object();
        const bool reachable = !state->timedOut && reply->error() == QNetworkReply::NoError &&
            status >= 200 && status < 300 && parseError.error == QJsonParseError::NoError &&
            document.isObject() && subject.value("id").toInt() == 12 &&
            (!subject.value("name").toString().isEmpty() || !subject.value("name_cn").toString().isEmpty());

        bool enabled = false;
        bool changed = false;
        bool initialized = false;
        {
            QMutexLocker locker(&animeProfileSettingMutex);
            animeProfileProbeInFlight = false;
            // A manual change during the probe always takes precedence.
            if (!animeProfileSettingInitialized && !animeImageRequestsStopping.load() &&
                !QCoreApplication::closingDown())
            {
                QSettings settings(serviceSettingsPath, QSettings::IniFormat);
                if (settings.contains(SERVICE_KEY_ENABLE_ANIME_PROFILE))
                {
                    enabled = settings.value(SERVICE_KEY_ENABLE_ANIME_PROFILE).toBool();
                }
                else
                {
                    enabled = !reachable;
                    // Persist false too, so a successful probe is not repeated next startup.
                    settings.setValue(SERVICE_KEY_ENABLE_ANIME_PROFILE, enabled);
                    settings.sync();
                    if (settings.status() != QSettings::NoError)
                        Logger::logger()->log(Logger::APP, "[KService]save initial anime profile setting failed");
                }
                animeProfileSettingInitialized = true;
                changed = animeProfileEnabled.exchange(enabled) != enabled;
                initialized = true;
            }
        }
        if (initialized)
        {
            Logger::logger()->log(Logger::APP, QString("[KService]Bangumi API probe: %1, HTTP %2, anime profile %3")
                                 .arg(reachable ? "available" : "unavailable").arg(status).arg(enabled ? "enabled" : "disabled"));
            if (changed) emit service->animeProfileSettingChanged(enabled);
        }
        reply->deleteLater();
    });
    deadline->start(5000);
}


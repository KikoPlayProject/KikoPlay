#ifndef KANIMEPROFILE_H
#define KANIMEPROFILE_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QStringList>
#include <atomic>
#include <functional>

class Anime;
class KService;
class QNetworkReply;

namespace KServiceAux
{
class KAnimeProfileTask : public QObject
{
public:
    KAnimeProfileTask(KService *service, const QString &settingsPath);

    bool getAnimeProfileSync(int srcType, const QString &scriptData, Anime *anime, QStringList &tags);
    bool enabled() const;
    void setEnabled(bool on);
    void initSetting();

private:
    using PostCallBack = std::function<void(QNetworkReply *)>;
    using AnimeImageRequest = std::function<bool(PostCallBack)>;

    void queueAnimeImageRequest(bool isCover, AnimeImageRequest request);
    void scheduleAnimeImageRequests();
    void startAnimeImageRequests();

    KService *service;
    const QString serviceSettingsPath;
    const QString pathAnimeProfileGet{"/api/profile_get"};
    const QString pathAnimeImage{"/api/anime_image"};
    std::atomic_bool animeProfileEnabled{false};
    QMutex animeProfileSettingMutex;
    bool animeProfileSettingInitialized = false;
    bool animeProfileProbeInFlight = false;

    QQueue<AnimeImageRequest> coverImageRequests;
    QQueue<AnimeImageRequest> characterImageRequests;
    int activeAnimeImageRequests = 0;
    bool animeImagePumpScheduled = false;
    std::atomic_bool animeImageRequestsStopping{false};
};
}

#endif // KANIMEPROFILE_H

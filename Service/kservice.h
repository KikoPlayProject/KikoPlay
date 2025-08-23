#ifndef KSERVICE_H
#define KSERVICE_H

#include <QObject>
#include <QTimer>
#include "MediaLibrary/animeinfo.h"

class QSettings;
class QNetworkReply;
class QNetworkRequest;
struct EventParam;
class MPVMediaInfo;
struct DanmuComment;
struct DanmuSource;

namespace kservice
{
    class EventHeader;
    class KFileInfo;
}
struct KServiceProfile
{
    std::atomic_bool login{false};
    QString deviceId;
    QString userId;
    QString userName;
    QString email;
    QString accessToken;
    QString refreshToken;
    QDateTime accessTokenExpiration;
    QDateTime refreshTokenExpiration;

    struct StashCommentInfo
    {
        QString poolId;
        QString file;
        QSharedPointer<DanmuComment> comment;
    };

    QList<StashCommentInfo> stashComments;

    bool accessTokenValid() const;
    bool refreshTokenValid() const;
    QDateTime parseTokenExpiration(const QString &token);

    void loadProfile(QSettings *settings);
    void saveProfile(QSettings *settings);

    QString generateDeviceId() const;
};

class KService : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KService)

    explicit KService();

public:
    static KService *instance();
    virtual ~KService();

    static constexpr const char *kSrc = "KService_DM";

public:
    void statsUV();
    void fileRecognize(const QString &path);
    void launch(QSharedPointer<DanmuComment> comment, const QString &poolId, const QString &file);
    void login(const QString &email, const QString &password);
    void sendVerification(const QString &email);
    void registerU(const QString &email, const QString &password, const QString &userName, const QString &verificationCode);
    void getDanmu(const QString &poolId, int duration = -1);
    void getDanmu(const DanmuSource &kSrc);

signals:
    void recognized(int status, const QString &errMsg, const QString &path, MatchResult result);
    void loginFinished(int status, const QString &errMsg);
    void registerFinished(int status, const QString &errMsg);

public:
    QString isValidUserName(const QString &userName) const;
    QString isValidEmail(const QString &email) const;
    QString isValidPassword(const QString &password) const;
    bool enableKServiceMatch() const;
    void setEnableKServiceMatch(bool on);
    QList<QPair<QString, QPair<int, bool> > > getLibrarySource() const;
    void setLibrarySourceIndex(const QList<QPair<int, bool> > &indexSelected);

protected:
    void timerEvent(QTimerEvent* event);

private:
    QScopedPointer<QThread> serviceThread;
    const QString baseURL;
    QBasicTimer eventTimer;
    QScopedPointer<QSettings> serviceData;
    MPVMediaInfo *mediaInfo{nullptr};
    KServiceProfile profile;

private:
    const QString pathKStatsUV{"/api/stats"};
    const QString pathFileReco{"/api/reco"};
    const QString pathKMatchEvent{"/api/match_ev"};
    const QString pathKDanmuSrcEvent{"/api/dm_src_ev"};
    const QString pathCommonEvent{"/api/c_ev"};
    const QString pathLaunch{"/api/launch"};
    const QString pathLogin{"/api/login"};
    const QString pathRegister{"/api/register"};
    const QString pathSendVerification{"/api/send_verify"};
    const QString pathRefreshToken{"/api/token_refresh"};
    const QString pathGetDanmu{"/api/get_danmu"};

private:
    using PostCallBack = std::function<void(QNetworkReply *)>;

    void setReqHeader(QNetworkRequest &request, const QString &path);
    void setEventHeader(kservice::EventHeader &header, const QString &ev, qint64 ts = -1);
    void setFileInfo(kservice::KFileInfo &fileInfo, const QString &path);
    void post(const QString &path, const QByteArray &content, PostCallBack cb = nullptr, bool loginRequired = false);
    void listenMatchDown(const EventParam *p);
    void listenDanmuAdded(const EventParam *p);
    void listenCommonEvents(const EventParam *p);

    void refreshToken();
    void resendStashedComments();


private:
    void kStatsUV(bool isStartup);
    void kFileReco(const QString &path);
    void kLaunch(QSharedPointer<DanmuComment> comment, const QString &poolId, const QString &launchFile);
    void kLogin(const QString &email, const QString &password);
    void kRegister(const QString &email, const QString &password, const QString &userName, const QString &verificationCode);
    void kSendVerification(const QString &email);
    void kGetDanmu(const QString &poolId, int duration = -1);

    void handleUV(QNetworkReply *reply);
    void handleFileReco(const QString &path, QNetworkReply *reply);
    void handleLaunch(QNetworkReply *reply);
    void handleLogin(QNetworkReply *reply);
    void handleRefreshToken(QNetworkReply *reply);
    void handleRegister(QNetworkReply *reply);
    void handleGetDanmu(QNetworkReply *reply);
};

#endif // KSERVICE_H

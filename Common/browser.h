#ifndef BROWSER_H
#define BROWSER_H
#include <QObject>
#include <QMap>

class QWebEngineView;
class QWebEngineProfile;
class BrowserManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(BrowserManager)
public:
    virtual ~BrowserManager() {};
    static BrowserManager *instance();

    QWebEngineView *get();
    void release(QWebEngineView *webView);

    QMap<QString, QMap<QString, QString>> getAllCookie();
    QMap<QString, QString> getDomainCookie(const QString &domain);
    QString getDomainPathCookie(const QString &domain, const QString &path);
    QString getUserAgent();
    void setUserAgent(const QString &ua);

private:
    explicit BrowserManager(QObject *parent = nullptr);

    QWebEngineProfile *profile{nullptr};
    QMap<QString, QMap<QString, QString>> allCookies;
    bool cookieLoaded{false};
    QString persistentStoragePath;

    void loadLocalCookie();
};

#endif // BROWSER_H

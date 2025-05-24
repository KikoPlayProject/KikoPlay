#include "browser.h"
#include <QWebEngineView>
#include <QThread>
#include <QFile>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include "Common/lrucache.h"
#include "globalobjects.h"
#include "qcoreapplication.h"

namespace
{
static TimeLimitedCachePool<QWebEngineView *> browserCache{30*1000, [](QWebEngineView *view){view->deleteLater();}};
}

BrowserManager *BrowserManager::instance()
{
    static QScopedPointer<BrowserManager> manager;
    if (!manager)
    {
        if (QThread::currentThread() == QCoreApplication::instance()->thread())
        {
            manager.reset(new BrowserManager);
        }
        else
        {
            QMetaObject::invokeMethod(QCoreApplication::instance(), [&](){
                manager.reset(new BrowserManager);
            }, Qt::BlockingQueuedConnection);
        }
    }
    return manager.get();
}

QWebEngineView *BrowserManager::get()
{
    QWebEngineView *view = browserCache.get();
    if (!view)
    {
        QMetaObject::invokeMethod(this, [&view, this](){
            if (!cookieLoaded) loadLocalCookie();
            view = new QWebEngineView(profile);
        }, QThread::currentThread() == this->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection);
    }
    return view;
}

void BrowserManager::release(QWebEngineView *webView)
{
    browserCache.put(webView, 600 * 1000);
}

QMap<QString, QMap<QString, QString>> BrowserManager::getAllCookie()
{
    QMap<QString, QMap<QString, QString> > allCookies;
    QMetaObject::invokeMethod(this, [this, &allCookies](){
        if (!cookieLoaded) loadLocalCookie();
        allCookies = this->allCookies;
    }, QThread::currentThread() == this->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection);
    return allCookies;
}

QMap<QString, QString> BrowserManager::getDomainCookie(const QString &domain)
{
    QMap<QString, QString> domainCookies;
    QMetaObject::invokeMethod(this, [this, &domainCookies, &domain](){
        if (!cookieLoaded) loadLocalCookie();
        domainCookies = this->allCookies.value(domain);
    }, QThread::currentThread() == this->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection);
    return domainCookies;
}

QString BrowserManager::getDomainPathCookie(const QString &domain, const QString &path)
{
    QString domainPathCookie;
    QMetaObject::invokeMethod(this, [this, &domainPathCookie, &domain, &path](){
        if (!cookieLoaded) loadLocalCookie();
        domainPathCookie = this->allCookies.value(domain).value(path);
    }, QThread::currentThread() == this->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection);
    return domainPathCookie;
}

QString BrowserManager::getUserAgent()
{
    QString ua;
    QMetaObject::invokeMethod(this, [this, &ua](){
        ua = this->profile->httpUserAgent();
    }, QThread::currentThread() == this->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection);
    return ua;
}

void BrowserManager::setUserAgent(const QString &ua)
{
    QMetaObject::invokeMethod(this, [this, &ua](){
        this->profile->setHttpUserAgent(ua);
    }, QThread::currentThread() == this->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection);
}

BrowserManager::BrowserManager(QObject *parent)
{
    persistentStoragePath = QString("%1/webview/").arg(GlobalObjects::context()->dataPath);
    profile = new QWebEngineProfile("kiko_webview", this);
    profile->setPersistentStoragePath(persistentStoragePath);
    profile->setCachePath(QString("%1/cache/").arg(persistentStoragePath));
    profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    QObject::connect(profile->cookieStore(), &QWebEngineCookieStore::cookieAdded, this, [=](const QNetworkCookie &cookie){
        this->allCookies[cookie.domain()][cookie.path()] = cookie.value();
    });
    QObject::connect(profile->cookieStore(), &QWebEngineCookieStore::cookieRemoved, this, [=](const QNetworkCookie &cookie){
        if (this->allCookies.contains(cookie.domain()))
        {
            this->allCookies[cookie.domain()].remove(cookie.path());
        }
    });
}

void BrowserManager::loadLocalCookie()
{
    QString dbFile(persistentStoragePath + "Cookies");
    if (!QFile::exists(dbFile))
    {
        cookieLoaded = true;
        return;
    }
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", "kiko_webview_cookie");
    database.setDatabaseName(dbFile);
    database.open();
    QSqlQuery query(database);

    query.prepare("select host_key, name, value from cookies");
    query.exec();
    const int hostKeyNo = query.record().indexOf("host_key"),
        nameNo = query.record().indexOf("name"), valueNo = query.record().indexOf("value");
    while (query.next())
    {
        allCookies[query.value(hostKeyNo).toString()][query.value(nameNo).toString()] = query.value(valueNo).toString();
    }
    cookieLoaded = true;
}

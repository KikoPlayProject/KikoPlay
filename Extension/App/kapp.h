#ifndef KAPP_H
#define KAPP_H
#include "Extension/App/AppWidgets/appwidget.h"
#include <QVector>
#include <QHash>
#include <QThread>
#include <QIcon>

namespace Extension
{

class KApp : public QObject
{
    Q_OBJECT
public:
    static KApp *create(const QString &path);
    static KApp *getApp(lua_State *L);
public:
    enum LaunchScene
    {
        LaunchScene_AppMenu
    };

    KApp &operator=(const KApp&) = delete;
    KApp(const KApp&) = delete;
    ~KApp();
    bool start(LaunchScene scene = LaunchScene::LaunchScene_AppMenu);
    void stop();
    void show();

public:
    void eventCall(const QString &func, const QVariant &param);
    void eventCall(const QString &func, int paramCount);
    QVariantList funcCall(const QString &func, const QVariantList &params, int nRet, QString &errInfo);
    bool hasFunc(const QByteArrayList &tnames);

public:
    AppWidget *getWidget(const QString &name);
    QThread *getThread() const { return appThread; }
    lua_State *getState() const { return L; }
    QString id() const {return appInfo.value("id").toString();}
    QString name() const {return appInfo.value("name").toString();}
    QString path() const { return appInfo.value("path").toString(); }
    QString dataPath() const { return appInfo.value("dataPath").toString(); }
    const QIcon &icon() const;
    qint64 time() const { return appInfo.value("time").toLongLong(); }
    qint64 getLatestFileModifyTime() const;
    bool isLoaded() const { return loaded; }
    AppWidget *window() const { return mainWindow; }

    void addRes(const QString &key, AppRes *res);
    AppRes *getRes(const QString &key);
    void removeRes(const QString &key);

signals:
    void appClose(Extension::KApp *app);
    void appFlash(Extension::KApp *app);
private:
    KApp();
private:
    QString loadAppInfo(const QString &path);
    QString loadScript(const QString &path);
    void setEnvInfo();
    void callAppLoaded(LaunchScene scene);
    bool getTable(const QStringList &tnames);

    static const char *globalAppObj;
    const char *appTable = "app";
    const char *appEventLoaded = "loaded";
    const char *appEventClose = "close";
    const char *appEventHide = "hide";
    const char *appEventShow = "show";
    const char *envTable = "env";

    static void exitHook(lua_State *L, lua_Debug *ar);

    bool loaded;
    QThread *appThread;
    lua_State *L;
    AppWidget *mainWindow;
    QVector<AppRes *> appResources;
    QHash<QString, QVariant> appInfo;
    QHash<QString, AppRes *> resHash;
    QIcon appIcon;

    void setAppWindowEvent();
    void loadFont();
};

}

#endif // KAPP_H

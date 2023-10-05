#include "kapp.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QFontDatabase>
#include "Extension/Common/ext_common.h"
#include "Extension/Modules/lua_apputil.h"
#include "Extension/Modules/lua_regex.h"
#include "Extension/Modules/lua_appnet.h"
#include "Extension/Modules/lua_appui.h"
#include "Extension/Modules/lua_dir.h"
#include "Extension/Modules/lua_appcommondialog.h"
#include "Extension/Modules/lua_stringutil.h"
#include "Extension/Modules/lua_appimage.h"
#include "Extension/Modules/lua_appevent.h"
#include "Extension/Modules/lua_playerinterface.h"
#include "Extension/Modules/lua_playlistinterface.h"
#include "Extension/Modules/lua_storageinterface.h"
#include "Extension/Modules/lua_clipboardinterface.h"
#include "Extension/Modules/lua_danmuinterface.h"
#include "Extension/Modules/lua_libraryinterface.h"
#include "Extension/Modules/lua_downloadinterface.h"
#include "Extension/Modules/lua_process.h"
#include "Extension/Modules/lua_timer.h"
#include "Extension/Modules/lua_htmlparser.h"
#include "Extension/Modules/lua_xmlreader.h"
#include "appframelessdialog.h"
#include "Common/threadtask.h"
#include "Common/logger.h"
#include "globalobjects.h"

namespace Extension
{
const char *KApp::globalAppObj = "kiko_app";

struct FontRes : public AppRes
{
    FontRes(int id) : fontId(id) {}
    ~FontRes() { QFontDatabase::removeApplicationFont(fontId); }
    int fontId;
};

KApp *KApp::create(const QString &path)
{
    const QString appJsonPath = path + "/app.json";
    const QString appIndexPath = path + "/app.xml";
    const QString appScriptPath = path + "/app.lua";
    if (!QFile::exists(appJsonPath) || !QFile::exists(appIndexPath) || !QFile::exists(appScriptPath))
    {
        return nullptr;
    }
    QScopedPointer<KApp> app (new KApp);
    QString errInfo = app->loadAppInfo(appJsonPath);
    if (!errInfo.isEmpty())
    {
        return nullptr;
    }
    return app.take();
}

KApp *KApp::getApp(lua_State *L)
{
    lua_pushstring(L, KApp::globalAppObj);
    if (lua_gettable(L, LUA_REGISTRYINDEX) == LUA_TNIL) return nullptr;
    KApp *app = (KApp *)lua_topointer(L, -1);
    lua_pop(L, 1);
    return app;
}

KApp::~KApp()
{
    stop();
}

bool KApp::start(LaunchScene scene)
{
    if (loaded) return true;

    L = luaL_newstate();
    if (!L) return false;

    //  setup lua env -------------
    luaL_openlibs(L);
    setEnvInfo();
    loadFont();
    AppUtil(L).setup();
    StringUtil(L).setup();
    Regex(L).setup();
    AppNet(L).setup();
    AppImage(L).setup();
    AppUI(L).setup();
    AppCommonDialog(L).setup();
    PlayerInterface(L).setup();
    PlayListInterface(L).setup();
    StorageInterface(L).setup();
    ClipboardInterface(L).setup();
    DanmuInterface(L).setup();
    LibraryInterface(L).setup();
    DownloadInterface(L).setup();
    Process(L).setup();
    Dir(L).setup();
    Timer(L).setup();
    XmlReader(L).setup();
    HtmlParser(L).setup();
    AppEventBusEvent(L).setup();
    AppWidget::registEnv(L);

    //  start app ----------------
    appThread = new QThread;
    appThread->setObjectName("kapp/" + id());
    appThread->start();
    QObject tmp;
    tmp.moveToThread(appThread);
    QString errInfo;
    QMetaObject::invokeMethod(&tmp, [this, &errInfo](){
        errInfo = loadScript(this->path() + "/app.lua");
    }, Qt::BlockingQueuedConnection);
    if (!errInfo.isEmpty())
    {
        Logger::logger()->log(Logger::Extension, QString("[%1]app start error: %2").arg(id(), errInfo));
        return false;
    }
    AppWidget *w = AppWidget::parseFromXmlFile(path() + "/app.xml", this, "window", &errInfo);
    if (!w)
    {
        Logger::logger()->log(Logger::Extension, QString("[%1]xml error: %2").arg(id(), errInfo));
        return false;
    }
    w->moveToThread(appThread);
    this->mainWindow = w;
    this->appResources.append(new WindowRes(w));
    setAppWindowEvent();

    QMetaObject::invokeMethod(w, [this, scene](){
        callAppLoaded(scene);
#ifdef QT_DEBUG
        this->window()->dumpObjectTree();
#endif
    }, Qt::QueuedConnection);
    loaded = true;
    return true;
}

void KApp::close()
{
    if (!loaded || !mainWindow || !mainWindow->getWidget()) return;
    AppFramelessDialog *w = static_cast<AppFramelessDialog *>(mainWindow->getWidget());
    w->reject();
    stop();
}

void KApp::stop()
{
    if (!loaded) return;
    if (L)
    {
        lua_sethook(L, exitHook, LUA_MASKCOUNT, 1);
    }
    QWidget *window = mainWindow->getWidget();
    qDeleteAll(appResources);
    if (appThread)
    {
        QEventLoop eventLoop;
        QObject::connect(appThread, &QThread::finished, &eventLoop, &QEventLoop::quit);
        appThread->quit();
        eventLoop.exec();
    }
    if (window) window->deleteLater();
    if (L)
    {
        lua_close(L);
        L = nullptr;
    }
    if (appThread)
    {
        appThread->deleteLater();
        appThread = nullptr;
    }
    appResources.clear();
    resHash.clear();
    mainWindow = nullptr;
    loaded = false;
    emit appClose(this);
}

void KApp::show()
{
    if (!loaded || !mainWindow) return;
    AppFramelessDialog *w = static_cast<AppFramelessDialog *>(mainWindow->getWidget());
    QMetaObject::invokeMethod(w, [w](){
        w->show();
        w->raise();
    });
    ThreadTask task(appThread);
    task.RunOnce([=](){
        QVariantList params = {QVariantMap()};
        this->eventCall(this->appEventShow, params);
    });
}

void KApp::eventCall(const QString &func, const QVariant &param)
{
    //  in appThread
    if (!L)
    {
        return;
    }
    bool isRef = false;
    const int ref = func.toInt(&isRef);
    if (isRef)
    {
        if (lua_rawgeti(L, LUA_REGISTRYINDEX, ref) != LUA_TFUNCTION)
        {
            lua_pop(L, 1);
            return;
        }
    }
    else
    {
        QStringList funcs = func.split(".", Qt::SkipEmptyParts);
        if (funcs.size() == 1)
        {
            if (lua_getglobal(L, appTable) != LUA_TTABLE)
            {
                lua_pop(L, 1);
                return;
            }
            lua_pushstring(L, func.toStdString().c_str());  // t fname
        }
        else
        {
            QStringList tnames = {appTable};
            tnames += funcs.mid(0, funcs.length() - 1);
            if (!getTable(tnames))
            {
                Logger::logger()->log(Logger::Extension, QString("[eventCall]func not found: %1").arg(func));
                return;
            }
            lua_pushstring(L, funcs.back().toStdString().c_str());  // t fname
        }

        if (lua_gettable(L, -2) != LUA_TFUNCTION)
        {
            lua_pop(L, 2);
            return;
        }
        lua_remove(L, 1);  //func
    }
    if (param.isValid())
    {
        pushValue(L, param);
    }
    if(lua_pcall(L, param.isValid()? 1 : 0, 0, 0))
    {
        Logger::logger()->log(Logger::Extension, QString("[eventCall]error: %1").arg(lua_tostring(L, -1)));
        lua_pop(L, 1);
    }
}

void KApp::eventCall(const QString &func, int paramCount)
{
    //  in appThread
    if (!L)
    {
        return;
    }
    bool isRef = false;
    const int ref = func.toInt(&isRef);
    if (isRef)
    {
        if (lua_rawgeti(L, LUA_REGISTRYINDEX, ref) != LUA_TFUNCTION)
        {
            lua_pop(L, 1 + paramCount);
            return;
        }
    }
    else
    {
        QStringList funcs = func.split(".", Qt::SkipEmptyParts);
        if (funcs.size() == 1)
        {
            if (lua_getglobal(L, appTable) != LUA_TTABLE)
            {
                lua_pop(L, 1 + paramCount);
                return;
            }
            lua_pushstring(L, func.toUtf8().constData());  // <params> t fname
        }
        else
        {
            QStringList tnames = {appTable};
            tnames += funcs.mid(0, funcs.length() - 1);
            if (!getTable(tnames))
            {
                lua_pop(L, paramCount);
                Logger::logger()->log(Logger::Extension, QString("[eventCall]func not found: %1").arg(func));
                return;
            }
            lua_pushstring(L, funcs.back().toStdString().c_str());  // <params> t fname
        }
        if (lua_gettable(L, -2) != LUA_TFUNCTION)
        {
            lua_pop(L, 2 + paramCount);
            return;
        }
        lua_remove(L, -2);  // <params> func
    }
    lua_insert(L, 1);  // func <params>
    if(lua_pcall(L,  paramCount, 0, 0))
    {
        Logger::logger()->log(Logger::Extension, QString("[eventCall]error: %1").arg(lua_tostring(L, -1)));
        lua_pop(L, 1);
    }
}

QVariantList KApp::funcCall(const QString &func, const QVariantList &params, int nRet, QString &errInfo)
{
    if (!L)
    {
        errInfo = "Wrong Lua State";
        return QVariantList();
    }
    if (lua_getglobal(L, appTable) != LUA_TTABLE)
    {
        lua_pop(L, 1);
        errInfo = "app not exist";
        return QVariantList();
    }

    lua_pushstring(L, func.toUtf8().constData());  // t fname
    if (lua_gettable(L, -2) != LUA_TFUNCTION)
    {
        lua_pop(L, 2);
        errInfo = QString("%1 is not founded").arg(func);
        return QVariantList();
    }
    lua_remove(L, -2);  //func
    for (auto &p : params)
    {
        pushValue(L, p);
    }
    if (lua_pcall(L, params.size(), nRet, 0))
    {
        errInfo = QString(lua_tostring(L, -1));
        lua_pop(L,1);
        return QVariantList();
    }
    QVariantList rets;
    for (int i = 0; i < nRet; ++i)
    {
        rets.append(getValue(L, false));
        lua_pop(L, 1);
    }
    std::reverse(rets.begin(), rets.end());
    return rets;
}

bool KApp::hasFunc(const QByteArrayList &tnames)
{
    if (!L || tnames.empty()) return false;
    int lastType = -1, i = 1;
    lastType = lua_getglobal(L, tnames[0].constData());
    for (; i < tnames.size(); ++i)
    {
        if (lastType != LUA_TTABLE) break;
        lastType = lua_getfield(L, -1, tnames[i].constData());
        lua_remove(L, -2);  // t_i
    }
    lua_pop(L, 1);
    return i == tnames.size() && lastType == LUA_TFUNCTION;
}

AppWidget *KApp::getWidget(const QString &name)
{
    if (!mainWindow) return nullptr;
    return mainWindow->findChild<AppWidget *>(name);
}

const QIcon &KApp::icon() const
{
    static QIcon defaultIcon;
    if (defaultIcon.isNull())
    {
        QPixmap iconPixmap(":/res/images/app.png");
        iconPixmap = iconPixmap.scaled(256, 256, Qt::KeepAspectRatioByExpanding);
        defaultIcon.addPixmap(iconPixmap, QIcon::Normal);
        defaultIcon.addPixmap(iconPixmap, QIcon::Selected);
    }
    return appIcon.isNull() ? defaultIcon : appIcon;
}

qint64 KApp::getLatestFileModifyTime() const
{
    const QString path = this->path();
    const QStringList paths = {
        path + "/app.json",
        path + "/app.xml",
        path + "/app.lua"
    };
    qint64 modifyTime = -1;
    for (const QString &file :paths)
    {
        if (!QFile::exists(file)) return -1;
        QFileInfo fileInfo(file);
        modifyTime = qMax(modifyTime, fileInfo.fileTime(QFile::FileModificationTime).toSecsSinceEpoch());
    }
    return modifyTime;
}

void KApp::addRes(const QString &key, AppRes *res)
{
    if (appResources.contains(res)) return;
    appResources.append(res);
    if (!key.isEmpty())
    {
        resHash[key] = res;
    }
}

AppRes *KApp::getRes(const QString &key)
{
    return resHash.value(key, nullptr);
}

void KApp::removeRes(const QString &key)
{
    auto iter = resHash.find(key);
    if (iter == resHash.end()) return;
    AppRes *r = iter.value();
    resHash.erase(iter);
    appResources.removeOne(r);
    delete r;
}

KApp::KApp() : QObject(nullptr), loaded(false), appThread(nullptr), L(nullptr), mainWindow(nullptr)
{

}

QString KApp::loadAppInfo(const QString &path)
{
    QString errInfo;
    QFile appJson(path);
    appJson.open(QFile::ReadOnly);
    if (!appJson.isOpen())
    {
        errInfo = "Open 'app.json' failed";
        return errInfo;
    }
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(appJson.readAll(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        errInfo = jsonError.errorString();
        return errInfo;
    }
    QJsonObject appJsonObj = document.object();
    for(auto iter = appJsonObj.constBegin(); iter != appJsonObj.constEnd(); ++iter)
    {
        appInfo.insert(iter.key(), iter.value().toVariant());
    }
    if (!appInfo.contains("name") || !appInfo.contains("id"))
    {
        errInfo = "Invalid app info";
        return errInfo;
    }
    appInfo["path"] = QFileInfo(path).absolutePath();
    appInfo["dataPath"] = QString("%1/extension/app_data/%2/").arg(GlobalObjects::dataPath, id());
    appInfo["time"] = QString::number(getLatestFileModifyTime());
    if (appInfo.contains("icon"))
    {
        QPixmap appIconPixmap = QPixmap(appInfo["path"].toString() + "/" + appInfo["icon"].toString());
        if (!appIconPixmap.isNull())
        {
            appIconPixmap = appIconPixmap.scaled(256, 256, Qt::KeepAspectRatioByExpanding);
            appIcon.addPixmap(appIconPixmap, QIcon::Normal);
            appIcon.addPixmap(appIconPixmap, QIcon::Selected);
        }
    }
    if(appInfo.contains("min_kiko"))
    {
        QStringList minVer(appInfo["min_kiko"].toString().split('.', Qt::SkipEmptyParts));
        QStringList curVer(QString(GlobalObjects::kikoVersion).split('.', Qt::SkipEmptyParts));
        bool versionMismatch = false;
        if(minVer.size() == curVer.size())
        {
            for(int i = 0; i < minVer.size(); ++i)
            {
                int cv = curVer[i].toInt(), mv = minVer[i].toInt();
                if(cv < mv)
                {
                    versionMismatch = true;
                    break;
                }
                else if(cv > mv)
                {
                    versionMismatch = false;
                    break;
                }
            }
        }
        if(versionMismatch)
        {
            errInfo = QString("Script Version Mismatch, min: %1, cur: %2").arg(appInfo["min_kiko"].toString());
        }
    }
    Logger::logger()->log(Logger::Extension, QString("[loadAppInfo]app: %1, id: %2").arg(name(), id()));
    return errInfo;
}

QString KApp::loadScript(const QString &path)
{
    if(!L) return "App Error: Lua State Error";
    QFile luaFile(path);
    luaFile.open(QFile::ReadOnly);
    if(!luaFile.isOpen()) return "Open Script File Failed";
    QByteArray luaScript(luaFile.readAll());
    QString errInfo;
    if(luaL_loadbuffer(L, luaScript.constData(), luaScript.size(), nullptr) || lua_pcall(L,0,0,0))
    {
        errInfo="Script Error: "+ QString(lua_tostring(L, -1));
        lua_pop(L,1);
    }
    return errInfo;
}

void KApp::setEnvInfo()
{
    if(!L) return;
    lua_getglobal(L, envTable);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);
    }

    lua_pushstring(L, "app_path");  // table key
    lua_pushstring(L, path().toUtf8().constData());  // tabel key value
    lua_rawset(L, -3);  //table

    lua_pushstring(L, "os"); // table key
    lua_pushstring(L, QSysInfo::productType().toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "os_version"); // table key
    lua_pushstring(L, QSysInfo::productVersion().toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "kikoplay"); // table key
    lua_pushstring(L,  GlobalObjects::kikoVersion); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "data_path"); // table key
    const QString dataPath = this->dataPath();
    QDir dir;
    if(!dir.exists(dataPath))  dir.mkpath(dataPath);
    lua_pushstring(L, dataPath.toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    lua_setglobal(L, envTable);

    lua_pushstring(L, KApp::globalAppObj);
    lua_pushlightuserdata(L, (void *)this);
    lua_settable(L, LUA_REGISTRYINDEX);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    QString packagePath = lua_tostring(L, -1);
    packagePath += ";" + path() + "\\?.lua";
    lua_pushstring(L, packagePath.toStdString().c_str());
    lua_setfield(L, -3, "path");
    lua_pop(L, 2);
}

void KApp::callAppLoaded(LaunchScene scene)
{
    const QVariantMap params = {
        {"window", QVariant::fromValue(this->mainWindow)},
        {"scene", static_cast<int>(scene)},
    };
    eventCall(appEventLoaded, params);
}

bool KApp::getTable(const QStringList &tnames)
{
    if (!L || tnames.empty()) return false;
    lua_getglobal(L, tnames[0].toUtf8().constData());
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }
    for (int i = 1; i < tnames.size(); ++i)
    {
        int type = lua_getfield(L, -1, tnames[i].toUtf8().constData());  // t t_i
        lua_remove(L, -2);  // t_i
        if (type != LUA_TTABLE)
        {
            lua_pop(L, 1);
            return false;
        }
    }
    return true;
}

void KApp::exitHook(lua_State *L, lua_Debug *ar)
{
    //lua_sethook(L, nullptr, 0, 0);
    KApp *app = KApp::getApp(L);
    Logger::logger()->log(Logger::Extension, QString("terminate: %1").arg(app? app->id() : "[unknown]"));
    luaL_error(L, "app terminate");
}

void KApp::setAppWindowEvent()
{
    AppFramelessDialog *w = static_cast<AppFramelessDialog *>(mainWindow->getWidget());
    w->setCloseCallback([this](){
        ThreadTask task(appThread);
        bool isClose = task.Run([=](){
            if (!hasFunc({appTable, appEventClose})) return true;
            QVariantList params = {QVariantMap()};
            QString errInfo;
            QVariantList rets = this->funcCall(this->appEventClose, params, 1, errInfo);
            if (!errInfo.isEmpty())
            {
                Logger::logger()->log(Logger::Extension, QString("[funcCall]error: %1").arg(errInfo));
                return true;
            }
            return rets.first().toBool();
        }).toBool();
        if (isClose)
        {
            this->stop();
            return true;
        }
        return false;
    });
    w->setHideCallback([this](){
        ThreadTask task(appThread);
        bool isHide = task.Run([=](){
            if (!hasFunc({appTable, appEventHide})) return true;
            QVariantList params = {QVariantMap()};
            QString errInfo;
            QVariantList rets = this->funcCall(this->appEventHide, params, 1, errInfo);
            if (!errInfo.isEmpty())
            {
                Logger::logger()->log(Logger::Extension, QString("[funcCall]error: %1").arg(errInfo));
                return true;
            }
            return rets.first().toBool();
        }).toBool();
        return isHide;
    });
}

void KApp::loadFont()
{
    if (!appInfo.contains("font")) return;
    const QStringList fonts = appInfo["font"].toStringList();
    QStringList loadedFontFamilies;
    for (const QString &f : fonts)
    {
        const QString path = this->path() + "/" + f;
        int fontId = QFontDatabase::addApplicationFont(path);
        this->appResources.append(new FontRes(fontId));
        loadedFontFamilies += QFontDatabase::applicationFontFamilies(fontId);
    }
    lua_getglobal(L, envTable);
    lua_pushstring(L, "fontfamily"); // table key
    pushValue(L, loadedFontFamilies); // table key value
    lua_rawset(L, -3);
    lua_pop(L, 1);
}



}

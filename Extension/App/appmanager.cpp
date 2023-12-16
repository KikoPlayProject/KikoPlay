#include "appmanager.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include "Common/threadtask.h"
#include "globalobjects.h"
#include "Common/notifier.h"

AppManager::AppManager(QObject *parent)
    : QAbstractItemModel{parent}
{
    refresh();
}

AppManager::~AppManager()
{
    for (auto app : appList)
    {
        app->close();
    }
}

void AppManager::refresh(bool inWorkerThread)
{
    QVector<QSharedPointer<Extension::KApp>> newAppList;
    if (inWorkerThread)
    {
        ThreadTask task(GlobalObjects::workThread);
        task.Run([this, &newAppList](){
            newAppList = this->refresApp();
            return 0;
        }); 
    }
    else
    {
        newAppList = refresApp();
    }
    beginResetModel();
    this->appList.swap(newAppList);
    endResetModel();
}

void AppManager::start(const QModelIndex &index)
{
    if (index.isValid() && index.row() < appList.size())
    {
        auto &app = appList.at(index.row());
        if (!app->isLoaded())
        {
            if (app->start())
            {
                QObject::connect(app.get(), &Extension::KApp::appClose, this, &AppManager::appTerminated);
                QObject::connect(app.get(), &Extension::KApp::appFlash, this, &AppManager::appFlash);
                emit appLaunched(app.get());
            }
            else
            {
                Notifier::getNotifier()->showMessage(Notifier::APP_MENU_NOTIFY, tr("App Launch Failed"), NM_HIDE | NM_ERROR);
            }
        }
        else
        {
            app->show();
        }
    }
}

void AppManager::autoStart()
{
    const QStringList apps = GlobalObjects::appSetting->value("Extension/AutoStartApps").toStringList();
    autoStartAppIds = QSet<QString>(apps.begin(), apps.end());
    for (auto app : appList)
    {
        if (autoStartAppIds.contains(app->id()))
        {
            app->setAutoStart(true);
            if (app->start(Extension::KApp::LaunchScene::LaunchScene_AutoStart))
            {
                QObject::connect(app.get(), &Extension::KApp::appClose, this, &AppManager::appTerminated);
                QObject::connect(app.get(), &Extension::KApp::appFlash, this, &AppManager::appFlash);
                emit appLaunched(app.get());
            }
        }
    }
}

QVariant AppManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    auto &app = appList.at(index.row());
    AppManager::Columns col = static_cast<Columns>(index.column());
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
    {
        switch (col)
        {
        case Columns::NAME:
            return app->name();
        case Columns::VERSION:
            return app->version();
        case Columns::DESC:
            return app->desc();
        default:
            return QVariant();
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (col == Columns::NAME)
        {
            return app->icon();
        }
    }
    else if (role == Qt::CheckStateRole)
    {
        if (col == Columns::AUTO_START)
            return app->autoStart()? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
    }
    return QVariant();
}

bool AppManager::setData(const QModelIndex &index, const QVariant &value, int)
{
    auto &app = appList.at(index.row());
    AppManager::Columns col = static_cast<Columns>(index.column());
    if (col == Columns::AUTO_START)
    {
        bool on = value == Qt::Checked;
        app->setAutoStart(on);
        if (on) autoStartAppIds.insert(app->id());
        else autoStartAppIds.remove(app->id());
        GlobalObjects::appSetting->setValue("Extension/AutoStartApps", QStringList(autoStartAppIds.begin(), autoStartAppIds.end()));
        emit dataChanged(index, index);
    }
    return false;
}

QVariant AppManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers({tr("Name"), tr("Version"), tr("Description"), tr("Auto Start")});
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if(section<headers.size()) return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags AppManager::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    Columns col = static_cast<Columns>(index.column());
    if (index.isValid())
    {
        if (col == Columns::AUTO_START)
        {
            return  Qt::ItemIsUserCheckable | defaultFlags;
        }
    }
    return defaultFlags;
}

QString AppManager::getAppPath() const
{
    const QString strApp(QCoreApplication::applicationDirPath()+"/extension/app");
#ifdef CONFIG_UNIX_DATA
    const QString strHome(QDir::homePath()+"/.config/kikoplay/extension/app");
    const QString strSys("/usr/share/kikoplay/extension/app");

    const QFileInfo fileinfoHome(strHome);
    const QFileInfo fileinfoSys(strSys);
    const QFileInfo fileinfoApp(strApp);

    QString appPathBase;

    if (fileinfoHome.exists() || fileinfoHome.isDir()) {
        appPathBase = strHome;
    } else if (fileinfoSys.exists() || fileinfoSys.isDir()) {
        appPathBase = strSys;
    } else {
        appPathBase = strApp;
    }
    return appPathBase;
#else
    return strApp;
#endif
}

QVector<QSharedPointer<Extension::KApp>> AppManager::refresApp()
{
    QVector<QSharedPointer<Extension::KApp>> appList = this->appList;
    for (int i = appList.size() - 1; i >= 0; --i)
    {
        QSharedPointer<Extension::KApp> app = appList[i];
        if (!QFile::exists(app->path()))
        {
            appList.removeAt(i);
        }
    }
    QSet<QString> existAppPaths;
    for (int i = appList.size() - 1; i >= 0; --i)
    {
        QSharedPointer<Extension::KApp> &app = appList[i];
        const qint64 modifyTime = app->getLatestFileModifyTime();
        if (modifyTime > app->time())
        {
            Extension::KApp *newApp = Extension::KApp::create(app->path());
            if (newApp)
            {
                app.reset(newApp);
            }
            else
            {
                appList.removeAt(i);
                continue;
            }
        }
        existAppPaths.insert(app->path());
    }
    const QString appRootPath(getAppPath());
    QDir folder(appRootPath);
    QVector<QSharedPointer<Extension::KApp>> newAppList;
    const QFileInfoList infoList = folder.entryInfoList();
    for (const QFileInfo &fileInfo : infoList)
    {
        if (fileInfo.isDir() && !existAppPaths.contains(fileInfo.absoluteFilePath()))
        {
            Extension::KApp *app = Extension::KApp::create(fileInfo.absoluteFilePath());
            if (app)
            {
                newAppList.append(QSharedPointer<Extension::KApp>(app));
            }
        }
    }
    std::sort(newAppList.begin(), newAppList.end(), [](const QSharedPointer<Extension::KApp> &l, const QSharedPointer<Extension::KApp> &r){
        return l->time() > r->time();
    });
    newAppList.append(appList);
    appList.swap(newAppList);
    return appList;
}


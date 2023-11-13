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

QVariant AppManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    auto &app = appList.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
    {
        return app->name();
    }
    else if (role == Qt::DecorationRole)
    {
        return app->icon();
    }
    return QVariant();
}

QString AppManager::getAppPath() const
{
    const QString strApp(QCoreApplication::applicationDirPath()+"/extension/app");
#ifdef CONFIG_UNIX_DATA
    const QString strHome(GlobalObjects::dataPath+"extension/app");
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


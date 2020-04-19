#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/Render/danmurender.h"
#include "Play/Playlist/playlist.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/blocker.h"
#include "Play/Danmu/providermanager.h"
#include "MediaLibrary/animelibrary.h"
#include "LANServer/lanserver.h"
#include "Download/downloadmodel.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Download/Script/scriptmanager.h"
#include "Download/autodownloadmanager.h"
#include "Common/kcache.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QApplication>
#include <QSqlError>
#include <QFileInfo>

MPVPlayer *GlobalObjects::mpvplayer=nullptr;
DanmuPool *GlobalObjects::danmuPool=nullptr;
DanmuRender *GlobalObjects::danmuRender=nullptr;
PlayList *GlobalObjects::playlist=nullptr;
Blocker *GlobalObjects::blocker=nullptr;
QThread *GlobalObjects::workThread=nullptr;
QSettings *GlobalObjects::appSetting=nullptr;
ProviderManager *GlobalObjects::providerManager=nullptr;
AnimeLibrary *GlobalObjects::library=nullptr;
DownloadModel *GlobalObjects::downloadModel=nullptr;
DanmuManager *GlobalObjects::danmuManager=nullptr;
LANServer *GlobalObjects::lanServer=nullptr;
ScriptManager *GlobalObjects::scriptManager=nullptr;
AutoDownloadManager *GlobalObjects::autoDownloadManager=nullptr;
KCache *GlobalObjects::kCache=nullptr;
QFont GlobalObjects::iconfont;
QString GlobalObjects::dataPath;
namespace  {
    const char *mt_db_names[]={"Comment_M", "Bangumi_M","Download_M"};
    const char *wt_db_names[]={"Comment_W", "Bangumi_W","Download_W"};
}
void GlobalObjects::init()
{
    const QFileInfo fileinfoConfig(QDir::homePath()+"/.config");
    /* Test Linux/MacOS style environment */
    if (fileinfoConfig.exists() || fileinfoConfig.isDir() || fileinfoConfig.isWritable()) {
        dataPath=QDir::homePath()+"/.config/kikoplay/data/";
    } else {
        dataPath=QCoreApplication::applicationDirPath()+"/data/";
    }

    QDir dir;
    if(!dir.exists(dataPath))
    {
        dir.mkpath(dataPath);
    }

    initDatabase(mt_db_names);
    appSetting=new QSettings(dataPath+"settings.ini",QSettings::IniFormat);
    workThread=new QThread();
    workThread->setObjectName(QStringLiteral("workThread"));
    workThread->start(QThread::NormalPriority);
    mpvplayer=new MPVPlayer();
    danmuPool=new DanmuPool();
    danmuRender=new DanmuRender();
    QObject::connect(mpvplayer,&MPVPlayer::positionChanged, danmuPool,&DanmuPool::mediaTimeElapsed);
    QObject::connect(mpvplayer,&MPVPlayer::positionJumped,danmuPool,&DanmuPool::mediaTimeJumped);
    playlist=new PlayList();
    QObject::connect(playlist, &PlayList::currentMatchChanged, danmuPool, &DanmuPool::setPoolID);
    blocker=new Blocker();
    QObject *workObj=new QObject();
    workObj->moveToThread(workThread);
    QMetaObject::invokeMethod(workObj,[workObj](){
        initDatabase(wt_db_names);
        workObj->deleteLater();
    },Qt::QueuedConnection);
    providerManager=new ProviderManager();
    library=new AnimeLibrary();
    downloadModel=new DownloadModel();
    danmuManager=new DanmuManager();
    lanServer=new LANServer();
    scriptManager=new ScriptManager();
    autoDownloadManager=new AutoDownloadManager();
    kCache=new KCache(128);

    int fontId = QFontDatabase::addApplicationFont(":/res/iconfont.ttf");
    QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
    iconfont.setFamily(fontFamilies.at(0));
	QApplication::setFont(QFont("Microsoft Yahei UI", 10));
}

void GlobalObjects::clear()
{ 
    workThread->quit();
    workThread->wait();
	mpvplayer->deleteLater();
	danmuRender->deleteLater();
	danmuPool->deleteLater();
	playlist->deleteLater();
    blocker->deleteLater();
    providerManager->deleteLater();
    appSetting->deleteLater();
    library->deleteLater();
    downloadModel->deleteLater();
    danmuManager->deleteLater();
    lanServer->deleteLater();
    scriptManager->deleteLater();
    autoDownloadManager->deleteLater();
    delete kCache;
}

QSqlDatabase GlobalObjects::getDB(int db)
{
    if(QThread::currentThread()->objectName()==QStringLiteral("workThread"))
    {
        return QSqlDatabase::database(wt_db_names[db]);
    }
    return QSqlDatabase::database(mt_db_names[db]);
}

void GlobalObjects::initDatabase(const char *db_names[])
{
    const int db_count=3;
    const char *db_files[]={"comment","bangumi","download"};
    for(int i=0;i<db_count;++i)
    {
        setDatabase(db_names[i],db_files[i]);
    }
}

void GlobalObjects::setDatabase(const char *name, const char *file)
{
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE",name);
    QString dbFile(dataPath+file+".db");
    bool dbFileExist = QFile::exists(dbFile);
    database.setDatabaseName(dbFile);
    database.open();
    QSqlQuery query(database);
    query.exec("PRAGMA foreign_keys = ON;");
    if(!dbFileExist)
    {
        QFile sqlFile(QString(":/res/db/%1.sql").arg(file));
        sqlFile.open(QFile::ReadOnly);
        if(sqlFile.isOpen())
        {
            QStringList sqls=QString(sqlFile.readAll()).split(';',QString::SkipEmptyParts);
            for(const QString &sql:sqls)
            {
                query.exec(sql);
            }
        }
    }
}

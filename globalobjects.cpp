#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/Render/danmurender.h"
#include "Play/Playlist/playlist.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/blocker.h"
#include "Play/Danmu/danmuprovider.h"
#include "Play/playcontext.h"
#include "MediaLibrary/animeprovider.h"
#include "LANServer/lanserver.h"
#include "Download/downloadmodel.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Extension/Script/scriptmanager.h"
#include "Extension/App/appmanager.h"
#include "Download/autodownloadmanager.h"
#include "Common/notifier.h"
#include "Common/eventbus.h"
#include "Common/logger.h"
#include "Common/kstats.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QApplication>
#include <QSqlError>
#include <QFileInfo> 
#include <QStandardPaths>

MPVPlayer *GlobalObjects::mpvplayer=nullptr;
DanmuPool *GlobalObjects::danmuPool=nullptr;
DanmuRender *GlobalObjects::danmuRender=nullptr;
PlayList *GlobalObjects::playlist=nullptr;
Blocker *GlobalObjects::blocker=nullptr;
QThread *GlobalObjects::workThread=nullptr;
QSettings *GlobalObjects::appSetting=nullptr;
DanmuProvider *GlobalObjects::danmuProvider=nullptr;
AnimeProvider *GlobalObjects::animeProvider=nullptr;
DownloadModel *GlobalObjects::downloadModel=nullptr;
DanmuManager *GlobalObjects::danmuManager=nullptr;
LANServer *GlobalObjects::lanServer=nullptr;
ScriptManager *GlobalObjects::scriptManager=nullptr;
AppManager *GlobalObjects::appManager=nullptr;
AutoDownloadManager *GlobalObjects::autoDownloadManager=nullptr;
QMainWindow *GlobalObjects::mainWindow=nullptr;
QFont* GlobalObjects::iconfont;
QString GlobalObjects::dataPath;

namespace  {
    const char *mt_db_names[]={"Comment_M", "Bangumi_M","Download_M"};
    const char *wt_db_names[]={"Comment_W", "Bangumi_W","Download_W"};
}

constexpr const char *GlobalObjects::normalFont;
constexpr const char *GlobalObjects::kikoVersion;

void GlobalObjects::init()
{
	dataPath = QCoreApplication::applicationDirPath() + "/data/";

#ifdef CONFIG_UNIX_DATA
    QStringList standardLocations = QStandardPaths::standardLocations(QStandardPaths::DataLocation);  
    const QFileInfo fileInfoConfig(standardLocations.first()); // Take the first location as a file path  
    /* Test Linux/MacOS style environment */  
    if (fileInfoConfig.exists() || !fileInfoConfig.isDir() || fileInfoConfig.isWritable()) {  
    dataPath = standardLocations.first() + "/"; // Set dataPath to the first location  
    }
#endif
    QDir dir;
    if(!dir.exists(dataPath))
        dir.mkpath(dataPath);

    registerCustomSettingType();
    appSetting=new QSettings(dataPath+"settings.ini",QSettings::IniFormat);

    Logger::logger();

    initDatabase(mt_db_names);

    auto locNames = GlobalObjects::appSetting->value("KikoPlay/Language", "").toStringList();
    if(locNames.join("").isEmpty()) {
        locNames = QLocale().uiLanguages();
    }
    static QTranslator translator;
    for(const auto &locName:locNames) {
        auto loc = QLocale(locName);
        if(loc.uiLanguages().first().startsWith("en-")) {
            break;
        }
        if(translator.load(loc, "", "", ":/res/lang")) {
            qApp->installTranslator(&translator);
            break;
        }
    }
    Notifier::getNotifier();
    EventBus::getEventBus();
    PlayContext::context();
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
    scriptManager = new ScriptManager();
    danmuProvider = new DanmuProvider();
    animeProvider = new AnimeProvider();
    downloadModel = new DownloadModel();
    danmuManager = new DanmuManager();
    lanServer = new LANServer();
    iconfont = new QFont();
    autoDownloadManager = new AutoDownloadManager();
    appManager = new AppManager();

    int fontId = QFontDatabase::addApplicationFont(":/res/iconfont.ttf");
    QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
    iconfont->setFamily(fontFamilies.at(0));
    QApplication::setFont(QFont("Microsoft Yahei UI", 10));
    KStats::instance();
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
    danmuProvider->deleteLater();
    animeProvider->deleteLater();
    downloadModel->deleteLater();
    danmuManager->deleteLater();
    lanServer->deleteLater();
    autoDownloadManager->deleteLater();
    scriptManager->deleteLater();
    appManager->deleteLater();
    appSetting->deleteLater();
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
            QStringList sqls=QString(sqlFile.readAll()).split(';', Qt::SkipEmptyParts);
            for(const QString &sql:sqls)
            {
                query.exec(sql);
            }
        }
    }
}

void GlobalObjects::registerCustomSettingType()
{
    using ShortCutInfo = QPair<QString, QPair<QString,QString>>;
    qRegisterMetaType<QList<ShortCutInfo>>("QList<ShortCutInfo>");
    qRegisterMetaTypeStreamOperators<QList<ShortCutInfo>>("QList<ShortCutInfo>");

    qRegisterMetaType<QVector<QPair<QString, QString>>>("QVector<QPair<QString, QString>>");
    qRegisterMetaTypeStreamOperators<QVector<QPair<QString, QString>>>("QVector<QPair<QString, QString>>");

    qRegisterMetaType<QVector<QStringList>>("QVector<QStringList>");
    qRegisterMetaTypeStreamOperators<QVector<QStringList>>("QVector<QStringList>");
}


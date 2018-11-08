#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/danmurender.h"
#include "Play/Playlist/playlist.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/blocker.h"
#include "Play/Danmu/providermanager.h"
#include "MediaLibrary/animelibrary.h"
#include "LANServer/lanserver.h"
#include "Download/downloadmodel.h"
#include "Play/Danmu/danmumanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QApplication>
#include <QSqlError>


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
QFont GlobalObjects::iconfont;

void GlobalObjects::init()
{
	initDatabase();
    appSetting=new QSettings(QCoreApplication::applicationDirPath()+"/settings.ini",QSettings::IniFormat);
    mpvplayer=new MPVPlayer();
    danmuPool=new DanmuPool();
    danmuRender=new DanmuRender();
    mpvplayer->setDanmuRender(danmuRender);
    QObject::connect(mpvplayer,&MPVPlayer::positionChanged, danmuPool,&DanmuPool::mediaTimeElapsed);
    QObject::connect(mpvplayer,&MPVPlayer::positionJumped,danmuPool,&DanmuPool::mediaTimeJumped);
    playlist=new PlayList();
	QObject::connect(playlist, &PlayList::currentMatchChanged, danmuPool, &DanmuPool::refreshCurrentPoolID);
    blocker=new Blocker();
    workThread=new QThread();
    workThread->setObjectName(QStringLiteral("workThread"));
    workThread->start(QThread::NormalPriority);
    QObject *workObj=new QObject();
    workObj->moveToThread(workThread);
    QMetaObject::invokeMethod(workObj,[workObj](){
        QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE","WT");
        database.setDatabaseName(QCoreApplication::applicationDirPath()+"\\kikoplay.db");
        database.open();
        QSqlQuery query(QSqlDatabase::database("WT"));
        query.exec("PRAGMA foreign_keys = ON;");
        workObj->deleteLater();
    },Qt::QueuedConnection);
    providerManager=new ProviderManager();
    library=new AnimeLibrary();
    downloadModel=new DownloadModel();
    danmuManager=new DanmuManager();
    lanServer=new LANServer();

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
}

void GlobalObjects::initDatabase()
{
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE","MT");
    bool dbFileExist = QFile::exists(QCoreApplication::applicationDirPath()+"\\kikoplay.db");
    database.setDatabaseName(QCoreApplication::applicationDirPath()+"\\kikoplay.db");
    database.open();
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.exec("PRAGMA foreign_keys = ON;");
    if(!dbFileExist)
    {
        QSqlQuery sqlQuery(QSqlDatabase::database("MT"));
        sqlQuery.exec("CREATE TABLE 'bangumi' (\
                        'PoolID'  TEXT(32) NOT NULL,\
                        'AnimeTitle'   TEXT,\
                        'Title'  TEXT,\
                        PRIMARY KEY ('PoolID') ON CONFLICT REPLACE\
                        );");
        sqlQuery.exec("CREATE UNIQUE INDEX 'PoolID'\
                      ON 'bangumi' ('PoolID' ASC);");
        sqlQuery.exec("CREATE TABLE 'danmu' ( \
                          'PoolID'  TEXT(32) NOT NULL,\
                          'Time'  INTEGER,\
                          'Date'  INTEGER,\
                          'Color'  INTEGER,\
                          'Mode'  INTEGER,\
                          'Size'  INTEGER,\
                          'Source'  INTEGER,\
                          'User'  TEXT,\
                          'Text'  TEXT,\
                          CONSTRAINT 'PoolID' FOREIGN KEY ('PoolID') REFERENCES 'bangumi' ('PoolID') ON DELETE CASCADE\
                          );");
        sqlQuery.exec("CREATE TABLE 'source' (\
                           'PoolID'  TEXT(32),\
                           'ID'  INTEGER,\
                           'Name'  TEXT,\
                           'Delay'  INTEGER,\
                           'URL'  TEXT,\
                           'TimeLine'  TEXT,\
                           CONSTRAINT 'PoolID' FOREIGN KEY ('PoolID') REFERENCES 'bangumi' ('PoolID') ON DELETE CASCADE\
                           );");
        sqlQuery.exec("CREATE TABLE 'match' (\
                          'MD5'  TEXT NOT NULL ON CONFLICT IGNORE,\
                          'PoolID'  TEXT(32) NOT NULL ON CONFLICT IGNORE,\
                          PRIMARY KEY ('MD5'),\
                          CONSTRAINT 'PoolID' FOREIGN KEY ('PoolID') REFERENCES 'bangumi' ('PoolID') ON DELETE CASCADE\
                          );");
        sqlQuery.exec("CREATE TABLE 'block' (\
                          'Id'  INTEGER NOT NULL,\
                          'Field'  INTEGER,\
                          'Relation'  INTEGER,\
                          'IsRegExp'  INTEGER,\
                          'Enable'  INTEGER,\
                          'Content'  TEXT,\
                          PRIMARY KEY ('Id' ASC)\
                          );");
        sqlQuery.exec("CREATE TABLE 'anime' (\
                          'Anime'  TEXT NOT NULL,\
                          'AddTime'  INTEGER,\
                          'EpCount'  INTEGER,\
                          'Summary'  TEXT,\
                          'Date'  TEXT,\
                          'Staff'  TEXT,\
                          'BangumiID'  INTEGER,\
                          'Cover'  BLOB,\
                          PRIMARY KEY ('Anime')\
                          );");
        sqlQuery.exec("CREATE TABLE 'character' (\
                          'Anime'  TEXT NOT NULL,\
                          'Name'  TEXT NOT NULL,\
                          'NameCN'  TEXT NOT NULL,\
                          'Actor'  TEXT,\
                          'BangumiID'  INTEGER,\
                          'Image'  BLOB,\
                          CONSTRAINT 'Anime' FOREIGN KEY ('Anime') REFERENCES 'anime' ('Anime') ON DELETE CASCADE ON UPDATE CASCADE\
                          );");
        sqlQuery.exec("CREATE TABLE 'eps' (\
                            'Anime'  TEXT NOT NULL,\
                            'Name'  TEXT,\
                            'LocalFile'  TEXT,\
                            'LastPlayTime'  TEXT,\
                            CONSTRAINT 'Anime' FOREIGN KEY ('Anime') REFERENCES 'anime' ('Anime') ON DELETE CASCADE ON UPDATE CASCADE\
                            );");
        sqlQuery.exec("CREATE TABLE 'download' (\
                             'TaskID'  TEXT NOT NULL,\
                             'Title'  TEXT,\
                             'Dir'  TEXT,\
                             'CTime'  INTEGER,\
                             'FTime'  INTEGER,\
                             'TLength'  INTEGER,\
                             'CLength'  INTEGER,\
                             'URI'  TEXT,\
                             'SFIndexes'  TEXT,\
                             'Torrent'  BLOB,\
                             PRIMARY KEY ('TaskID')\
                             );");
        sqlQuery.exec("CREATE TABLE 'tag' (\
                            'Anime'  TEXT NOT NULL,\
                            'Tag'  TEXT NOT NULL,\
                            PRIMARY KEY (\"Anime\", \"Tag\"),\
                            CONSTRAINT 'Anime' FOREIGN KEY ('Anime') REFERENCES 'anime' ('Anime') ON DELETE CASCADE ON UPDATE CASCADE\
                            );");
    }
}

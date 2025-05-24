#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/Render/danmurender.h"
#include "Play/Playlist/playlist.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/blocker.h"
#include "Play/Danmu/danmuprovider.h"
#include "Play/Subtitle/subtitletranslator.h"
#include "Play/playcontext.h"
#include "MediaLibrary/animeprovider.h"
#include "MediaLibrary/labelmodel.h"
#include "LANServer/lanserver.h"
#include "Download/downloadmodel.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Extension/Script/scriptmanager.h"
#include "Extension/App/appmanager.h"
#include "Download/autodownloadmanager.h"
#include "Common/notifier.h"
#include "Common/eventbus.h"
#include "Common/logger.h"
#include "Common/keyactionmodel.h"
#include "Common/dbmanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QApplication>
#include <QSqlError>
#include <QFileInfo> 
#include <QElapsedTimer>

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

constexpr const char *GlobalObjects::normalFont;
constexpr const char *GlobalObjects::kikoVersion;

void GlobalObjects::init(QElapsedTimer *elapsedTimer)
{
    context()->init();

    registerCustomSettingType();
    appSetting=new QSettings(context()->dataPath + "settings.ini", QSettings::IniFormat);
    Logger::logger();

    QThread::currentThread()->setObjectName(QStringLiteral("mainThread"));
    workThread = new QThread();
    workThread->setObjectName(QStringLiteral("workThread"));
    workThread->start(QThread::NormalPriority);
    GlobalObjects::context()->tick(elapsedTimer, "init");

    DBManager::instance();
    GlobalObjects::context()->tick(elapsedTimer, "db");

    auto locNames = GlobalObjects::appSetting->value("KikoPlay/Language", "").toStringList();
    if (locNames.join("").isEmpty())
    {
        locNames = QLocale().uiLanguages();
    }
    static QTranslator translator;
    for (const auto &locName:locNames)
    {
        auto loc = QLocale(locName);
        if (loc.uiLanguages().first().startsWith("en-")) break;
        if (translator.load(loc, "", "", ":/res/lang"))
        {
            qApp->installTranslator(&translator);
            GlobalObjects::context()->lang = locName;
            break;
        }
    }
    Notifier::getNotifier();
    EventBus::getEventBus();

    PlayContext::context();
    mpvplayer = new MPVPlayer();
    GlobalObjects::context()->tick(elapsedTimer, "player");

    danmuPool = new DanmuPool();
    danmuRender = new DanmuRender();
    QObject::connect(mpvplayer, &MPVPlayer::positionChanged, danmuPool, &DanmuPool::mediaTimeElapsed);
    QObject::connect(mpvplayer, &MPVPlayer::positionJumped, danmuPool, &DanmuPool::mediaTimeJumped);
    playlist = new PlayList();
    QObject::connect(playlist, &PlayList::currentMatchChanged, danmuPool, &DanmuPool::setPoolID);
    blocker = new Blocker();
    GlobalObjects::context()->tick(elapsedTimer, "list");

    scriptManager = new ScriptManager();
    danmuProvider = new DanmuProvider();
    animeProvider = new AnimeProvider();
    QObject workObj;
    workObj.moveToThread(workThread);
    QMetaObject::invokeMethod(&workObj, [](){
        LabelModel::instance()->loadLabels();
    }, Qt::QueuedConnection);
    GlobalObjects::context()->tick(elapsedTimer, "script");

    downloadModel = new DownloadModel();
    autoDownloadManager = new AutoDownloadManager();
    danmuManager = new DanmuManager();
    GlobalObjects::context()->tick(elapsedTimer, "dm");

    lanServer = new LANServer();
    GlobalObjects::context()->tick(elapsedTimer, "lan");

    appManager = new AppManager();
    GlobalObjects::context()->tick(elapsedTimer, "ext");

    iconfont = new QFont();
    QFontDatabase::addApplicationFont(":/res/ElaAwesome.ttf");
    int fontId = QFontDatabase::addApplicationFont(":/res/iconfont.ttf");
    QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
    iconfont->setFamily(fontFamilies.at(0));

    QFont font = qApp->font();
    font.setPixelSize(13);
    font.setFamily("Microsoft YaHei");
    font.setHintingPreference(QFont::PreferNoHinting);
    qApp->setFont(font);
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

GlobalContext *GlobalObjects::context()
{
    static GlobalContext context;
    return &context;
}

void GlobalObjects::registerCustomSettingType()
{
    qRegisterMetaType<QVector<QPair<QString, QString>>>("QVector<QPair<QString, QString>>");

    qRegisterMetaType<QVector<QStringList>>("QVector<QStringList>");

    qRegisterMetaType<QList<QSharedPointer<KeyActionItem>>>("QList<QSharedPointer<KeyActionItem>>");

    qRegisterMetaType<QList<TranslatorConfig>>("QList<TranslatorConfig>");
}


void GlobalContext::init()
{
    dataPath = QCoreApplication::applicationDirPath() + "/data/";

#ifdef CONFIG_UNIX_DATA
    const QFileInfo fileinfoConfig(QDir::homePath() + "/.config");
    /* Test Linux/MacOS style environment */
    if (fileinfoConfig.exists() || fileinfoConfig.isDir() || fileinfoConfig.isWritable()) {
        dataPath = QDir::homePath() + "/.config/kikoplay/data/";
    }
#endif

    QDir dir;
    if (!dir.exists(dataPath))
    {
        dir.mkpath(dataPath);
    }

    tmpPath = dataPath + "tmp/";
    if (!dir.exists(tmpPath))
    {
        dir.mkpath(tmpPath);
    }
}

qint64 GlobalContext::tick(QElapsedTimer *timer, const QString &step)
{
    if (!timer) return 0;
    qint64 elapsed = timer->restart();
    startupTime += elapsed;
    stepTime.append(QPair<QString, qint64>(step, elapsed));
    return elapsed;
}

QIcon GlobalContext::getFontIcon(QChar iconChar, QColor fontColor)
{
    QPixmap pix(64, 64);
    pix.fill(Qt::transparent);
    QPainter painter;
    painter.begin(&pix);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setPen(fontColor);
    GlobalObjects::iconfont->setPixelSize(64);
    painter.setFont(*GlobalObjects::iconfont);
    painter.drawText(pix.rect(), Qt::AlignCenter, iconChar);
    painter.end();
    return QIcon(pix);
}

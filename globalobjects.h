#ifndef GLOBALOBJECTS_H
#define GLOBALOBJECTS_H
#include <QString>
#include <QVector>
#include <QIcon>
class MPVPlayer;
class DanmuPool;
class DanmuRender;
class PlayList;
class Blocker;
class QFont;
class QThread;
class QObject;
class QSettings;
class DanmuProvider;
class AnimeProvider;
class LabelModel;
class DownloadModel;
class DanmuManager;
class LANServer;
class ScriptManager;
class AppManager;
class AutoDownloadManager;
class QMainWindow;
class QElapsedTimer;

struct GlobalContext
{
    void init();

    QString lang{"en-US"};

    QString dataPath;
    QString tmpPath;

    float devicePixelRatioF{1.0f};

    int curMainPage{0};

    qint64 startupTime;
    QVector<QPair<QString, qint64>> stepTime;
    qint64 tick(QElapsedTimer *timer, const QString &step);

    QIcon getFontIcon(QChar iconChar, QColor fontColor);
};

class GlobalObjects
{
public:
    static void init(QElapsedTimer *elapsedTimer = nullptr);
    static void clear();
    static MPVPlayer *mpvplayer;
    static DanmuPool *danmuPool;
    static DanmuRender *danmuRender;
    static PlayList *playlist;
    static Blocker *blocker;
    static QFont* iconfont;
    static QThread *workThread;
    static QSettings *appSetting;
    static DanmuProvider *danmuProvider;
    static AnimeProvider *animeProvider;
    static DownloadModel *downloadModel;
    static DanmuManager *danmuManager;
    static LANServer *lanServer;    
    static ScriptManager *scriptManager;
    static AppManager *appManager;
    static AutoDownloadManager *autoDownloadManager;
    static QMainWindow *mainWindow;

    static GlobalContext *context();

    static constexpr const char *normalFont = "Microsoft Yahei UI";
    static constexpr const char *kikoVersion = "2.0.0";


    static void registerCustomSettingType();
};
#endif // GLOBALOBJECTS_H

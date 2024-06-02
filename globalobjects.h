#ifndef GLOBALOBJECTS_H
#define GLOBALOBJECTS_H
#include <QString>
#include <QSqlDatabase>
#include <QVector>
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
    static QString dataPath;

    static constexpr const int Comment_DB=0;
    static constexpr const int Bangumi_DB=1;
    static constexpr const int Download_DB=2;
    static QSqlDatabase getDB(int db, bool *hasError = nullptr);

    static constexpr const char *normalFont = "Microsoft Yahei UI";
    static constexpr const char *kikoVersion = "1.0.3";

    static qint64 startupTime;
    static QVector<QPair<QString, qint64>> stepTime;
    static qint64 tick(QElapsedTimer *timer, const QString &step);
private:
    static void initDatabase(const char *db_names[]);
    static void setDatabase(const char *name, const char *file);
    static void registerCustomSettingType();
};
#endif // GLOBALOBJECTS_H

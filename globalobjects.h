#ifndef GLOBALOBJECTS_H
#define GLOBALOBJECTS_H
#include <QString>
class MPVPlayer;
class DanmuPool;
class DanmuRender;
class PlayList;
class Blocker;
class QFont;
class QThread;
class QObject;
class QSettings;
class ProviderManager;
class AnimeLibrary;
class DownloadModel;
class DanmuManager;
class LANServer;
class GlobalObjects
{
public:
    static void init();
    static void clear();
    static MPVPlayer *mpvplayer;
    static DanmuPool *danmuPool;
    static DanmuRender *danmuRender;
    static PlayList *playlist;
    static Blocker *blocker;
    static QFont iconfont;
    static QThread *workThread;
    static QSettings *appSetting;
    static ProviderManager *providerManager;
    static AnimeLibrary *library;
    static DownloadModel *downloadModel;
    static DanmuManager *danmuManager;
    static LANServer *lanServer;    
    static QString dataPath;
private:
    static void initDatabase(const char *db_names[]);
    static void setDatabase(const char *name, const char *file);
};
enum PopMessageFlag
{
    PM_HIDE=1,
    PM_PROCESS=2,
    PM_INFO=4,
    PM_OK=8

};
#endif // GLOBALOBJECTS_H

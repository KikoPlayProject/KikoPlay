#ifndef GLOBALOBJECTS_H
#define GLOBALOBJECTS_H
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
private:
    static void initDatabase();
};
enum ListPopMessageFlag
{
    LPM_HIDE=1,
    LPM_PROCESS=2,
    LPM_INFO=4,
    LPM_OK=8

};
#endif // GLOBALOBJECTS_H

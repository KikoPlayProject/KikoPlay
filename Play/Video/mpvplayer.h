#ifndef MPVPLAYER_H
#define MPVPLAYER_H

#include <QOpenGLWidget>
#include <QtCore>
#include <QtGui>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <mpv/qthelper.hpp>
#include "Play/Danmu/common.h"
#include "mpvpreview.h"
class DanmuRender;
class MPVPlayer : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit MPVPlayer(QWidget *parent = nullptr);
    ~MPVPlayer();

    enum PlayState
    {
        Play,
        Pause,
        Stop,
        EndReached
    };
    struct VideoSizeInfo
    {
        int width;
        int height;
        int dwidth;
        int dheight;
    };
    struct ChapterInfo
    {
        QString title;
        int position;
    };

    const QStringList videoFileFormats = {"*.mp4","*.mkv","*.avi","*.flv","*.wmv"};
    const QStringList subtitleFormats = {"*.sub","*.srt","*.ass","*.ssa","*.smi","*.rt","*.txt","*.mks","*.vtt","*.sup"};
    const QStringList speedLevel={"0.5","0.75","1","1.5","2"};
    const QStringList videoAspect={tr("Auto"),"4:3","16:9","2.35:1"};
	const double videoAspcetVal[4] = { -1,4.0 / 3,16.0 / 9,2.35 / 1 };

    inline PlayState getState() const {return state;}
    inline bool getDanmuHide() const{return danmuHide;}
    inline bool getMute() const{return mute;}
    inline const QStringList &getTrackList(int type){return type==0?audioTrack.desc_str:subtitleTrack.desc_str;}
    inline int getCurrentAudioTrack() const {return audioTrack.ids.indexOf(mpv::qt::get_property(mpv,"aid").toInt());}
    inline int getCurrentSubTrack() const{return subtitleTrack.ids.indexOf(mpv::qt::get_property(mpv,"sid").toInt());}
    inline double getTime() const{return mpv::qt::get_property(mpv,"playback-time").toDouble();}
    inline int getDuration() const{return currentDuration;}
    inline QString getMediaTitle() const {return mpv::qt::get_property(mpv,"media-title").toString();}
    inline const QList<ChapterInfo> &getChapters() const {return chapters;}
    inline QPixmap *getPreview(int timePos, bool refresh=true) { if(!mpvPreview) return nullptr; return mpvPreview->getPreview(timePos, refresh);}
    inline const QString &getCurrentFile() const {return currentFile;}
    inline int getVolume() const {return volume;}
    QString getMPVProperty(const QString &property, bool &hasError);

    VideoSizeInfo getVideoSizeInfo();
    QString expandMediaInfo(const QString &text);
    void setOptions();
    void drawTexture(QList<const DanmuObject *> &objList, float alpha);
signals:
    void fileChanged();
    void durationChanged(int value);
    void positionChanged(int value);
    void positionJumped(int value);
    void stateChanged(PlayState state);
    void trackInfoChange(int type);
    void chapterChanged();
    void initContext();
    void showLog(const QString &log);
    void refreshPreview(int time, QPixmap *pixmap);
public slots:   
    void setMedia(const QString &file);
    void setState(PlayState newState);
    void seek(int pos,bool relative=false);
	void frameStep(bool forward = true);
    void setVolume(int volume);
    void setMute(bool mute);
    void hideDanmu(bool hide);
    void addSubtitle(const QString &path);
    void setTrackId(int type,int id);
    void hideSubtitle(bool on);
    void setSubDelay(int delay);
    void setSpeed(int index);
    void setVideoAspect(int index);
    void screenshot(const QString &filename);
    void setBrightness(int val);
    void setContrast(int val);
    void setSaturation(int val);
    void setGamma(int val);
    void setHue(int val);
    void setSharpen(int val);

protected:
    void initializeGL() override;
    void paintGL() override;
private slots:
    void swapped();
    void on_mpv_events();
    void maybeUpdate();
private:
    struct TrackInfo
    {
      QStringList desc_str;
      QList<int> ids;
    };
    mpv_handle *mpv;
    mpv_render_context *mpv_gl;
    void handle_mpv_event(mpv_event *event);
    static void on_update(void *ctx);
    static void wakeup(void *ctx);
    static void *get_proc_address(void *ctx, const char *name);

    const int timeRefreshInterval=200;
    PlayState state;
    bool mute;
    bool danmuHide;
    int volume;
    bool oldOpenGLVersion;
    QString currentFile;
    QOpenGLShaderProgram danmuShader;
    QTimer refreshTimer;
    QElapsedTimer elapsedTimer;
    QMap<QString, QString> optionsMap;

    int currentDuration;
    TrackInfo audioTrack,subtitleTrack;
    void loadTracks();

    QList<ChapterInfo> chapters;
    void loadChapters();

    MPVPreview *mpvPreview;
    QThread *previewThread;

    inline int setMPVCommand(const QVariant& params);
    inline void setMPVProperty(const QString& name, const QVariant& value);
};

#endif // MPVPLAYER_H

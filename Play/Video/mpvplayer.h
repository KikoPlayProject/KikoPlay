#ifndef MPVPLAYER_H
#define MPVPLAYER_H

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QtCore>
#include <QtGui>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <mpv/qthelper.hpp>
#include "Play/Danmu/common.h"
#include "mpvpreview.h"
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
    enum TrackType
    {
        AudioTrack,
        SubTrack
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
    struct TrackInfo
    {
        QString title;
        int id;
        bool isExternal;
        QString externalFile;
        QString codec;
        int ffIndex;
    };

    const QStringList videoFileFormats{"*.mp4","*.mkv","*.avi","*.flv","*.wmv","*.webm","*.vob","*.mts","*.ts","*.m2ts","*.mov","*.rm","*.rmvb","*.asf","*.m4v","*.mpg","*.mp2","*.mpeg","*.mpe","*.mpv","*.m2v","*.m4v","*.3gp","*.f4v"};
    const QStringList audioFormats{"*.mp3","*.wav","*.wma","*.ogg","*.flac","*.aac","*.ape","*.ac3","*.m4a","*.mka"};
    const QStringList subtitleFormats{"*.sub","*.srt","*.ass","*.ssa","*.smi","*.rt","*.txt","*.mks","*.vtt","*.sup"};
    const QStringList speedLevel{"0.5","0.75","1","1.25","1.5","2"};
    const QStringList videoAspect{tr("Auto"),"4:3","16:9","2.35:1"};
    const double videoAspectVal[4] = { -1,4.0 / 3,16.0 / 9,2.35 / 1 };

    inline PlayState getState() const {return state;}
    inline bool getDanmuHide() const{return danmuHide;}
    inline bool getMute() const{return mute;}
    inline bool getSeekable() const { return seekable; }
    inline bool isLocalFile() const { return curIsLocalFile; }
    inline bool needPause() const { return !currentFile.isEmpty() && state == MPVPlayer::Play && seekable; }
    inline const QVector<TrackInfo> &getTrackList(TrackType type){return type==AudioTrack?audioTracks:subTracks;}
    inline double getTime() const{return mpv::qt::get_property(mpv,"playback-time").toDouble();}
    inline int getDuration() const{return currentDuration;}
    inline QString getMediaTitle() const {return mpv::qt::get_property(mpv,"media-title").toString();}
    inline const QVector<ChapterInfo> &getChapters() const {return chapters;}
    inline QPixmap *getPreview(int timePos, bool refresh=true) { if(!mpvPreview || !curIsLocalFile) return nullptr; return mpvPreview->getPreview(timePos, refresh);}
    inline const QString &getCurrentFile() const {return currentFile;}
    inline int getVolume() const {return volume;}
    inline qint64 getCacheSpeed() const { return cacheSpeed; }
    inline bool isWebSource() const { return mpv::qt::get_property(mpv,"demuxer-via-network").toBool(); }
    QString getMPVProperty(const QString &property, int &errCode);
    QVariant getMPVPropertyVariant(const QString &property, int &errCode);
    int getCurrentTrack(TrackType type) const;
    int getExternalTrackCount(TrackType type) const;
    int getVideoAspectIndex() const { return videoAspectIndex; }
    double getPlaySpeed() const { return playSpeed; }
    int getBrightness() const { return brightness; }
    int getContrast() const { return contrast; }
    int getSaturation() const { return saturation; }
    int getGamma() const { return gamma; }
    int getHue() const { return hue; }
    int getSharpen() const { return sharpen; }
    int getClickBehavior() const { return clickBehavior; }
    int getDbClickBehavior() const { return dbClickBehaivior; }
    bool getShowPreview() const { return isShowPreview; }
    bool getShowRecent() const { return isShowRecent; }
    bool getUseSample2DArray() const { return useSample2DArray; }
    QString getSubAuto() const { return subAuto; }
    QString getSubFont() const { return subFont; }
    int getSubFontSize() const { return subFontSize; }
    bool getSubBold() const { return subBold; }
    unsigned int getSubColor() const { return subColor; }
    unsigned int getSubOutlineColor() const { return subOutlineColor; }
    unsigned int getSubBackColor() const { return subBackColor; }
    double getSubOutlineSize() const { return subOutlineSize; }
    QString getSubBorderStyle() const { return subBorderStyle; }

    VideoSizeInfo getVideoSizeInfo();
    QString expandMediaInfo(const QString &text);
    void setIccProfileOption();
    void drawTexture(QVector<const DanmuObject *> &objList, float alpha);

    void loadOptions();
    void loadSettings();
    void loadPredefineOptions(QStringList &optionGroupKeys, QVector<QStringList> &optionsGroupList);
    bool setOptionGroup(const QString &key);
    const QStringList &allOptionGroups() const {return optionGroupKeys;}
    const QString &currentOptionGroup() const {return curOptionGroup;}

    int runCommand(const QVariant& params) { return setMPVCommand(params); }
signals:
    void fileChanged();
    void durationChanged(int value);
    void positionChanged(int value);
    void positionJumped(int value);
    void stateChanged(PlayState state);
    void trackInfoChange(TrackType type);
    void chapterChanged();
    void subDelayChanged(int value);
    void speedChanged(double value);
    void brightnessChanged(int value);
    void contrastChanged(int value);
    void saturationChanged(int value);
    void gammaChanged(int value);
    void hueChanged(int value);
    void sharpenChanged(double value);
    void volumeChanged(int value);
    void muteChanged(bool value);
    void seekableChanged(bool value);
    void bufferingStateChanged(int value);
    void optionGroupChanged();
    void showRecentChanged(bool on);
    void toggleFullScreen();
    void toggleMiniMode();
    void triggerStop();

    void initContext();
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
    void addAudioTrack(const QString &path);
    void clearExternalAudio();
    void setTrackId(TrackType type, int index);
    void hideSubtitle(bool on);
    void setSubDelay(int delay);
    void clearExternalSub();
    void setSpeed(double speed);
    void setVideoAspect(int index);
    void screenshot(const QString &filename);
    void setBrightness(int val);
    void setContrast(int val);
    void setSaturation(int val);
    void setGamma(int val);
    void setHue(int val);
    void setSharpen(int val);
    void setClickBehavior(int val);
    void setDbClickBehavior(int val);
    void setShowPreview(bool on);
    void setShowRecent(bool on);
    void setUseSample2DArray(bool on);
    void setSubAuto(const QString val);
    void setSubFont(const QString val);
    void setSubFontSize(int val);
    void setSubBold(bool on);
    void setSubColor(QColor color);
    void setSubOutlineColor(QColor color);
    void setSubBackColor(QColor color);
    void setSubOutlineSize(double size);
    void setSubBorderStyle(const QString val);

protected:
    void initializeGL() override;
    void paintGL() override;
private slots:
    void swapped();
    void on_mpv_events();
    void maybeUpdate();
private:
    mpv_handle *mpv;
    mpv_render_context *mpv_gl;
    void handle_mpv_event(mpv_event *event);
    static void on_update(void *ctx);
    static void wakeup(void *ctx);
    static void *get_proc_address(void *ctx, const char *name);

    const int timeRefreshInterval=400;
    PlayState state;
    bool mute;
    bool seekable;
    bool curIsLocalFile;
    bool danmuHide;
    int volume;
    int videoAspectIndex;
    bool oldOpenGLVersion;
    double playSpeed;
    qint64 cacheSpeed;
    int brightness;
    int contrast;
    int saturation;
    int gamma;
    int hue;
    int sharpen;
    QString currentFile;
    QOpenGLShaderProgram danmuShader;
    QTimer refreshTimer;
    qint64 refreshTimestamp;
    QElapsedTimer elapsedTimer;
    int clickBehavior{0}, dbClickBehaivior{0};
    bool isShowPreview;
    bool isShowRecent;
    bool useSample2DArray;

    QString subAuto;
    QString subFont;
    int subFontSize;
    bool subBold;
    unsigned int subColor;
    unsigned int subOutlineColor;
    unsigned int subBackColor;
    double subOutlineSize;
    QString subBorderStyle;

    QString curOptionGroup;
    QStringList optionGroupKeys;
    QHash<QString, QMap<QString, QString>> optionsGroupMap;

    int currentDuration;
    QVector<TrackInfo> audioTracks, subTracks;
    void loadTracks();
    void loadTracks(const QVariantList &allTracks);

    QVector<ChapterInfo> chapters;
    void loadChapters();

    MPVPreview *mpvPreview;
    QThread *previewThread;

    int setMPVCommand(const QVariant& params);
    void setMPVProperty(const QString& name, const QVariant& value);
};

#endif // MPVPLAYER_H

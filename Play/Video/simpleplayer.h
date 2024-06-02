#ifndef SIMPLEPLAYER_H
#define SIMPLEPLAYER_H
#include <QOpenGLWidget>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <mpv/qthelper.hpp>


class SimplePlayer : public QOpenGLWidget
{
    Q_OBJECT
public:
    SimplePlayer(QWidget *parent = nullptr);
    ~SimplePlayer();
    enum PlayState
    {
        Play,
        Pause,
        EndReached,
        Stop,
        Unknown
    };
public:
    inline int getTime() const{return mpv::qt::get_property(mpv,"playback-time").toDouble();}
    inline double getDuration() const{return currentDuration;}
    const QSize getVideoSize();
    QVariant getMPVPropertyVariant(const QString &property, int &errCode);
    int setMPVCommand(const QVariant& params);
    void setClickPause(bool on) { clickPause = on; }
    bool isClickPause() const { return clickPause; }

signals:
    void durationChanged(double value);
    void positionChanged(double value);
    void stateChanged(PlayState state);
    void mouseWheel(int delta);
    void mouseClick();
public:
    void setMedia(const QString &file);
    void setState(PlayState newState);
    void seek(double pos);
    void setVolume(int volume);
    void screenshot(const QString &fileName);
    void setMute(bool mute);
protected:
    void initializeGL() override;
    void paintGL() override;
private slots:
    void on_mpv_events();
private:
    static void on_update(void *ctx);
    static void wakeup(void *ctx);

    mpv_handle *mpv;
    mpv_render_context *mpv_gl;

    QTimer *refreshTimer;
    const int timeRefreshInterval=400;
    PlayState state;
    double currentDuration;
    int volume;
    bool clickPause = true;
    double currentPos = 0;

    void handle_mpv_event(mpv_event *event);
    inline int setCommand(const QVariant& params);
    inline void setProperty(const QString& name, const QVariant& value);

    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
};

#endif // SIMPLEPLAYER_H

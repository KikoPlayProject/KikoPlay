#include "simpleplayer.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QTimer>
#include <stdexcept>
namespace
{
static void *get_proc_address(void *ctx, const char *name) {
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx) return nullptr;
    return reinterpret_cast<void *>(glctx->getProcAddress(QByteArray(name)));
}
}
SimplePlayer::SimplePlayer(QWidget *parent): QOpenGLWidget(parent)
{
    mpv = mpv_create();
    if (!mpv)  throw std::runtime_error("could not create mpv context");

    if (mpv_initialize(mpv) < 0) throw std::runtime_error("could not initialize mpv context");


    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "keep-open", "yes");
    mpv_set_option_string(mpv, "vo", "libmpv");

    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "eof-reached", MPV_FORMAT_FLAG);

    mpv_set_wakeup_callback(mpv, wakeup, this);

    refreshTimer = new QTimer(this);
    QObject::connect(refreshTimer,&QTimer::timeout,[this](){
       double curTime = mpv::qt::get_property(mpv,QStringLiteral("playback-time")).toDouble();
       if (curTime == currentPos) return;
       currentPos = curTime;
       emit positionChanged(curTime);
    });
}

SimplePlayer::~SimplePlayer()
{
    makeCurrent();
    if (mpv_gl) mpv_render_context_free(mpv_gl);
    mpv_terminate_destroy(mpv);
}

const QSize SimplePlayer::getVideoSize()
{
    return QSize(mpv::qt::get_property(mpv,"width").toInt(), mpv::qt::get_property(mpv,"height").toInt());
}

QVariant SimplePlayer::getMPVPropertyVariant(const QString &property, int &errCode)
{
    auto ret = mpv::qt::get_property(mpv, property);
    errCode = mpv::qt::get_error(ret);
    return ret;
}

int SimplePlayer::setMPVCommand(const QVariant &params)
{
    return mpv::qt::get_error(mpv::qt::command(mpv, params));
}

void SimplePlayer::setMedia(const QString &file)
{
    if(!setCommand(QStringList() << "loadfile" << file))
    {
        state = PlayState::Play;
        currentPos = 0;
        refreshTimer->start(timeRefreshInterval);
        setProperty("pause",false);
        setProperty("volume",volume);
        emit stateChanged(state);
    }
}

void SimplePlayer::setState(SimplePlayer::PlayState newState)
{
    if(newState==state)return;
    switch (newState)
    {
    case PlayState::Play:
        setProperty("pause",false);
        break;
    case PlayState::Pause:
        setProperty("pause",true);
        break;
    case PlayState::Stop:
        setCommand(QVariantList()<<"stop");
        refreshTimer->stop();
        currentPos = 0;
    default:
        break;
    }
}

void SimplePlayer::seek(double pos)
{
    setCommand(QVariantList() << "seek" << (double)pos << "absolute");
}

void SimplePlayer::setVolume(int vol)
{
    volume = vol;
    setProperty("volume",volume);
}

void SimplePlayer::screenshot(const QString &fileName)
{
    setCommand(QVariantList() << "screenshot-to-file" << fileName);
}

void SimplePlayer::setMute(bool mute)
{
    setProperty("mute",mute);
}

void SimplePlayer::initializeGL()
{
    QOpenGLFunctions *glFuns=context()->functions();
    glFuns->initializeOpenGLFunctions();
    mpv_opengl_init_params gl_init_params{get_proc_address, nullptr};
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
        throw std::runtime_error("failed to initialize mpv GL context");
    mpv_render_context_set_update_callback(mpv_gl, SimplePlayer::on_update, reinterpret_cast<void *>(this));
}

void SimplePlayer::paintGL()
{
    mpv_opengl_fbo mpfbo{static_cast<int>(defaultFramebufferObject()),
                static_cast<int>(width()*devicePixelRatioF()),  static_cast<int>(height()*devicePixelRatioF()), 0};
    int flip_y{1};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    mpv_render_context_render(mpv_gl, params);
}

void SimplePlayer::on_update(void *ctx)
{
    QMetaObject::invokeMethod((SimplePlayer*)ctx, "update");
}

void SimplePlayer::wakeup(void *ctx)
{
    QMetaObject::invokeMethod((SimplePlayer*)ctx, "on_mpv_events", Qt::QueuedConnection);
}

void SimplePlayer::on_mpv_events()
{
    while (mpv)
    {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;
        handle_mpv_event(event);
    }
}

void SimplePlayer::handle_mpv_event(mpv_event *event)
{
    switch (event->event_id)
    {
    case MPV_EVENT_PROPERTY_CHANGE:
    {
        mpv_event_property *prop = (mpv_event_property *)event->data;
        if (strcmp(prop->name, "duration") == 0)
        {
            if (prop->format == MPV_FORMAT_DOUBLE)
            {
                double time = *(double *)prop->data;
                emit durationChanged(time);
            }
        }
        else if (strcmp(prop->name, "pause") == 0)
        {
            if (prop->format == MPV_FORMAT_FLAG)
            {
                int flag = *(int *)prop->data;
                state=flag?PlayState::Pause:PlayState::Play;
                if(state==PlayState::Pause)
                    refreshTimer->stop();
                else
                    refreshTimer->start(timeRefreshInterval);
                emit stateChanged(state);
            }
        }
        else if (strcmp(prop->name, "eof-reached") == 0)
        {
            if (prop->format == MPV_FORMAT_FLAG)
            {
                int flag = *(int *)prop->data;
                if(flag)
                {
                    state = PlayState::EndReached;
                    refreshTimer->stop();
                    emit stateChanged(state);
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

int SimplePlayer::setCommand(const QVariant &params)
{
    return mpv::qt::get_error(mpv::qt::command_variant(mpv, params));
}

void SimplePlayer::setProperty(const QString &name, const QVariant &value)
{
    mpv::qt::set_property(mpv,name,value);
}

void SimplePlayer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (clickPause)
        {
            setProperty("pause", state == PlayState::Play);
        }
        emit mouseClick();
    }
}

void SimplePlayer::wheelEvent(QWheelEvent *event)
{
    emit mouseWheel(event->angleDelta().y());
}

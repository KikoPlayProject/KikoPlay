#include "mpvmediainfo.h"

#include <QOffscreenSurface>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QPixmap>
#include <QThread>

namespace
{
static void *get_proc_address(void *ctx, const char *name) {
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx) return nullptr;
    return reinterpret_cast<void *>(glctx->getProcAddress(QByteArray(name)));
}
}

MPVMediaInfo::~MPVMediaInfo()
{
    ctx->makeCurrent(pSurface);
    if (pFbo)
    {
        pFbo->release();
        delete pFbo;
        pFbo = nullptr;
    }
    ctx->doneCurrent();
    delete ctx;
    ctx = nullptr;

    if (infoThread)
    {
        infoThread->quit();
    }
}

bool MPVMediaInfo::loadFile(const QString &path)
{
    locker.lock();
    filePromise.reset(new std::promise<bool>);
    auto fileLoadedFuture = filePromise->get_future();
    if (mpv::qt::is_error(mpv::qt::command_variant(mpv, QStringList() << "loadfile" << path)))
    {
        locker.unlock();
        return false;
    }
    mpv::qt::set_property(mpv, "pause", true);
    previewCache.clear();
    posSet.clear();

    auto status = fileLoadedFuture.wait_for(std::chrono::seconds(10));
    if (status == std::future_status::ready)
    {
        curFilename = path;
        return true;
    }
    else
    {
        locker.unlock();
        return false;
    }
}

void MPVMediaInfo::reset()
{
    mpv::qt::command_variant(mpv, "stop");
    curFilename.clear();
    locker.unlock();
}

qint64 MPVMediaInfo::getDuration()
{
    return mpv::qt::get_property(mpv, "duration").toDouble() * 1000;  // ms
}

QPixmap MPVMediaInfo::getPreview(int time)
{
    previewLock.lockForRead();
    if (previewCache.contains(time))
    {
        QPixmap pixmap = previewCache[time];
        previewLock.unlock();
        return pixmap;
    }
    previewLock.unlock();

    previewPromise.reset(new std::promise<bool>);
    auto previewFuture = previewPromise->get_future();
    curPreviewTime = time;
    mpv::qt::command_variant(mpv, QVariantList() << "seek" << time << "absolute");
    auto status = previewFuture.wait_for(std::chrono::seconds(10));
    if (status == std::future_status::ready)
    {
        QReadLocker rL(&previewLock);
        if (previewCache.contains(time))
        {
            return previewCache[time];
        }
    }
    return QPixmap();
}

MPVMediaInfo::MPVMediaInfo(QObject *parent) : QObject{parent}
{
    mpv = mpv_create();
    if (!mpv)
        throw std::runtime_error("could not create mpv context");
    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");
    mpv::qt::set_property(mpv, "mute", true);
    mpv_set_option_string(mpv, "vo", "libmpv");

    pSurface = new QOffscreenSurface(nullptr, this);
    pSurface->setFormat(QSurfaceFormat::defaultFormat());
    pSurface->create();
    ctx = new QOpenGLContext(this);
    ctx->setFormat(pSurface->format());
    ctx->create();
    ctx->makeCurrent(pSurface);
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setSamples(0);
    this->previewSize = QSize(330, 186);
    QSize vSize(previewSize);
    vSize.rwidth()*=2;
    vSize.rheight()*=2;
    pFbo = new QOpenGLFramebufferObject(vSize, fboFormat);

    int enable_advanced_control = 0;
    mpv_opengl_init_params gl_init_params{get_proc_address, nullptr};
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &enable_advanced_control},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
        throw std::runtime_error("failed to initialize mpv GL context");

    mpv_set_wakeup_callback(mpv, wakeup, this);
    mpv_render_context_set_update_callback(mpv_gl, MPVMediaInfo::on_update, reinterpret_cast<void *>(this));

    infoThread = new QThread(this);
    infoThread->setObjectName("MPVInfo");
    infoThread->start();
    moveToThread(infoThread);
}

void MPVMediaInfo::update()
{
    bool ret = ctx->makeCurrent(pSurface);
    ret = pFbo->bind();
    mpv_opengl_fbo mpfbo{static_cast<int>(pFbo->handle()), pFbo->width(), pFbo->height(), 0};
    int flip_y{1};

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    mpv_render_context_render(mpv_gl, params);

    int ptime = mpv::qt::get_property(mpv, "playback-time").toInt();
    if(!posSet.contains(ptime))
    {
        posSet.insert(ptime);
        ctx->doneCurrent();
        return;
    }
    previewLock.lockForWrite();
    previewCache[ptime] = QPixmap::fromImage(pFbo->toImage().scaled(previewSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    previewLock.unlock();

    if (previewPromise && ptime == curPreviewTime)
    {
        previewPromise->set_value(true);
    }

    ctx->doneCurrent();
}

void MPVMediaInfo::on_mpv_events()
{
    while (mpv)
    {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;
        switch (event->event_id)
        {
        case MPV_EVENT_FILE_LOADED:
        {
            if (filePromise) filePromise->set_value(true);
            break;
        }
        default:
            break;
        }
    }
}

void MPVMediaInfo::on_update(void *ctx)
{
    QMetaObject::invokeMethod((MPVMediaInfo*)ctx, "update");
}

void MPVMediaInfo::wakeup(void *ctx)
{
    QMetaObject::invokeMethod((MPVMediaInfo*)ctx, "on_mpv_events", Qt::QueuedConnection);
}

#include "mpvpreview.h"

#include <QOffscreenSurface>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QPixmap>
#include <QDebug>
#include <stdexcept>
namespace
{
static void *get_proc_address(void *ctx, const char *name) {
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx)
        return nullptr;
    return reinterpret_cast<void *>(glctx->getProcAddress(QByteArray(name)));
}
}

MPVPreview::MPVPreview(const QSize &previewSize, int pInterval, QObject *parent):QObject (parent),previewInterval(pInterval)
{
    mpv = mpv_create();
    if (!mpv)
        throw std::runtime_error("could not create mpv context");
    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");
    mpv::qt::set_property(mpv, "mute", true);

    pSurface = new QOffscreenSurface(nullptr, this);
    pSurface->setFormat(QSurfaceFormat::defaultFormat());
    pSurface->create();
    ctx = new QOpenGLContext(this);
    ctx->setFormat(pSurface->format());
    ctx->create();
    ctx->makeCurrent(pSurface);
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setSamples(0);
    this->previewSize = previewSize;
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

    mpv_render_context_set_update_callback(mpv_gl, MPVPreview::on_update, reinterpret_cast<void *>(this));
}

MPVPreview::~MPVPreview()
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
}

QPixmap *MPVPreview::getPreview(int timePos, bool refresh)
{
    if(curFilename.isEmpty()) return nullptr;
    int pos = timePos / previewInterval;
    QPixmap *pixmap = nullptr;
    locker.lockForRead();
    if(previewCache.contains(pos))
    {
        pixmap = &previewCache[pos];
    }
    locker.unlock();
    if(!pixmap && refresh)
    {
        mpv::qt::command_variant(mpv, QVariantList() << "seek" << timePos << "absolute");
    }
    return pixmap;
}

void MPVPreview::reset(const QString &filename)
{
    if(filename.isEmpty())
    {
        mpv::qt::command_variant(mpv, "stop");
    }
    else if(filename != curFilename)
    {
        curFilename = filename;
        mpv::qt::command_variant(mpv, QStringList() << "loadfile" << curFilename);
        mpv::qt::set_property(mpv,"pause",true);
        locker.lockForWrite();
        previewCache.clear();
        locker.unlock();
        posSet.clear();
    }
}

void MPVPreview::on_update(void *ctx)
{
    QMetaObject::invokeMethod((MPVPreview*)ctx, "update");
}

void MPVPreview::update()
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
    int pos = ptime / previewInterval;
    if(!posSet.contains(ptime))
    {
        posSet.insert(ptime);
        ctx->doneCurrent();
        return;
    }
    locker.lockForWrite();
    previewCache[pos] = QPixmap::fromImage(pFbo->toImage().scaled(previewSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    locker.unlock();
    emit previewDown(ptime, &previewCache[pos]);

    ctx->doneCurrent();
}




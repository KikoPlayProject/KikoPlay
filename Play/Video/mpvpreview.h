#ifndef MPVPREVIEW_H
#define MPVPREVIEW_H

#include <QHash>
#include <QObject>
#include <QReadWriteLock>
#include <QSet>
#include <QSize>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <mpv/qthelper.hpp>

class QOpenGLFramebufferObject;
class QOffscreenSurface;
class QOpenGLContext;
class MPVPreview : public QObject
{
    Q_OBJECT
public:
    MPVPreview(const QSize &previewSize, int pInterval=3, QObject *parent = nullptr);
    ~MPVPreview();
    QPixmap *getPreview(int timePos, bool refresh=true);
    void reset(const QString &filename="");
signals:
    void previewDown(int time, QPixmap *pixmap);
private slots:
    void update();
private:
    static void on_update(void *ctx);
    static void wakeup(void *ctx);

    mpv_handle *mpv;
    mpv_render_context *mpv_gl;
    QOpenGLContext *ctx = nullptr;
    QOffscreenSurface *pSurface = nullptr;
    QOpenGLFramebufferObject *pFbo = nullptr;
    QSize previewSize;
    QSet<int> posSet;

    QString curFilename;
    int previewInterval;
    QReadWriteLock locker;
    QHash<int, QPixmap> previewCache;

};

#endif // MPVPREVIEW_H

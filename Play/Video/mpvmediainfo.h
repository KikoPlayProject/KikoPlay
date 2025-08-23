#ifndef MPVMEDIAINFO_H
#define MPVMEDIAINFO_H
#include <future>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <mpv/qthelper.hpp>
#include <QObject>
#include <QSize>
#include <QMutex>
#include <QReadWriteLock>

class QOpenGLFramebufferObject;
class QOffscreenSurface;
class QOpenGLContext;

class MPVMediaInfo : public QObject
{
    Q_OBJECT
public:
    explicit MPVMediaInfo(QObject *parent = nullptr);
    ~MPVMediaInfo();

    bool loadFile(const QString &path);
    void reset();

    qint64 getDuration();
    QPixmap getPreview(int time);


private slots:
    void update();
    void on_mpv_events();

private:
    static void on_update(void *ctx);
    static void wakeup(void *ctx);

    QThread *infoThread;

    mpv_handle *mpv;
    mpv_render_context *mpv_gl;
    QOpenGLContext *ctx = nullptr;
    QOffscreenSurface *pSurface = nullptr;
    QOpenGLFramebufferObject *pFbo = nullptr;

    QString curFilename;
    QSize previewSize;
    QSet<int> posSet;
    QHash<int, QPixmap> previewCache;
    QMutex locker;
    QReadWriteLock previewLock;

    QScopedPointer<std::promise<bool>> filePromise;

    int curPreviewTime;
    QScopedPointer<std::promise<bool>> previewPromise;


};

#endif // MPVMEDIAINFO_H

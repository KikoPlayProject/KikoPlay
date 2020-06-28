#ifndef MEDIAHANDLER_H
#define MEDIAHANDLER_H
#include "qhttpengine/filesystemhandler.h"
#include <QMimeDatabase>
class IODeviceCopier : public QObject
{
    Q_OBJECT

public:
    IODeviceCopier(QIODevice *srcDevice, QIODevice *destDevice, QObject *parent = 0);
    void setBufferSize(qint64 size);
    void setRange(qint64 from, qint64 to);

signals:
    void error(const QString &message);
    void finished();

public:

    void start();
    void stop();

private:
    QIODevice *const src;
    QIODevice *const dest;

    qint64 bufferSize;

    qint64 rangeFrom;
    qint64 rangeTo;

    void onReadyRead();
    void onReadChannelFinished();
    void nextBlock();
};

class MediaHandler : public QHttpEngine::FilesystemHandler
{
    Q_OBJECT
public:
    MediaHandler(const QHash<QString,QString> *mediaHash,QObject *parent = nullptr);
signals:
    void requestMedia(const QString &path);
private:
    const QHash<QString,QString> *mediaHashTable;
    QMimeDatabase database;
    void processFile(QHttpEngine::Socket *socket, const QString &absolutePath);
    QByteArray mimeType(const QString &absolutePath);
protected:
    virtual void process(QHttpEngine::Socket *socket, const QString &path);
};

#endif // MEDIAHANDLER_H

#include "mediahandler.h"
#include <QIODevice>
#include <QTimer>
#include <QFileInfo>
#include "qhttpengine/socket.h"
#include "qhttpengine/range.h"

IODeviceCopier::IODeviceCopier(QIODevice *srcDevice, QIODevice *destDevice, QObject *parent):
    QObject(parent), src(srcDevice), dest(destDevice), bufferSize(65536), rangeFrom(0), rangeTo(-1)
{
    connect(src, &QIODevice::destroyed, this, &IODeviceCopier::stop);
    connect(dest, &QIODevice::destroyed, this, &IODeviceCopier::stop);
}

void IODeviceCopier::setBufferSize(qint64 size)
{
    bufferSize = size;
}

void IODeviceCopier::setRange(qint64 from, qint64 to)
{
    rangeFrom = from;
    rangeTo = to;
}

void IODeviceCopier::start()
{
    if (!src->isOpen())
    {
        if (!src->open(QIODevice::ReadOnly))
        {
            emit error(tr("Unable to open source device for reading"));
            emit finished();
            return;
        }
    }

    if (!dest->isOpen())
    {
        if (dest->open(QIODevice::WriteOnly))
        {
            emit error(tr("Unable to open destination device for writing"));
            emit finished();
            return;
        }
    }

    if (rangeFrom > 0 && !src->isSequential())
    {
        if (!src->seek(rangeFrom))
        {
            emit error(tr("Unable to seek source device for specified range"));
            emit finished();
            return;
        }
    }

    connect(src, &QIODevice::readyRead, this, &IODeviceCopier::onReadyRead);
    connect(src, &QIODevice::readChannelFinished, this, &IODeviceCopier::onReadChannelFinished);

    if(!src->isSequential())
        connect(dest, &QIODevice::bytesWritten, this, &IODeviceCopier::nextBlock);

    QTimer::singleShot(0, this, src->isSequential()? &IODeviceCopier::onReadyRead:&IODeviceCopier::nextBlock);
}

void IODeviceCopier::stop()
{
    disconnect(src, &QIODevice::readyRead, this, &IODeviceCopier::onReadyRead);
    disconnect(src, &QIODevice::readChannelFinished, this, &IODeviceCopier::onReadChannelFinished);
    disconnect(dest, &QIODevice::bytesWritten, this, &IODeviceCopier::nextBlock);
    emit finished();
}

void IODeviceCopier::onReadyRead()
{
    if (dest->write(src->readAll()) == -1)
    {
        emit error(dest->errorString());
        src->close();
    }
}

void IODeviceCopier::onReadChannelFinished()
{
    if (src->bytesAvailable())
    {
        onReadyRead();
    }
    emit finished();
}

void IODeviceCopier::nextBlock()
{
    QByteArray data;
    data.resize(bufferSize);
    qint64 dataRead = src->read(data.data(), bufferSize);

    if (dataRead == -1)
    {
        emit error(src->errorString());
        emit finished();
        return;
    }

    if (rangeTo != -1 && src->pos() > rangeTo)
    {
        dataRead -= src->pos() - rangeTo - 1;
    }

    if (dest->write(data.constData(), dataRead) == -1)
    {
        emit error(dest->errorString());
        emit finished();
        return;
    }

    if (src->atEnd() || (rangeTo != -1 && src->pos() > rangeTo))
    {
        emit finished();
    }
}


MediaHandler::MediaHandler(const QHash<QString, QString> *mediaHash, QObject *parent) :
    FilesystemHandler(parent),mediaHashTable(mediaHash)
{

}

QByteArray MediaHandler::mimeType(const QString &absolutePath)
{
    return database.mimeTypeForFile(absolutePath).name().toUtf8();
}

void MediaHandler::process(QHttpEngine::Socket *socket, const QString &path)
{
    if(path.startsWith("media/"))
    {
        QString mediaId(path.mid(6).trimmed());
        QString mediaPath(mediaHashTable->value(mediaId,""));
        mediaPath.isEmpty()?socket->writeError(QHttpEngine::Socket::NotFound):processFile(socket,mediaPath);
    }
    else if(path.startsWith("sub/")) // eg. sub/ass/...
    {
        QStringList infoList(path.split('/',QString::SkipEmptyParts));
        if(infoList.count()<3)
        {
            socket->writeError(QHttpEngine::Socket::BadRequest);
            return;
        }
        QString mediaPath(mediaHashTable->value(infoList[2],""));
        QFileInfo fi(mediaPath);
        QString subPath=QString("%1/%2.%3").arg(fi.absolutePath(),fi.baseName(),infoList[1]);
        processFile(socket, subPath);
    }
    else
    {
        FilesystemHandler::process(socket,path);
    }
}

void MediaHandler::processFile(QHttpEngine::Socket *socket, const QString &absolutePath)
{
    QFile *file = new QFile(absolutePath);
    if (!file->open(QIODevice::ReadOnly))
    {
        delete file;
        socket->writeError(QHttpEngine::Socket::Forbidden);
        return;
    }

    IODeviceCopier *copier = new IODeviceCopier(file, socket);
    connect(copier, &IODeviceCopier::finished, copier, &IODeviceCopier::deleteLater);
    connect(copier, &IODeviceCopier::finished, file, &QFile::deleteLater);
    connect(copier, &IODeviceCopier::finished, [socket]() {
        socket->close();
    });

    connect(socket, &QHttpEngine::Socket::disconnected, copier, &IODeviceCopier::stop);

    qint64 fileSize = file->size();
    // Checking for partial content request
    QByteArray rangeHeader = socket->headers().value("Range");
    QHttpEngine::Range range;

    if (!rangeHeader.isEmpty() && rangeHeader.startsWith("bytes="))
    {
        // Skiping 'bytes=' - first 6 chars and spliting ranges by comma
        QList<QByteArray> rangeList = rangeHeader.mid(6).split(',');

        // Taking only first range, as multiple ranges require multipart
        // reply support
        range = QHttpEngine::Range(QString(rangeList.at(0)), fileSize);
    }

    // If range is valid, send partial content
    if (range.isValid())
    {
        socket->setStatusCode(QHttpEngine::Socket::PartialContent);
        socket->setHeader("Accept-Ranges", "bytes");
        socket->setHeader("Content-Length", QByteArray::number(range.length()));
        socket->setHeader("Content-Range", QByteArray("bytes ") + range.contentRange().toLatin1());
        copier->setRange(range.from(), range.to());
    }
    else
    {
        socket->setStatusCode(QHttpEngine::Socket::OK);
        // If range is invalid or if it is not a partial content request,
        // send full file
        socket->setHeader("Content-Length", QByteArray::number(fileSize));
        socket->setHeader("Accept-Ranges", "bytes");
    }

    // Set the mimetype and content length
    socket->setHeader("Content-Type", mimeType(absolutePath));
    socket->writeHeaders();

    copier->start();
    emit requestMedia(QString("[%1]Media%3: %2").arg(socket->peerAddress().toString(), absolutePath, range.isValid()?"(Range)":""));
}

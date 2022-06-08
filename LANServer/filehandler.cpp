#include "filehandler.h"


FileHandler::FileHandler(QObject *parent) : stefanfrings::HttpRequestHandler(parent), mediaHashTable(nullptr)
{

}

void FileHandler::service(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QByteArray path = request.getPath().mid(1);
    if(mediaHashTable && path.startsWith("media/"))
    {
        QString mediaId(path.mid(6).trimmed());
        if(mediaId.indexOf('.') > 0)
        {
            mediaId = mediaId.mid(0, mediaId.indexOf('.'));
        }
        QString mediaPath(mediaHashTable->value(mediaId,""));
        mediaPath.isEmpty()? response.setStatus(stefanfrings::HttpResponse::NotFound) :
                             processFile(request, response, mediaPath);
    }
    else if(mediaHashTable && path.startsWith("sub/")) // eg. sub/ass/...
    {
        QStringList infoList(QString(path).split('/', Qt::SkipEmptyParts));
        if(infoList.count()<3)
        {
            response.setStatus(stefanfrings::HttpResponse::BadRequest);
            return;
        }
        QString mediaPath(mediaHashTable->value(infoList[2],""));
        QFileInfo fi(mediaPath);
        QString subPath=QString("%1/%2.%3").arg(fi.absolutePath(),fi.baseName(),infoList[1]);
        processFile(request, response, subPath);;
    }
    else
    {
        if (path.contains("/.."))
        {
           response.setStatus(stefanfrings::HttpResponse::Forbidden);
           return;
        }
        if(path.trimmed().isEmpty()) path = "index.html";
        QString absolutePath = root.absoluteFilePath(path);
        QFileInfo(absolutePath).isDir() ? processDirectory(request, response, absolutePath) :
                                          processFile(request, response, absolutePath);
    }
}

void FileHandler::processDirectory(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response, const QString &path)
{
    Q_UNUSED(request)
    // Add entries for each of the files
    QString listing;
    for(QFileInfo info : QDir(path).entryInfoList(QDir::NoFilter, QDir::Name | QDir::DirsFirst | QDir::IgnoreCase))
    {
        listing.append(QString("<li><a href=\"%1%2\">%1%2</a></li>")
                .arg(info.fileName().toHtmlEscaped())
                .arg(info.isDir() ? "/" : ""));
    }

    static const QString ListTemplate =
            "<!DOCTYPE html>"
            "<html>"
              "<head>"
                "<meta charset=\"utf-8\">"
                "<title>%1</title>"
              "</head>"
              "<body>"
                "<h1>%1</h1>"
                "<p>Directory listing:</p>"
                "<ul>%2</ul>"
                "<hr>"
                "<p><em>KikoPlay</em></p>"
              "</body>"
            "</html>";

    // Build the response and convert the string to UTF-8
    QByteArray data = ListTemplate
            .arg("/" + path.toHtmlEscaped())
            .arg(listing)
            .toUtf8();

    // Set the headers and write the content
    response.setHeader("Content-Type", "text/html");
    response.write(data, true);
}

void FileHandler::processFile(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response, const QString &path)
{
    QFile file(path);
    if(!file.exists())
    {
        response.setStatus(stefanfrings::HttpResponse::NotFound);
        return;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        response.setStatus(stefanfrings::HttpResponse::InternalServerError);
        return;
    }
    qint64 fileSize = file.size();
    QByteArray rangeHeader = request.getHeader("Range");

    bool rangeValid = false;
    qint64 from = 0, to = fileSize - 1;

    if (!rangeHeader.isEmpty() && rangeHeader.startsWith("bytes="))
    {
        // Skiping 'bytes=' - first 6 chars and spliting ranges by comma
        QList<QByteArray> rangeList = rangeHeader.mid(6).split(',');
        // Taking only first range, as multiple ranges require multipart
        // reply support
        rangeValid = getRange(QString(rangeList.at(0)), fileSize, from, to);
        //emit message(QString("range request: %1-%2/%3, length=%4, %5").
        //             arg(from).arg(to).arg(fileSize).arg(to-from+1).arg(rangeValid));
        if(from >= fileSize)
        {
            response.setStatus(stefanfrings::HttpResponse::RequestedRangeNotSatisfiable);
            return;
        }
    }
    response.setHeader("Content-Type",  database.mimeTypeForFile(path).name().toUtf8());
    response.setHeader("Accept-Ranges", "bytes");
    if(rangeValid)
    {
        response.setStatus(stefanfrings::HttpResponse::PartialContent);
        QString rangeStr = QString("%1-%2/%3").arg(QString::number(from), QString::number(to), QString::number(fileSize));
        response.setHeader("Content-Range", QByteArray("bytes ") + rangeStr.toLatin1());
    }
    Logger::logger()->log(Logger::LANServer, QString("[%1]Media%3: %2").arg(request.getPeerAddress().toString(), path, rangeValid?"(Range)":""));

    file.seek(from);
    // Return the file content, do not store in cache
    while (!file.atEnd() && !file.error() && file.pos() <= to && response.isConnected())
    {
        qint64 blockSize = qMin<qint64>(65536, to - file.pos() + 1);
        response.write(file.read(blockSize), file.atEnd());
    }
}

bool FileHandler::getRange(const QString &range, qint64 fileSize, qint64 &from, qint64 &to)
{
    static QRegExp regExp("^(\\d*)-(\\d*)$");

    from = 0, to = fileSize - 1;
    if (regExp.indexIn(range.trimmed()) == -1) return false;

    QString fromStr = regExp.cap(1);
    QString toStr = regExp.cap(2);

    if (fromStr.isEmpty() && toStr.isEmpty()) return false;

    bool okFrom = true, okTo = true;
    if(!fromStr.isEmpty())  from = fromStr.toLongLong(&okFrom);
    if(!toStr.isEmpty())  to = toStr.toLongLong(&okTo);

    if(!okFrom || !okTo)  return false;

    if(fromStr.isEmpty())
    {
        from = - to + fileSize;
        to = fileSize - 1;
        if(from < 0) from = 0;
    }
    else if(toStr.isEmpty())
    {
        to = fileSize - 1;
    }
    if(from > to) return false;
    return true;
}

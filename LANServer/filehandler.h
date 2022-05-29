#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "httpserver/httprequesthandler.h"
#include <QString>
#include <QHash>
#include <QDir>
#include <QMimeDatabase>

class FileHandler : public stefanfrings::HttpRequestHandler
{
    Q_OBJECT
    Q_DISABLE_COPY(FileHandler)
public:
    FileHandler(QObject* parent = nullptr);
    void service(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void setRoot(const QString &path) {root.setPath(path);}
    void setMediaHash(const QHash<QString,QString> *mediaHash) {mediaHashTable = mediaHash;}
private:
    QDir root;
    QMimeDatabase database;
    const QHash<QString,QString>  *mediaHashTable;

    void processDirectory(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response, const QString &path);
    void processFile(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response, const QString &path);

    bool getRange(const QString &range, qint64 fileSize, qint64 &from, qint64 &to);
};

#endif // FILEHANDLER_H

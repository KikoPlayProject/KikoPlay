#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QHash>
#include <QJsonDocument>
#include "qhttpengine/socket.h"
#include "qhttpengine/server.h"
class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);
    ~HttpServer();
    bool isListening() const {return server->isListening();}
private:
    QHttpEngine::Server *server;
    QHash<QString,QString> mediaHash;
    void genLog(const QString &logInfo);
    QJsonDocument playlistDoc;
signals:
    void showLog(const QString &logInfo);
public slots:
    QString startServer(quint16 port);
    void stopServer();

    void api_Playlist(QHttpEngine::Socket *socket);
    void api_UpdateTime(QHttpEngine::Socket *socket);
    void api_Danmu(QHttpEngine::Socket *socket);
    void api_DanmuFull(QHttpEngine::Socket *socket);
    void api_UpdateDelay(QHttpEngine::Socket *socket);
    void api_UpdateTimeline(QHttpEngine::Socket *socket);
    void api_Subtitle(QHttpEngine::Socket *socket);
};

#endif // HTTPSERVER_H

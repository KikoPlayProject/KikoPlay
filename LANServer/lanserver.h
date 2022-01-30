#ifndef LANSERVER_H
#define LANSERVER_H

#include <QObject>
class HttpServer;
class LANServer : public QObject
{
    Q_OBJECT
public:
    explicit LANServer(QObject *parent = nullptr);
    ~LANServer();
    bool startServer(quint16 port);
    void stopServer();
    bool isStart() const;
private:
    HttpServer *server;
    QThread *httpThread;
};

#endif // LANSERVER_H

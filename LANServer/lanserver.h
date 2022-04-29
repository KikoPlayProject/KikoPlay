#ifndef LANSERVER_H
#define LANSERVER_H

#include <QObject>
class Router;
class QSettings;
namespace stefanfrings
{
class HttpListener;
}
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
    Router *router;
    stefanfrings::HttpListener *listener;
    QSettings* serverSettings;

    QThread *httpThread;
};

#endif // LANSERVER_H

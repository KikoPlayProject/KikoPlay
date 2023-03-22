#ifndef LANSERVER_H
#define LANSERVER_H

#include <QObject>
class Router;
class UPnP;
class DLNAMediaServer;
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
    void startDLNA();
    void stopDLNA();
    bool isStart() const;
    bool isDLNAStart() const;
    UPnP *getUPnP() const {return upnp;}
private:
    Router *router;
    UPnP *upnp;
    DLNAMediaServer *dlnaMediaServer;
    stefanfrings::HttpListener *listener;
    QSettings* serverSettings;

    QThread *httpThread;
};

#endif // LANSERVER_H

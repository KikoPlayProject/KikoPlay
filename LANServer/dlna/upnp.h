#ifndef UPNP_H
#define UPNP_H
#include <QObject>
#include <QVector>
#include <QMap>
#include <QHostAddress>
#include "LANServer/httpserver/httprequesthandler.h"

class QUdpSocket;
class UPnPDevice;

struct UPnPRequest
{
    enum Method
    {
        M_SEARCH, NOTIFY, UNKNOWN
    };
    UPnPRequest() = default;
    UPnPRequest(Method m, const QString &sUrl, const QString &sProtocol) : method(m), url(sUrl), protocol(sProtocol) {}

    void send(QUdpSocket *socket, const QHostAddress &remoteAddress, quint16 remotePort);

    Method method;
    QString url;
    QString protocol;
    QHostAddress fromAddress;
    quint16 fromPort;
    QMap<QByteArray, QByteArray> headers;
};

struct UPnPResponse
{
    UPnPResponse() = default;
    UPnPResponse(int status, const QString &phrase, const QString &protocolStr) : statusCode(status), reasonPhrase(phrase), protocol(protocolStr) {}

    void send(QUdpSocket *socket, const QHostAddress &remoteAddress, quint16 remotePort);

    int statusCode;
    QString reasonPhrase;
    QString protocol;
    QVector<QPair<QByteArray, QByteArray>> headers;
    QByteArray content;
};

class UPnP : public QObject
{
    Q_OBJECT
public:
    UPnP(QObject *parent = nullptr);
    ~UPnP();

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    bool isStart() const {return upnpStart;}
    void addDevice(UPnPDevice *device);
    void removeDevice(UPnPDevice *device = nullptr);
    void handleHttpRequest(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
private:
    bool upnpStart;
    QUdpSocket *socket;
    QVector<UPnPDevice *> devices;
    void handleMessage();
    bool parseRequest(const QByteArray &content,  UPnPRequest &request);

};

#endif // UPNP_H

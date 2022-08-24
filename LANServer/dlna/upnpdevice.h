#ifndef UPNPDEVICE_H
#define UPNPDEVICE_H
#include "upnp.h"
#include "upnpservice.h"
#include <QVector>
#include <QSharedPointer>
#include "LANServer/httpserver/httprequesthandler.h"

class UPnPDevice
{
public:
    UPnPDevice();
    UPnPDevice(const QByteArray &usn);
    virtual ~UPnPDevice() {}

    const QByteArray &getUUID() const {return uuid;}

    QByteArray &getLocation() {return location;}
    const QByteArray &getLocation() const {return location;}
    QByteArray &getServer() {return server;}
    const QByteArray &getServer() const {return server;}
    QByteArray &getFriendlyName() {return friendlyName;}
    const QByteArray &getFriendlyName() const {return friendlyName;}
    QByteArray &getModelDescription() {return modelDescription;}
    const QByteArray &getModelDescription() const {return modelDescription;}
    QMap<QString, QSharedPointer<UPnPService>> &getServicesMap() {return services;}

    enum AnnounceType
    {
        ALIVE, BYEBYE, UPDATE
    };
    virtual void announce(AnnounceType type, QUdpSocket *socket, const QHostAddress &addr, quint16 port);
    virtual void handleUPnPRequest(QSharedPointer<const UPnPRequest> request, QUdpSocket *socket);
    virtual void handleHttpRequest(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
protected:
    QByteArray uuid;
    QByteArray deviceType;
    QByteArray server;
    QByteArray friendlyName;
    QByteArray manufacturer;
    QByteArray manufacturerURL;
    QByteArray modelDescription;
    QByteArray modelName;
    QByteArray modelURL;
    QByteArray location;
    quint32 bootId, configId;
    QMap<QString, QSharedPointer<UPnPService>> services;
    QByteArray generateUUID();

    bool parseAction(UPnPService *service, const QByteArray &actionName, const QByteArray &body, UPnPAction &action);
    void sendStateVariables(UPnPService *service, UPnPEventSubscriber *subscriber);
    virtual QUrl getDescriptionURL(const QHostAddress &fromAddress);

    virtual void onGetDesctiption(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    virtual void onSubscriberRequest(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    virtual void onHttpPost(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    virtual void onAction(UPnPAction &, stefanfrings::HttpRequest& , stefanfrings::HttpResponse& ) {}


};

#endif // UPNPDEVICE_H

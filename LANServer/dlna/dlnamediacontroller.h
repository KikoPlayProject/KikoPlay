#ifndef DLNAMEDIACONTROLLER_H
#define DLNAMEDIACONTROLLER_H

#include "upnpdevice.h"
class QXmlStreamWriter;
#define AVTransportService "urn:schemas-upnp-org:service:AVTransport:1"

class DLNAMediaController
{
public:
    DLNAMediaController(QSharedPointer<UPnPDevice> d) : device(d) {}
    void setAVTransportURI(const QString &uri);
    void seek(int pos);
    void play();
private:
    QSharedPointer<UPnPDevice> device;
    const UPnPService *getService(const QString &service);
    void writeRequest(QByteArray &content, std::function<void(QXmlStreamWriter &)>func);
};

#endif // DLNAMEDIACONTROLLER_H

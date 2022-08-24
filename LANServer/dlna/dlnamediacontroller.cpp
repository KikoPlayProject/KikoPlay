#include "dlnamediacontroller.h"
#include "Common/network.h"
#include <QXmlStreamWriter>

void DLNAMediaController::setAVTransportURI(const QString &uri)
{
    if(!device) return;
    const UPnPService *s = getService(AVTransportService);
    if(!s) return;
    QByteArray content;
    writeRequest(content, [&](QXmlStreamWriter &writer){
        writer.writeStartElement("u:SetAVTransportURI");
        writer.writeNamespace(AVTransportService, "u");
        writer.writeTextElement("InstanceID", "0");
        writer.writeTextElement("CurrentURI", uri);
        writer.writeTextElement("CurrentURIMetaData", "");
        writer.writeEndElement();
    });
    QUrl url(device->getLocation());
    url.setPath(s->controlURL);
    Network::httpPost(url.toString(), content,
                      {"Content-Type", "text/xml; charset=\"utf-8\"",
                       "SOAPAction", "\"" AVTransportService "#SetAVTransportURI\""});
}

void DLNAMediaController::seek(int pos)
{
    if(!device) return;
    const UPnPService *s = getService(AVTransportService);
    if(!s) return;
    int lh = pos / 3600;
    int lmin = (pos % 3600) / 60;
    int ls = pos % 60;
    QString seekTime = QString("%0:%1:%2").arg(lh,2,10,QChar('0')).arg(lmin,2,10,QChar('0')).arg(ls,2,10,QChar('0'));
    QByteArray content;
    writeRequest(content, [&](QXmlStreamWriter &writer){
        writer.writeStartElement("u:Seek");
        writer.writeNamespace(AVTransportService, "u");
        writer.writeTextElement("InstanceID", "0");
        writer.writeTextElement("Unit", "ABS_TIME");
        writer.writeTextElement("Target", seekTime);
        writer.writeEndElement();
    });
    QUrl url(device->getLocation());
    url.setPath(s->controlURL);
    Network::httpPost(url.toString(), content,
                      {"Content-Type", "text/xml; charset=\"utf-8\"",
                       "SOAPAction", "\"" AVTransportService "#Seek\""});
}

void DLNAMediaController::play()
{
    if(!device) return;
    const UPnPService *s = getService(AVTransportService);
    if(!s) return;
    QByteArray content;
    writeRequest(content, [&](QXmlStreamWriter &writer){
        writer.writeStartElement("u:Play");
        writer.writeNamespace(AVTransportService, "u");
        writer.writeTextElement("InstanceID", "0");
        writer.writeTextElement("Speed", "1");
        writer.writeEndElement();
    });
    QUrl url(device->getLocation());
    url.setPath(s->controlURL);
    Network::httpPost(url.toString(), content,
                      {"Content-Type", "text/xml; charset=\"utf-8\"",
                       "SOAPAction", "\"" AVTransportService "#Play\""});
}

const UPnPService *DLNAMediaController::getService(const QString &service)
{
    for(const QSharedPointer<UPnPService> &s : device->getServicesMap())
    {
        if(s->serviceType == service)
        {
            return s.get();
        }
    }
    return nullptr;
}

void DLNAMediaController::writeRequest(QByteArray &content, std::function<void (QXmlStreamWriter &)> func)
{
    QXmlStreamWriter writer(&content);
    writer.writeStartDocument();
    writer.writeNamespace("http://schemas.xmlsoap.org/soap/envelope/", "s");
    writer.writeStartElement("http://schemas.xmlsoap.org/soap/envelope/", "Envelope");
    writer.writeAttribute("http://schemas.xmlsoap.org/soap/envelope/", "encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
    writer.writeStartElement("http://schemas.xmlsoap.org/soap/envelope/", "Body");
    func(writer);
    writer.writeEndElement();    //Body
    writer.writeEndElement();    //Envelope
    writer.writeEndDocument();
}

#include "upnpdevice.h"
#include <QDateTime>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>
#include <random>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include "globalobjects.h"
#include "Common/network.h"
#include "Common/logger.h"

UPnPDevice::UPnPDevice() : uuid(generateUUID()), bootId(0), configId(1)
{

}

UPnPDevice::UPnPDevice(const QByteArray &usn) : bootId(0), configId(1)
{
    int midPos =usn.startsWith("uuid:")? 5 : 0;
    uuid = usn.mid(midPos);
}

void UPnPDevice::announce(AnnounceType type, QUdpSocket *socket, const QHostAddress &addr, quint16 port)
{
    UPnPRequest request(UPnPRequest::NOTIFY, "*", "HTTP/1.1");
    request.headers["Host"] = "239.255.255.250:1900";
    QByteArray nts;
    switch (type)
    {
    case AnnounceType::ALIVE:
        ++bootId;
        nts = "ssdp:alive";
        request.headers["Cache-Control"] = "max-age=1800";
        request.headers["Server"] = server;
        request.headers["Location"] = this->getDescriptionURL(socket->localAddress()).toString().toUtf8();
        break;
    case AnnounceType::BYEBYE:
        nts = "ssdp:byebye";
        break;
    case AnnounceType::UPDATE:
        request.headers["NEXTBOOTID.UPNP.ORG"] = QByteArray::number(bootId + 1);
        nts = "ssdp:update";
        request.headers["Location"] = this->getDescriptionURL(socket->localAddress()).toString().toUtf8();
        break;
    }
    request.headers["BOOTID.UPNP.ORG"] = QByteArray::number(bootId);
    request.headers["CONFIGID.UPNP.ORG"] = QByteArray::number(configId);
    request.headers["NTS"] = nts;
    {
        request.headers["USN"] = "uuid:" + this->uuid + "::upnp:rootdevice";
        request.headers["NT"] = "upnp:rootdevice";
        request.send(socket, addr, port);
    }
    {
        request.headers["USN"] = "uuid:" + this->uuid;
        request.headers["NT"] = "uuid:" + this->uuid;
        request.send(socket, addr, port);
    }
    {
        request.headers["USN"] = "uuid:" + this->uuid + "::" + this->deviceType;
        request.headers["NT"] = this->deviceType;
        request.send(socket, addr, port);
    }
    {
        for(const QSharedPointer<UPnPService> &service : services)
        {
            request.headers["USN"] = "uuid:" + this->uuid + "::" + service->serviceType.toUtf8();
            request.headers["NT"] = service->serviceType.toUtf8();
            request.send(socket, addr, port);
        }
    }
}

void UPnPDevice::handleUPnPRequest(QSharedPointer<const UPnPRequest> request, QUdpSocket *socket)
{
    if(request->method != UPnPRequest::M_SEARCH) return;
    QByteArray st = request->headers.value("st");
    if(st.isEmpty()) return;
    int mx = qBound(0, request->headers.value("mx", "0").toInt(), 5);
    if(mx == 0) return;
    bool match = false;
    QByteArray usn, target;
    if(st == "ssdp:all" || st == "upnp:rootdevice")
    {
        usn = "uuid:" + this->uuid + "::upnp:rootdevice";
        target = "upnp:rootdevice";
        match = true;
    }
    else if(st == "uuid:" + this->uuid)
    {
        usn = "uuid:" + this->uuid;
        target = usn;
        match = true;
    }
    else if(st == this->deviceType)
    {
        usn = "uuid:" + this->uuid + "::" + this->deviceType;
        target = this->deviceType;
        match = true;
    }
    else
    {
        for(const QSharedPointer<UPnPService> &service : services)
        {
            if(st == service->serviceType)
            {
                usn = "uuid:" + this->uuid + "::" + service->serviceType.toUtf8();
                target = service->serviceType.toUtf8();
                match = true;
                break;
            }
        }
    }
    if(match)
    {
        QTimer::singleShot(mx * 1000, [=](){
            QUdpSocket tmpSocket;
            tmpSocket.connectToHost(request->fromAddress, 1900);
            UPnPResponse response{200, "OK", "HTTP/1.1"};
            response.headers.append({"Location", this->getDescriptionURL(tmpSocket.localAddress()).toString().toUtf8()});
            response.headers.append({"Cache-Control", QByteArray("max-age=1800")});
            response.headers.append({"Server", server});
            response.headers.append({"EXT", QByteArray("")});
            response.headers.append({"BOOTID.UPNP.ORG", QByteArray::number(bootId)});
            response.headers.append({"USN", usn});
            response.headers.append({"ST", st});
            response.headers.append({"Date", QDateTime::currentDateTime().toString(Qt::RFC2822Date).toUtf8()});
            response.send(socket, request->fromAddress, request->fromPort);
        });

    }
}

void UPnPDevice::handleHttpRequest(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QByteArray method = request.getMethod();
    if(method == "GET" || method == "HEAD")
    {
        QByteArray path = request.getPath();  // /upnp/<uuid>/xxx
        path = path.mid(4 + uuid.size() + 3);
        if(path == "description.xml")
        {
            onGetDesctiption(request, response);
        }
        else if(path == "icon.png")
        {
            QFile iconFile(":/res/images/kikoplay-4.png");
            if (!iconFile.open(QIODevice::ReadOnly))
            {
                response.setStatus(stefanfrings::HttpResponse::InternalServerError);
                return;
            }
            response.setHeader("Content-Type", "image/png");
            response.write(iconFile.readAll(), true);
        }
        else
        {
			int pos = path.indexOf('/');
			if (pos != -1)
			{
				QByteArray serviceName = path.mid(0, pos);
				if (services.contains(serviceName))
				{
					QByteArray SCDPXml;
					services[serviceName]->dumpSCDPXml(SCDPXml);
					response.setHeader("Content-Type", "text/xml; charset=\"utf-8\"");
					response.write(SCDPXml, true);
				}
			}
        }
    }
    else if(method == "SUBSCRIBE" || method == "UNSUBSCRIBE")
    {
        onSubscriberRequest(request, response);
    }
    else if(method == "POST")
    {
        onHttpPost(request, response);
    }
}

QByteArray UPnPDevice::generateUUID()
{
    QByteArray uuid = "";
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned short>u(0, 15);
    for (int i = 0; i < 32; i++)
    {
        char c = (char)u(e);
        uuid += (c < 10) ? ('0' + c) : ('a' + (c - 10));
        if (i == 7 || i == 11 || i == 15 || i == 19)
        {
            uuid += '-';
        }
    }
    return uuid;
}

QUrl UPnPDevice::getDescriptionURL(const QHostAddress &fromAddress)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(fromAddress.toString());
    url.setPort(GlobalObjects::appSetting->value("Server/Port", 8000).toUInt());
    url.setPath(QString("/upnp/%1/description.xml").arg(QString(uuid)));
    return url;
}

void UPnPDevice::onGetDesctiption(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(request)
    QByteArray desc;
    QXmlStreamWriter writer(&desc);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("root");
    writer.writeDefaultNamespace("urn:schemas-upnp-org:device-1-0");
    writer.writeNamespace("urn:schemas-dlna-org:device-1-0", "dlna");
    writer.writeAttribute("configId", QString::number(configId));

    writer.writeStartElement("specVersion");
    writer.writeTextElement("", "major", "1");
    writer.writeTextElement("", "minor", "1");
    writer.writeEndElement();

    writer.writeStartElement("device");
    writer.writeTextElement("", "deviceType", deviceType);
    writer.writeTextElement("", "friendlyName", friendlyName);
    writer.writeTextElement("", "manufacturer", manufacturer);
    writer.writeTextElement("", "manufacturerURL", manufacturerURL);
    writer.writeTextElement("", "modelDescription", modelDescription);
    writer.writeTextElement("", "modelName", modelName);
    writer.writeTextElement("", "modelURL", modelURL);
    writer.writeTextElement("", "UDN", "uuid:" + uuid);

    writer.writeStartElement("urn:schemas-dlna-org:device-1-0", "X_DLNADOC");
    writer.writeNamespace("urn:schemas-dlna-org:device-1-0", "dlna");
    writer.writeCharacters("DMS-1.50");
    writer.writeEndElement();

    writer.writeStartElement("iconList");
    writer.writeStartElement("icon");
    writer.writeTextElement("", "mimetype", "image/png");
    writer.writeTextElement("", "width", "128");
    writer.writeTextElement("", "height", "128");
    writer.writeTextElement("", "depth", "32");
    writer.writeTextElement("", "url", QString("/upnp/%1/icon.png").arg(QString(uuid)));
    writer.writeEndElement();
    writer.writeEndElement();   //iconList

    writer.writeStartElement("serviceList");
    for(const QSharedPointer<UPnPService> &service : services)
    {
        writer.writeStartElement("service");
        writer.writeTextElement("", "serviceType", service->serviceType);
        writer.writeTextElement("", "serviceId", service->serviceId);
        writer.writeTextElement("", "SCPDURL", QString("/upnp/%1/%2").arg(uuid, service->SCPDURL));
        writer.writeTextElement("", "controlURL", QString("/upnp/%1/%2").arg(uuid, service->controlURL));
        writer.writeTextElement("", "eventSubURL", QString("/upnp/%1/%2").arg(uuid, service->eventSubURL));
        writer.writeEndElement();
    }
    writer.writeEndElement();   // serviceList

    writer.writeEndElement();   //device
    writer.writeEndElement();   //root
    writer.writeEndDocument();

    response.setHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    response.write(desc, true);
    Logger::logger()->log(Logger::LANServer, QString("[DLNA][%1]GetDesctiption").arg(request.getPeerAddress().toString()));
}

void UPnPDevice::onSubscriberRequest(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    UPnPService *service = nullptr;
    QByteArray path = request.getPath();  // /upnp/<uuid>/xxx
    path = path.mid(4 + uuid.size() + 3);
    for(QSharedPointer<UPnPService> &s : services)
    {
        if(s->eventSubURL == path)
        {
            service = s.get();
            break;
        }
    }
    do
    {
        if(!service) break;
        QByteArray method = request.getMethod();
        QByteArray sid = request.getHeader("SID");
        QByteArray callBackUrl = request.getHeader("CALLBACK");
        QByteArray nt = request.getHeader("NT");
        if(method == "SUBSCRIBE")
        {
            if(sid.isEmpty())
            {
                if(nt.isEmpty() || nt != "upnp:event" || callBackUrl.isEmpty()) break;
                if(service->subscribers.size() > 10)
                {
                    service->cleanSubscribers();
                    if(service->subscribers.size() > 10)
                    {
                        response.setStatus(stefanfrings::HttpResponse::InternalServerError);
                        return;
                    }
                }
                if (callBackUrl[0] == '<')
                {
                    int end = callBackUrl.indexOf('>');
                    if(end == -1) break;
                    callBackUrl = callBackUrl.mid(1, end - 1);
                    if(callBackUrl.isEmpty()) break;
                }
                sid = "uuid:" + generateUUID();
                UPnPEventSubscriber newSubscriber;
                newSubscriber.sid = sid;
                newSubscriber.callBackUrl = callBackUrl;
                newSubscriber.timeout = QDateTime::currentSecsSinceEpoch() + 1800;
                response.setHeader("SERVER", this->server);
                response.setHeader("SID", sid);
                response.setHeader("TIMEOUT", "Second-1800");
                sendStateVariables(service, &newSubscriber);
                service->addSubscriber(newSubscriber);
            }
            else  // renewing a subscription
            {
                if(!nt.isEmpty() || !callBackUrl.isEmpty()) break;
                auto iter = std::find_if(service->subscribers.begin(), service->subscribers.end(), [&](const QSharedPointer<UPnPEventSubscriber> &subscriber){
                    return subscriber->sid == sid;
                });
                if(iter == service->subscribers.end()) break;
                if((*iter)->timeout < QDateTime::currentSecsSinceEpoch())
                {
                    service->removeSubscriber(sid);
                    break;
                }
                (*iter)->timeout = QDateTime::currentSecsSinceEpoch() + 1800;
                response.setHeader("SID", sid);
                response.setHeader("TIMEOUT", "Second-1800");
            }
        }
        else
        {
            if(sid.isEmpty() || !nt.isEmpty() || !callBackUrl.isEmpty()) break;
            service->removeSubscriber(sid);
        }
        Logger::logger()->log(Logger::LANServer, QString("[DLNA][%1]Subscribe, Method: %2").arg(request.getPeerAddress().toString(), method));
        return;
    }while(false);
    response.setStatus(stefanfrings::HttpResponse::BadRequest);
}

void UPnPDevice::onHttpPost(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    UPnPService *service = nullptr;
    QByteArray path = request.getPath();  // /upnp/<uuid>/xxx
    path = path.mid(4 + uuid.size() + 3);
    for(QSharedPointer<UPnPService> &s : services)
    {
        if(s->controlURL == path)
        {
            service = s.get();
            break;
        }
    }
    do
    {
        if(!service) break;
        QByteArray SOAPAction = request.getHeader("SOAPAction");
        if(SOAPAction.startsWith("\"")) SOAPAction = SOAPAction.mid(1);
        if(SOAPAction.endsWith("\"")) SOAPAction.chop(1);
        if(SOAPAction.isEmpty()) break;
        QList<QByteArray> actionComponents = SOAPAction.split('#');
        if(actionComponents.size() != 2) break;
        QByteArray actionName = actionComponents[1];
        QByteArray reqBody = request.getBody();
        QSharedPointer<UPnPAction> action(service->getAction(actionName));
        if(!action) break;
        if(!parseAction(service, actionName, reqBody, *action)) break;
        onAction(*action, request, response);
        return;
    }while(false);
    response.setStatus(stefanfrings::HttpResponse::BadRequest);
}

bool UPnPDevice::parseAction(UPnPService *service, const QByteArray &actionName, const QByteArray &body, UPnPAction &action)
{
    QXmlStreamReader reader(body);
    int state = 0;
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name = reader.name();
            if(state == 0)
            {
                if(name == "Envelope") state = 1;
            }
            else if(state == 1)
            {
                if(name == "Body") state = 2;
            }
            else if(state == 2)
            {
                if(name == actionName)
                {
                    auto namespaces = reader.namespaceDeclarations();
                    if(!namespaces.isEmpty() && namespaces[0].namespaceUri() == service->serviceType)
                    {
                        state = 3;
                    }
                }
            }
            else if(state == 3)
            {
                action.setArg(name.toString(), reader.readElementText());
            }
        }
        reader.readNext();
    }
    return state == 3 && !reader.hasError() && action.verifyArgs();
}

void UPnPDevice::sendStateVariables(UPnPService *service, UPnPEventSubscriber *subscriber)
{
    QNetworkAccessManager *m = Network::getManager();
    QNetworkRequest request;
    request.setUrl(QUrl(subscriber->callBackUrl));
    request.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    request.setRawHeader("NT", "upnp:event");
    request.setRawHeader("NTS", "upnp:propchange");
    request.setRawHeader("SID", subscriber->sid);
    request.setRawHeader("SEQ", QByteArray::number(subscriber->eventKey));
    QByteArray content;
    service->dumpStateVariables(content);
    m->sendCustomRequest(request, "NOTIFY", content);
    subscriber->eventKey++;
}



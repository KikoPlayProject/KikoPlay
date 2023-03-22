#include "upnp.h"
#include "upnpdevice.h"
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QSharedPointer>
#include "Common/network.h"
#include "Common/logger.h"

UPnP::UPnP(QObject *parent) : QObject(parent), upnpStart(false), socket(nullptr)
{
    socket = new QUdpSocket(this);
}

UPnP::~UPnP()
{
    removeDevice();
}

void UPnP::start()
{
    if(socket->state() != QUdpSocket::UnconnectedState) return;
    QObject::connect(socket, &QUdpSocket::readyRead, this, &UPnP::handleMessage);
    socket->bind(QHostAddress::AnyIPv4, 1900);
    QHostAddress groupAddress("239.255.255.250");
    for (QNetworkInterface interface:  QNetworkInterface::allInterfaces())
    {
        socket->leaveMulticastGroup(groupAddress, interface);
        socket->joinMulticastGroup(groupAddress, interface);
    }
    upnpStart = true;
    Logger::logger()->log(Logger::LANServer, "UPnP Start");
}

void UPnP::stop()
{
    QObject::disconnect(socket, &QUdpSocket::readyRead, this, &UPnP::handleMessage);
    socket->disconnectFromHost();
    upnpStart = false;
    Logger::logger()->log(Logger::LANServer, "UPnP Stop");
}

void UPnP::addDevice(UPnPDevice *device)
{
    if(devices.contains(device) || !upnpStart) return;
	devices.append(device);
    for(QHostAddress address : QNetworkInterface::allAddresses())
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            QUdpSocket socket;
            socket.bind(address);
            device->announce(UPnPDevice::AnnounceType::ALIVE, &socket, QHostAddress("239.255.255.250"), 1900);
        }
    }
}

void UPnP::removeDevice(UPnPDevice *device)
{
    QVector<UPnPDevice *> removeDevices;
    if(device)
    {
        if(!devices.contains(device)) return;
        devices.removeAll(device);
        removeDevices.append(device);
    }
    else
    {
        removeDevices = devices;
        devices.clear();
    }
    if(!upnpStart) return;
    for(QNetworkInterface interface : QNetworkInterface::allInterfaces())
    {
        if(interface.flags().testFlag(QNetworkInterface::IsUp))
        {
            for(QNetworkAddressEntry entry : interface.addressEntries())
            {
                if(entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    QUdpSocket socket;
                    for(UPnPDevice *d : removeDevices)
                    {
                        d->announce(UPnPDevice::AnnounceType::BYEBYE, &socket, entry.broadcast(), 1900);
                    }
                }
            }
        }
    }
}

void UPnP::handleHttpRequest(stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    if(!upnpStart || devices.isEmpty()) return;
    QByteArray method = request.getMethod();
    QByteArray path = request.getPath();  // /upnp/<uuid>/xxx
    do
    {
        if(!path.startsWith("/upnp/")) break;
        path = path.mid(6);
        if(path.size() < devices[0]->getUUID().size() + 1) break;
        QByteArray uuid = path.mid(0, devices[0]->getUUID().size());
        UPnPDevice *device = nullptr;
        for(UPnPDevice *d : devices)
        {
            if(d->getUUID() == uuid)
            {
                device = d;
                break;
            }
        }
        if(!device) break;
        device->handleHttpRequest(request, response);
        return;
    }while(false);
    response.setStatus(stefanfrings::HttpResponse::BadRequest);
}

void UPnP::handleMessage()
{
    while (socket->hasPendingDatagrams())
    {
        QByteArray replyBytes;
        replyBytes.resize(socket->pendingDatagramSize());
        auto request = QSharedPointer<UPnPRequest>::create();
        socket->readDatagram(replyBytes.data(), replyBytes.size(), &(*request).fromAddress, &(*request).fromPort);
        if(parseRequest(replyBytes, *request))
        {
            for(UPnPDevice *d : devices)
            {
                d->handleUPnPRequest(request, socket);
            }
        }
    }
}

bool UPnP::parseRequest(const QByteArray &content, UPnPRequest &request)
{
    int pos = 0, lastPos = 0;
    int state = 0;
    QByteArray currentHeader;
    static const QMap<QByteArray, UPnPRequest::Method> methodMap = {
        {"M-SEARCH", UPnPRequest::M_SEARCH}
    };
    while((pos = content.indexOf("\r\n", pos)) > 0)
    {
        QByteArray line(content.mid(lastPos, pos - lastPos));
        pos += 2;
        lastPos = pos;
        if(state == 0)
        {
            QList<QByteArray> list= line.split(' ');
            if(list.count() != 3)
            {
                state = -1;
                break;
            }
            auto iter = methodMap.find(list[0]);
            if(iter == methodMap.cend())
            {
                state = -1;
                break;
            }
            request.method = iter.value();
            request.url = list[1];
            request.protocol = list[2];
            state = 1;
        }
        else if(state == 1)
        {
            int colon = line.indexOf(':');
            if(colon > 0)
            {
                currentHeader = line.left(colon).toLower();
                QByteArray value= line.mid(colon + 1).trimmed();
                request.headers.insert(currentHeader, value);
            }
            else
            {
                if (!line.isEmpty())
                {
                    if(request.headers.contains(currentHeader))
                    {
                        request.headers.insert(currentHeader, request.headers.value(currentHeader) + " " + line);
                    }
                }
                else  // reply body
                {
                    state = 2;
                    break;
                }
            }
        }
    }
    return state == 2;
}

void UPnPRequest::send(QUdpSocket *socket, const QHostAddress &remoteAddress, quint16 remotePort)
{
    static QByteArray requestTypes[] = {"M-SEARCH", "NOTIFY", "UNKNOWN"};
    QByteArray buffer;
    buffer.append(requestTypes[method]);
    buffer.append(" ");
    buffer.append(url.toUtf8());
    buffer.append(" ");
    buffer.append(protocol.toUtf8());
    buffer.append("\r\n");

    for(auto iter = headers.begin(); iter != headers.end(); ++iter)
    {
        buffer.append(iter.key());
        buffer.append(": ");
        buffer.append(iter.value());
        buffer.append("\r\n");
    }
    buffer.append("\r\n");
    socket->writeDatagram(buffer.data(), remoteAddress, remotePort);
}

void UPnPResponse::send(QUdpSocket *socket, const QHostAddress &remoteAddress, quint16 remotePort)
{
    QByteArray buffer;
    buffer.append(protocol.toUtf8());
    buffer.append(" ");
    buffer.append(QByteArray::number(statusCode));
    buffer.append(" ");
    buffer.append(reasonPhrase.toUtf8());
    buffer.append("\r\n");

    for(const auto &header : headers)
    {
        buffer.append(header.first);
        buffer.append(": ");
        buffer.append(header.second);
        buffer.append("\r\n");
    }
    buffer.append("\r\n");
    socket->writeDatagram(buffer.data(), remoteAddress, remotePort);
}

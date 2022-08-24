#include "upnpctrlpoint.h"
#include <QUdpSocket>
#include <random>
#include <QXmlStreamReader>
#include "Common/network.h"
UPnPCtrlPoint::UPnPCtrlPoint(QObject *parent) : QAbstractItemModel(parent)
{
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned short>u(1025, 15000);
    socket = new QUdpSocket(this);
    while(true)
    {
        quint16 port = u(e);
        if(socket->bind(QHostAddress::AnyIPv4, port)) break;
    }
    QObject::connect(socket, &QUdpSocket::readyRead, this, &UPnPCtrlPoint::readMessage);
}

void UPnPCtrlPoint::search(const QByteArray &searchTarget)
{
    UPnPRequest request(UPnPRequest::M_SEARCH, "*", " HTTP/1.1");
    request.headers["MAN"] = "\"ssdp:discover\"";
    request.headers["MX"] = "1";
    request.headers["ST"] = searchTarget;
    request.headers["Host"] = "239.255.255.250:1900";
    request.headers["Connection"] = "close";
    request.headers["User-Agent"] = "UPnP/1.0 KikoPlay/0.9.1";
    /*
    QByteArray buffer;
    buffer.append("M-SEARCH * HTTP/1.1\r\n");
    QVector<QPair<QByteArray, QByteArray>> headers = {
        {"MAN", "\"ssdp:discover\""},
        {"MX", "1"},
        {"ST", searchTarget},
        {"Host", "239.255.255.250:1900"},
        {"Connection", "close"},
        {"User-Agent", "UPnP/1.0 KikoPlay/0.9.1"}
    };
    for(const auto &header : headers)
    {
        buffer.append(header.first);
        buffer.append(": ");
        buffer.append(header.second);
        buffer.append("\r\n");
    }
    buffer.append("\r\n");
    */
    beginResetModel();
    devices.clear();
    endResetModel();
    request.send(socket, QHostAddress("255.255.255.255"), 1900);
}

QSharedPointer<UPnPDevice> UPnPCtrlPoint::getDevice(const QModelIndex &index)
{
    if(!index.isValid()) return nullptr;
    return devices[index.row()];
}

void UPnPCtrlPoint::readMessage()
{
    while (socket->hasPendingDatagrams())
    {
        QByteArray reply;
        reply.resize(socket->pendingDatagramSize());
        QHostAddress fromAddress;
        quint16 fromPort;
        socket->readDatagram(reply.data(), reply.size(), &fromAddress, &fromPort);
        QMap<QByteArray, QByteArray> headers;
        if(readHeaders(reply, headers))
        {
            processSearchReply(headers);
        }
    }
}

bool UPnPCtrlPoint::readHeaders(const QByteArray &reply, QMap<QByteArray, QByteArray> &headers)
{
    int pos = 0, lastPos = 0;
    int state = 0;
    bool hasError = false;
    QByteArray currentHeader;
    while((pos = reply.indexOf("\r\n", pos)) > 0)
    {
        QByteArray line(reply.mid(lastPos, pos - lastPos));
        pos += 2;
        lastPos = pos;
        if(state == 0)
        {
            QList<QByteArray> list= line.split(' ');
            if(list.count() != 3 || list.at(0) != "HTTP/1.1" || !list.at(1).startsWith('2'))
            {
                hasError = true;
                break;
            }
            state = 1;
        }
        else if(state == 1)
        {
            int colon = line.indexOf(':');
            if(colon > 0)
            {
                currentHeader = line.left(colon).toLower();
                QByteArray value= line.mid(colon + 1).trimmed();
                headers.insert(currentHeader,value);
            }
            else if (!line.isEmpty())
            {
                if (headers.contains(currentHeader))
                {
                    headers.insert(currentHeader, headers.value(currentHeader) + " " + line);
                }
            }
        }
    }
    return !hasError;
}

void UPnPCtrlPoint::processSearchReply(const QMap<QByteArray, QByteArray> &headers)
{
    QString usn = headers["usn"];
    if(usn.isEmpty()) return;
    for(const auto &d : devices)
    {
        if("uuid:" + d->getUUID() == usn) return;
    }
    QSharedPointer<UPnPDevice> device = QSharedPointer<UPnPDevice>::create(usn.toUtf8());
    device->getLocation() = headers["location"];
    device->getServer() = headers["server"];
    if(getDescription(*device))
    {
        beginInsertRows(QModelIndex(), devices.size(), devices.size());
        devices.append(device);
        endInsertRows();
    }
}

QByteArray UPnPCtrlPoint::httpGet(const QString &surl)
{
    QNetworkAccessManager NAM;
    QNetworkRequest request;
    request.setUrl(QUrl(surl));
    QNetworkReply *reply = NAM.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();
    return reply->readAll();
}

bool UPnPCtrlPoint::getDescription(UPnPDevice &device)
{
    Network::Reply descReply = Network::httpGet(device.getLocation(), QUrlQuery());
    if(descReply.hasError) return false;
    QXmlStreamReader reader(descReply.content);
    bool serviceStart = false;
    int serviceIndex = 0;
    QString serviceKey;
    auto &services = device.getServicesMap();
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name=="friendlyName"){
                device.getFriendlyName() = reader.readElementText().trimmed().toUtf8();
            } else if(name == "modelDescription") {
                device.getModelDescription() = reader.readElementText().trimmed().toUtf8();
            } else if(name == "service") {
                serviceStart = true;
                serviceKey = QString::number(serviceIndex++);
				services[serviceKey] = QSharedPointer<UPnPService>::create();
            }
            if(serviceStart)
            {
                if(name == "serviceType") {
                    services[serviceKey]->serviceType = reader.readElementText().trimmed();
                } else if(name == "serviceId") {
                    services[serviceKey]->serviceId = reader.readElementText().trimmed();
                } else if(name == "SCPDURL") {
                    services[serviceKey]->SCPDURL = reader.readElementText().trimmed();
                    if(!services[serviceKey]->SCPDURL.startsWith('/')){
                        services[serviceKey]->SCPDURL.push_front('/');
                    }
                } else if(name == "controlURL") {
                    services[serviceKey]->controlURL = reader.readElementText().trimmed();
                    if(!services[serviceKey]->controlURL.startsWith('/')){
                        services[serviceKey]->controlURL.push_front('/');
                    }
                } else if(name == "eventSubURL") {
                    services[serviceKey]->eventSubURL = reader.readElementText().trimmed();
                    if(!services[serviceKey]->eventSubURL.startsWith('/')){
                        services[serviceKey]->eventSubURL.push_front('/');
                    }
                }
            }
        }
        if(reader.isEndElement())
        {
            if(reader.name() == "service")
            {
                serviceStart = false;
            }
        }
        reader.readNext();
    }
    return !reader.hasError();
}

QVariant UPnPCtrlPoint::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    Column col=static_cast<Column>(index.column());
    const UPnPDevice &device=*devices.at(index.row());
    if(role==Qt::DisplayRole || role == Qt::ToolTipRole)
    {
        switch (col)
        {
        case Column::FriendlyName:
            return device.getFriendlyName();
        case Column::Desc:
            return device.getModelDescription();
        case Column::Location:
        {
            QUrl location(device.getLocation());
            return QString("%1:%2").arg(location.host()).arg(location.port());
        }
        default:
            break;
        }
    }
    return QVariant();
}

QVariant UPnPCtrlPoint::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers{tr("Device"), tr("Description"), tr("Location")};
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if(section < headers.size()) return headers.at(section);
    }
    return QVariant();
}


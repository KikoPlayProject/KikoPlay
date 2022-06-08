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
        if(socket->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress)) break;
    }
    QObject::connect(socket, &QUdpSocket::readyRead, this, &UPnPCtrlPoint::readMessage);
}

void UPnPCtrlPoint::search(const QByteArray &searchTarget)
{
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
    beginResetModel();
    devices.clear();
    endResetModel();
    socket->writeDatagram(buffer.data(), QHostAddress("255.255.255.255"), 1900);
}

QSharedPointer<UPnPDevice> UPnPCtrlPoint::getDevice(const QModelIndex &index)
{
    if(!index.isValid()) return nullptr;
    return devices[index.row()];
}

void UPnPCtrlPoint::readMessage()
{
    while (socket->hasPendingDatagrams()) {
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
        if(d->usn == usn) return;
    }
    QSharedPointer<UPnPDevice> device = QSharedPointer<UPnPDevice>::create();
    device->location = headers["location"];
    device->server = headers["server"];
    device->st = headers["st"];
    device->usn = usn;
    getDescription(*device);
    beginInsertRows(QModelIndex(), devices.size(), devices.size());
    devices.append(device);
    endInsertRows();
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

void UPnPCtrlPoint::getDescription(UPnPDevice &device)
{
    QByteArray descReply = httpGet(device.location);
    QXmlStreamReader reader(descReply);
    bool serviceStart = false;
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name=="friendlyName"){
                device.friendlyName = reader.readElementText().trimmed();
            } else if(name == "modelDescription") {
                device.modelDescription = reader.readElementText().trimmed();
            } else if(name == "service") {
                serviceStart = true;
                device.services.append(UPnPService());
            }
            if(serviceStart)
            {
                if(name == "serviceType") {
                    device.services.back().serviceType = reader.readElementText().trimmed();
                } else if(name == "serviceId") {
                    device.services.back().serviceId = reader.readElementText().trimmed();
                } else if(name == "SCPDURL") {
                    device.services.back().SCPDURL = reader.readElementText().trimmed();
                    if(!device.services.back().SCPDURL.startsWith('/')){
                        device.services.back().SCPDURL.push_front('/');
                    }
                } else if(name == "controlURL") {
                    device.services.back().controlURL = reader.readElementText().trimmed();
                    if(!device.services.back().controlURL.startsWith('/')){
                        device.services.back().controlURL.push_front('/');
                    }
                } else if(name == "eventSubURL") {
                    device.services.back().eventSubURL = reader.readElementText().trimmed();
                    if(!device.services.back().eventSubURL.startsWith('/')){
                        device.services.back().eventSubURL.push_front('/');
                    }
                }
            }
        }
        if(reader.isEndElement())
        {
            if(reader.name() == "service") serviceStart = false;
        }
        reader.readNext();
    }
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
            return device.friendlyName;
        case Column::Desc:
            return device.modelDescription;
        case Column::Location:
        {
            QUrl location(device.location);
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
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size()) return headers.at(section);
    }
    return QVariant();
}

void DLNAMediaController::setAVTransportURI(const QString &uri)
{
    if(!device) return;
    const UPnPService *s = getService(AVTransportService);
    if(!s) return;
    QString fields = QString(
          "<InstanceID>0</InstanceID>"
          "<CurrentURI>%1</CurrentURI>"
          "<CurrentURIMetaData></CurrentURIMetaData>").arg(uri);
    QString body(fillRequestBody({
         {"action", "SetAVTransportURI"},
         {"urn", AVTransportService},
         {"fields", fields}
     }));
    QUrl url(device->location);
    url.setPath(s->controlURL);
    Network::httpPost(url.toString(), body.toUtf8(),
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
    qDebug() << "seek:" << seekTime;
    QString fields = QString(
          "<InstanceID>0</InstanceID>"
          "<Unit>ABS_TIME</Unit>"
          "<Target>%1</Target>").arg(seekTime);
    QString body(fillRequestBody({
         {"action", "Seek"},
         {"urn", AVTransportService},
         {"fields", fields}
     }));
    QUrl url(device->location);
    url.setPath(s->controlURL);
    Network::httpPost(url.toString(), body.toUtf8(),
                      {"Content-Type", "text/xml; charset=\"utf-8\"",
                       "SOAPAction", "\"" AVTransportService "#Seek\""});
}

void DLNAMediaController::play()
{
    if(!device) return;
    const UPnPService *s = getService(AVTransportService);
    if(!s) return;
    QString fields = QString(
          "<InstanceID>0</InstanceID>"
          "<Speed>1</Speed>");
    QString body(fillRequestBody({
         {"action", "Play"},
         {"urn", AVTransportService},
         {"fields", fields}
     }));
    QUrl url(device->location);
    url.setPath(s->controlURL);
    Network::httpPost(url.toString(), body.toUtf8(),
                      {"Content-Type", "text/xml; charset=\"utf-8\"",
                       "SOAPAction", "\"" AVTransportService "#Play\""});
}

QString DLNAMediaController::fillRequestBody(const QMap<QString, QString> &kv)
{
    const QString bodyTemplate =
        "<?xml version='1.0' encoding='utf-8'?>"
        "<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
          "<s:Body>"
            "<u:{action} xmlns:u=\"{urn}\">"
              "{fields}"
            "</u:{action}>"
          "</s:Body>"
        "</s:Envelope>";
    QString ret;
    QString curName;
    int state = 0;
    for(const QChar c: bodyTemplate)
    {
        if(state == 0)
        {
            if(c == '{')
            {
                state = 1;
                curName = "";
            }
            else ret.append(c);
        }
        else if(state == 1)
        {
            if(c == '}')
            {
                ret.append(kv.value(curName));
                state = 0;
            }
            else curName.append(c);
        }
    }
    return ret;
}

const UPnPService *DLNAMediaController::getService(const QString &service)
{
    for(const UPnPService &s : device->services)
    {
        if(s.serviceType == service)
        {
            return &s;
        }
    }
    return nullptr;
}

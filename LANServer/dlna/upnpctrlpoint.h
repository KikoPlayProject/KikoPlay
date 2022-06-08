#ifndef UPNPCTRLPOINT_H
#define UPNPCTRLPOINT_H

#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>
#define AVTransportService "urn:schemas-upnp-org:service:AVTransport:1"
class QUdpSocket;
struct UPnPService
{
    QString serviceType;
    QString serviceId;
    QString SCPDURL;
    QString controlURL;
    QString eventSubURL;
};
struct UPnPDevice
{
    QString location;
    QString server;
    QString st;
    QString usn;
    QString friendlyName;
    QString modelDescription;
    QVector<UPnPService> services;
};
class DLNAMediaController
{
public:
    DLNAMediaController(QSharedPointer<UPnPDevice> d) : device(d) {}
    void setAVTransportURI(const QString &uri);
    void seek(int pos);
    void play();
private:
    QSharedPointer<UPnPDevice> device;
    QString fillRequestBody(const QMap<QString ,QString> &kv);
    const UPnPService *getService(const QString &service);
};

class UPnPCtrlPoint : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit UPnPCtrlPoint(QObject *parent = nullptr);

    void search(const QByteArray &searchTarget);
    QSharedPointer<UPnPDevice> getDevice(const QModelIndex &index);
private:
    QVector<QSharedPointer<UPnPDevice>> devices;
    QUdpSocket *socket;

    void readMessage();
    bool readHeaders(const QByteArray &reply, QMap<QByteArray,QByteArray> &headers);
    void processSearchReply(const QMap<QByteArray, QByteArray> &headers);
    QByteArray httpGet(const QString &surl);
    void getDescription(UPnPDevice &device);
    enum class Column
    {
        FriendlyName, Desc, Location, NONE
    };

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const  {return parent.isValid()?0:devices.count();}
    virtual int columnCount(const QModelIndex &) const {return static_cast<int>(Column::NONE);}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // UPNPCTRLPOINT_H

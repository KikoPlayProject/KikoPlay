#ifndef UPNPSERVICE_H
#define UPNPSERVICE_H
#include <QString>
#include <QVector>
#include <QMap>
#include <QSharedPointer>
#include <QMutex>
struct UPnPService;
struct UPnPStateVariable
{
    QString name;
    QString dataType;
    QString defaultValue;
    QString value;
    struct  {
        int min;
        int max;
        int step;
    } range;
    QStringList allowedValues;
    bool hasRange = false;
    bool sendEvent = false;
};

struct UPnPArgDesc
{
    UPnPArgDesc(const UPnPStateVariable &stateVal) : relatedStateVariable(stateVal) {}
    enum Direction
    {
        IN, OUT
    };
    QString name;
    Direction d;
    const UPnPStateVariable &relatedStateVariable;
};

struct UPnPArg
{
    UPnPArg(const UPnPArgDesc &d) : desc(d) {}
    const UPnPArgDesc &desc;
    QString val;
};

struct UPnPActionDesc
{
    QString name;
    QVector<QSharedPointer<UPnPArgDesc>> argDescs;
    UPnPArgDesc &addArgDesc(const QString &name, const UPnPStateVariable &stateVal, UPnPArgDesc::Direction d);
};

struct UPnPAction
{
    UPnPAction(const UPnPService &s, const UPnPActionDesc &d) : service(s), desc(d), errCode(0) {}
    const UPnPService &service;
    const UPnPActionDesc &desc;
    int errCode;
    QString errDesc;
    QMap<QString, QSharedPointer<UPnPArg>> argMap;
    bool setArg(const QString &name, const QString &val);
    void fillStateVariableArg();
    bool verifyArgs();
    void writeResponse(QByteArray &buffer);
    void writeErrorResponse(QByteArray &buffer);
};

struct UPnPEventSubscriber
{
    QByteArray sid;
    QByteArray callBackUrl;
    qint64 timeout;
    unsigned int eventKey = 0;
};

struct UPnPService
{
    QString serviceType;
    QString serviceId;
    QString SCPDURL;
    QString controlURL;
    QString eventSubURL;
    QVector<QSharedPointer<UPnPActionDesc>> actionDescs;
    QVector<QSharedPointer<UPnPStateVariable>> stateVariables;
    QMap<QString, QSharedPointer<UPnPStateVariable>> stateVarMap;
    QVector<QSharedPointer<UPnPEventSubscriber>>subscribers;
    QMutex subscriberLock;

    UPnPStateVariable &addStateVariable(const QString &name, const QString &dataType);
    QSharedPointer<UPnPStateVariable> getStateVariable(const QString &name);
    UPnPActionDesc &addActionDesc(const QString &name);
    UPnPAction *getAction(const QString &name);
    void addSubscriber(const UPnPEventSubscriber &subscriber);
    void removeSubscriber(const QString &sid);
    void cleanSubscribers();
    void dumpSCDPXml(QByteArray &content);
    void dumpStateVariables(QByteArray &content);
};



#endif // UPNPSERVICE_H

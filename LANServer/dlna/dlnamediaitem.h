#ifndef DLNAMEDIAITEM_H
#define DLNAMEDIAITEM_H
#include <QString>
#include <QHostAddress>

class QXmlStreamWriter;
struct DLNAMediaItem
{
    enum ClassType
    {
        Container, Item
    };
    ClassType type;
    QString objID;
    QString parentID;
    QString title;
    int childSize = 0;
    qint64 fileSize = 0;

    QString protocolInfo;
    QString url;
    void toDidl(QXmlStreamWriter &writer, quint32 mask) const;
    void setResource(const QString &path, const QHostAddress &localAddress, quint16 port);

    static quint32 getMask(const QString &filter);
};

#endif // DLNAMEDIAITEM_H

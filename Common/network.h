#ifndef NETWORK_H
#define NETWORK_H
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QtCore>
namespace Network
{
    const int timeout=10000;
    QByteArray httpGet(const QString &url,QUrlQuery &query,const QStringList &header=QStringList());
    QByteArray httpPost(const QString &url,QByteArray &data,const QStringList &header=QStringList());
    QJsonDocument toJson(const QString &str);
    QJsonValue getValue(QJsonObject &obj, const QString &path);
    class NetworkError
    {
    public:
        NetworkError(QString info):errorInfo(info){}
        QString errorInfo;
    };
}

#endif // NETWORK_H

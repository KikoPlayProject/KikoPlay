#ifndef NETWORK_H
#define NETWORK_H
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QtCore>
namespace Network
{
    static constexpr const int timeout=10000;
    static constexpr const int maxRedirectTimes=10;
    struct Reply
    {
        int statusCode;
        bool hasError;
        QString errInfo;
        QByteArray content;
        QList<QPair<QByteArray,QByteArray>> headers;
    };

    Reply httpGet(const QString &url, const QUrlQuery &query, const QStringList &header=QStringList(), bool redirect=true);
    Reply httpPost(const QString &url, const QByteArray &data, const QStringList &header=QStringList());
    QList<Reply> httpGetBatch(const QStringList &urls, const QList<QUrlQuery> &querys, const QList<QStringList> &headers=QList<QStringList>(), bool redirect=true);
    QJsonDocument toJson(const QString &str);
    QJsonValue getValue(QJsonObject &obj, const QString &path);
    int decompress(const QByteArray &input, QByteArray &output);
    int gzipCompress(const QByteArray &input, QByteArray &output);
    int gzipDecompress(const QByteArray &input, QByteArray &output);
    class NetworkError
    {
    public:
        NetworkError(QString info):errorInfo(info){}
        QString errorInfo;
    };
}

#endif // NETWORK_H

#ifndef ARIA2JSONRPC_H
#define ARIA2JSONRPC_H
#include <QObject>
#include <QNetworkAccessManager>
class QProcess;
class RPCError
{
public:
    RPCError(const QString &info):errorInfo(info){}
    QString errorInfo;
};
class Aria2JsonRPC : public QObject
{
    Q_OBJECT
public:
    explicit Aria2JsonRPC(QObject *parent = nullptr);
    void exit();

private:
    QNetworkAccessManager manager;
    QProcess *aria2Process;
    QJsonObject rpcCall(const QString &method, const QJsonArray &params, const QString &id, bool async=true);
    void handleRPCReply(const QString &method, const QJsonObject &replyObj);
signals:
    void addNewTask(const QString &id, const QString &gid);
    void refreshStatus(const QJsonObject &statusObj);
    void refreshPeerStatus(const QJsonArray &peerArray);
    void refreshGlobalStatus(int downSpeed,int upSpeed,int numActive);
    void showLog(const QString &logInfo);
public slots:
    QString addUri(const QString &uri, const QJsonObject &options);
    QString addTorrent(const QString &base64Str, const QJsonObject &options);
    void tellStatus(const QString &gid);
    void getPeers(const QString &gid);
    void switchPauseStatus(const QString &gid, bool pause);
    void tellGlobalStatus();
    void removeTask(const QString &gid);
    void switchAllPauseStatus(bool pause);
    void changeOption(const QString &gid, const QJsonObject &options);
    void changeGlobalOption(const QJsonObject &options);

};

#endif // ARIA2JSONRPC_H

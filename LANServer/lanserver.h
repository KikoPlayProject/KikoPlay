#ifndef LANSERVER_H
#define LANSERVER_H

#include <QObject>
class HttpServer;
class LANServer : public QObject
{
    Q_OBJECT
public:
    explicit LANServer(QObject *parent = nullptr);
    QString startServer(qint64 port);
    void stopServer();
    const QStringList &getLog() const{return logs;}
    bool isStart() const;
private:
    HttpServer *server;
    QStringList logs;
signals:
    void showLog(const QString &log);
public slots:
};

#endif // LANSERVER_H

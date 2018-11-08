#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>

class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);

signals:

public slots:
};

#endif // HTTPSERVER_H
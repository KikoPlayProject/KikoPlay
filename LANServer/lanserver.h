#ifndef LANSERVER_H
#define LANSERVER_H

#include <QObject>

class LANServer : public QObject
{
    Q_OBJECT
public:
    explicit LANServer(QObject *parent = nullptr);

signals:

public slots:
};

#endif // LANSERVER_H
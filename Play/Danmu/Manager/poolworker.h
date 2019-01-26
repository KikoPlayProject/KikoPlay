#ifndef POOLWORKER_H
#define POOLWORKER_H

#include <QObject>

class PoolWorker : public QObject
{
    Q_OBJECT
public:
    explicit PoolWorker(QObject *parent = nullptr);

signals:

public slots:
};

#endif // POOLWORKER_H
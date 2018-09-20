#ifndef POOLMANAGER_H
#define POOLMANAGER_H

#include "framelessdialog.h"

class PoolManager : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit PoolManager(bool refreshPool, QWidget *parent = nullptr);

signals:

public slots:
};

#endif // POOLMANAGER_H

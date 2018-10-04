#ifndef POOLMANAGER_H
#define POOLMANAGER_H

#include "framelessdialog.h"

class PoolManager : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit PoolManager(QWidget *parent = nullptr);
};

#endif // POOLMANAGER_H

#ifndef POOLEDITOR_H
#define POOLEDITOR_H

#include <QDialog>
#include <QFrame>
#include "Play/Danmu/danmupool.h"
#include "framelessdialog.h"
class QVBoxLayout;
class QSpacerItem;
class PoolItem : public QFrame
{
    Q_OBJECT
public:
    explicit PoolItem(const DanmuSourceInfo *sourceInfo,QWidget *parent=nullptr);
};

class PoolEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit PoolEditor(QWidget *parent = nullptr);
private:
    QVBoxLayout *poolItemVLayout;
    void refreshItems();
protected:
    virtual void onClose();
};

#endif // POOLEDITOR_H

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
    explicit PoolItem(DanmuSourceInfo *sourceInfo,QWidget *parent=nullptr);
private:
    DanmuSourceInfo *source;
};

class PoolEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit PoolEditor(QWidget *parent = nullptr);
protected:
    virtual void onClose();
};

#endif // POOLEDITOR_H

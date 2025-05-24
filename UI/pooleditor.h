#ifndef POOLEDITOR_H
#define POOLEDITOR_H

#include <QDialog>
#include <QFrame>
#include "Play/Danmu/danmupool.h"
#include "framelessdialog.h"
class QVBoxLayout;
class QSpacerItem;
class QStackedLayout;

class PoolItem : public QWidget
{
    Q_OBJECT
public:
    explicit PoolItem(const DanmuSource *sourceInfo, QWidget *parent=nullptr);

    bool getSelectStatus() const { return isSelect; }
    void setSelectState(bool on);
    const DanmuSource *getSource() const { return src; }

private:
    bool isSelect{false};
    const DanmuSource *src;
    QLabel *pageSelectStatus;
    QString formatFixedDanmuCount(int count, const QFontMetrics& fm);
signals:
    void selectStateChanged(bool on);
    // QObject interface
public:
    bool eventFilter(QObject *watched, QEvent *event);
};

class PoolEditor : public CFramelessDialog
{
    Q_OBJECT
    friend class PoolItem;
public:
    explicit PoolEditor(QWidget *parent = nullptr);
    void refreshItems();
private:
    QStackedLayout *pageBtnSLayout;
    QVBoxLayout *poolItemVLayout;
    Pool *curPool;
    QVector<PoolItem *> poolItems;

};

#endif // POOLEDITOR_H

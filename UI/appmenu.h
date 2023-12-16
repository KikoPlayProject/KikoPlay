#ifndef APPMENU_H
#define APPMENU_H

#include <QMenu>
#include <QStyledItemDelegate>
#include "Common/notifier.h"
class DialogTip;
class AppMenu : public QMenu, public NotifyInterface
{
    Q_OBJECT
public:
    explicit AppMenu(QWidget *p, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event);

private:
    QWidget *popupFromWidget;
    DialogTip *dialogTip;

    // NotifyInterface interface
public:
    virtual void showMessage(const QString &content, int flag, const QVariant &);
};

class AppItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AppItemDelegate(int w, int h, QObject *parent = nullptr);
    // QAbstractItemDelegate interface
public:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
signals:
    void ItemClicked(const QModelIndex &index);
private:
    int itemWidth, itemHeight;
};
#endif // APPMENU_H

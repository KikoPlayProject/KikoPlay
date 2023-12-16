#ifndef APPPAGE_H
#define APPPAGE_H

#include "settingpage.h"
#include <QStyledItemDelegate>

class QTreeView;
class AppPage : public SettingPage
{
    Q_OBJECT
public:
    AppPage(QWidget *parent = nullptr);
    virtual void onAccept() override;
    virtual void onClose() override;
private:
    QTreeView *appView;
};

class AppPageItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AppPageItemDelegate(int rh, QObject *parent = nullptr) : QStyledItemDelegate(parent), rowHeight(rh) {}
    // QAbstractItemDelegate interface
public:
    virtual QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
    int rowHeight;
};

#endif // APPPAGE_H

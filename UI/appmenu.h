#ifndef APPMENU_H
#define APPMENU_H

#include <QMenu>
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

#endif // APPMENU_H

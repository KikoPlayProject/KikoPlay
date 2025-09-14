#include "appframelessdialog.h"
#include <QApplication>
#include <QPoint>
#include <QSize>
#include <QKeyEvent>
#include <QPushButton>
#include "UI/widgets/dialogtip.h"
#include "UI/widgets/loadingicon.h"

Q_TAKEOVER_NATIVEEVENT_CPP(AppFramelessDialog, elaAppBar);


AppFramelessDialog::AppFramelessDialog(const QString &titleStr, QWidget *parent)
    : QDialog(parent), isBusy(false), isPin(false)
{
    setObjectName(QStringLiteral("framelessDialog"));
    elaAppBar = new ElaAppBar(this, ElaAppBarControlType::AppDialog);
    setContentsMargins(6, elaAppBar->height(), 6, 6);
    setWindowTitle(titleStr);

    QObject::connect(elaAppBar, &ElaAppBar::closeButtonClicked, this, &AppFramelessDialog::reject);
    QObject::connect(this, &AppFramelessDialog::rejected, this, &AppFramelessDialog::onClose);
    QObject::connect(elaAppBar, &ElaAppBar::hideButtonClicked, this, &AppFramelessDialog::onHide);
    QObject::connect(elaAppBar, &ElaAppBar::pinButtonClicked, this, &AppFramelessDialog::onPin);

    dialogTip = new DialogTip(this);
    dialogTip->hide();

#ifndef Q_OS_WIN
    setAttribute(Qt::WA_Hover);
#endif
}

void AppFramelessDialog::onHide()
{
    if (onHideCallback)
    {
        if (!onHideCallback())
        {
            return;
        }
    }
    this->hide();
}

void AppFramelessDialog::onClose()
{
    if (onCloseCallback)
    {
        if (!onCloseCallback())
        {
            return;
        }
    }
}

void AppFramelessDialog::onPin()
{
    isPin = !isPin;
    elaAppBar->setOnTop(isPin);
    elaAppBar->dialogPinButton()->setText(isPin ? QChar(0xe863) : QChar(0xe864));
}

void AppFramelessDialog::setPin(bool pin)
{
    isPin = !pin;
    onPin();
}

void AppFramelessDialog::resizeEvent(QResizeEvent *)
{
    dialogTip->move((width()-dialogTip->width())/2, dialogTip->y());
}

void AppFramelessDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
        return;
    QDialog::keyPressEvent(event);
}


void AppFramelessDialog::showBusyState(bool busy)
{
    isBusy = busy;
    if(busy)
    {
        elaAppBar->dialogBusyIcon()->show();
        elaAppBar->dialogHideButton()->setEnabled(false);
        elaAppBar->dialogCloseButton()->setEnabled(false);
    }
    else
    {
        elaAppBar->dialogBusyIcon()->hide();
        elaAppBar->dialogHideButton()->setEnabled(true);
        elaAppBar->dialogCloseButton()->setEnabled(true);
    }
}

void AppFramelessDialog::setTitle(const QString &text)
{
    setWindowTitle(text);
}

void AppFramelessDialog::showMessage(const QString &msg, int type)
{
    dialogTip->showMessage(msg, type);
}

void AppFramelessDialog::setCloseCallback(const std::function<bool ()> &func)
{
    onCloseCallback = func;
}

void AppFramelessDialog::setHideCallback(const std::function<bool ()> &func)
{
    onHideCallback = func;
}





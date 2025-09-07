#include "framelessdialog.h"
#include <QApplication>
#include <QPoint>
#include <QSize>
#include "widgets/dialogtip.h"
#include "widgets/loadingicon.h"
#include "ela/ElaAppBar.h"

#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"

Q_TAKEOVER_NATIVEEVENT_CPP(CFramelessDialog, elaAppBar);

CFramelessDialog::CFramelessDialog(const QString &titleStr, QWidget *parent, bool showAccept, bool showClose, bool autoPauseVideo)
    : QDialog(parent), restorePlayState(false), isBusy(false)
{
    setObjectName(QStringLiteral("framelessDialog"));
    elaAppBar = new ElaAppBar(this, ElaAppBarControlType::Dialog);
    setContentsMargins(4, elaAppBar->height(), 4, 4);
    setWindowTitle(titleStr);

    elaAppBar->dialogCloseButton()->setVisible(showClose);
    elaAppBar->dialogAcceptButton()->setVisible(showAccept);

    QObject::connect(elaAppBar, &ElaAppBar::closeButtonClicked, this, &CFramelessDialog::onClose);
    QObject::connect(elaAppBar, &ElaAppBar::acceptButtonClicked, this, &CFramelessDialog::onAccept);

    if (autoPauseVideo && GlobalObjects::mpvplayer->needPause())
    {
        restorePlayState = true;
        GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
    }

    dialogTip = new DialogTip(this);
    dialogTip->hide();

#ifndef Q_OS_WIN
    setAttribute(Qt::WA_Hover);
#endif
}

void CFramelessDialog::setResizeable(bool resizeable)
{
    elaAppBar->setIsFixedSize(!resizeable);
}

void CFramelessDialog::onAccept()
{
    accept();
    if(restorePlayState)
        GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    if(!sizeSettingKey.isEmpty())
        GlobalObjects::appSetting->setValue(sizeSettingKey,size());
    for(auto &func : onCloseCallback)
        func();
}

void CFramelessDialog::onClose()
{
    reject();
    if(restorePlayState)
        GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    if(!sizeSettingKey.isEmpty())
        GlobalObjects::appSetting->setValue(sizeSettingKey,size());
    for(auto &func : onCloseCallback)
        func();
}

void CFramelessDialog::reject()
{
    QDialog::reject();
    if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
}

void CFramelessDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape && isBusy)
        return;
    QDialog::keyPressEvent(event);
}

void CFramelessDialog::resizeEvent(QResizeEvent *event)
{
    dialogTip->move((width()-dialogTip->width())/2, dialogTip->y());
}

void CFramelessDialog::showBusyState(bool busy)
{
    isBusy = busy;
    if(busy)
    {
        elaAppBar->dialogBusyIcon()->show();
        elaAppBar->dialogAcceptButton()->setEnabled(false);
        elaAppBar->dialogCloseButton()->setEnabled(false);
    }
    else
    {
        elaAppBar->dialogBusyIcon()->hide();
        elaAppBar->dialogAcceptButton()->setEnabled(true);
        elaAppBar->dialogCloseButton()->setEnabled(true);
    }
}

void CFramelessDialog::setTitle(const QString &text)
{
    setWindowTitle(text);
}

void CFramelessDialog::showMessage(const QString &msg, int type)
{
    dialogTip->showMessage(msg, type);
}

void CFramelessDialog::setSizeSettingKey(const QString &key, const QSize &initSize)
{
    sizeSettingKey = key;
    resize(GlobalObjects::appSetting->value(sizeSettingKey, initSize).toSize());
}

void CFramelessDialog::addOnCloseCallback(const std::function<void ()> &func)
{
    onCloseCallback.append(func);
}




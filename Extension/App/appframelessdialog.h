#ifndef APPFRAMELESSDIALOG_H
#define APPFRAMELESSDIALOG_H
#include "qsystemdetection.h"
#include <QDialog>
#include <QList>
#include <QMargins>
#include <QRect>
#include <QTimer>
#include "UI/ela/ElaAppBar.h"

class DialogTip;

class AppFramelessDialog : public QDialog
{
    Q_OBJECT
    Q_TAKEOVER_NATIVEEVENT_H

public:
    explicit AppFramelessDialog(const QString &titleStr, QWidget *parent = 0);

protected:
    void onHide();
    void onClose();
    void onPin();

public:
    void setPin(bool pin);
    bool isPinned() const { return isPin; }
private:
    ElaAppBar *elaAppBar;

    bool isBusy;
    bool isPin;

    DialogTip *dialogTip;
    std::function<bool()> onCloseCallback, onHideCallback;

    // QWidget interface
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;

public:
    void showBusyState(bool busy);
    void setTitle(const QString &text);
    void showMessage(const QString &msg, int type=1);
    void setCloseCallback(const std::function<bool()> &func);
    void setHideCallback(const std::function<bool()> &func);
};

#endif // APPFRAMELESSDIALOG_H

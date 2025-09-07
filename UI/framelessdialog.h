#ifndef CFRAMELESSDIALOG_H
#define CFRAMELESSDIALOG_H

#include <QDialog>
#include <QList>
#include <QMargins>
#include <QRect>
#include <QTimer>
#include "ela/ElaAppBar.h"

class DialogTip;

class CFramelessDialog : public QDialog
{
    Q_OBJECT
    Q_TAKEOVER_NATIVEEVENT_H

public:
    explicit CFramelessDialog(const QString &titleStr, QWidget *parent = nullptr, bool showAccept = false,
                              bool showClose = true, bool autoPauseVideo = true);
public:
    void setResizeable(bool resizeable=true);

protected:
    virtual void onAccept();
    virtual void onClose();
    virtual void reject() override;

private:
    ElaAppBar *elaAppBar;

    bool restorePlayState;
    bool isBusy;

    DialogTip *dialogTip;
    QString sizeSettingKey;
    QVector<std::function<void()>> onCloseCallback;

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

    void showBusyState(bool busy);
    void setTitle(const QString &text);
    void showMessage(const QString &msg, int type=1);
    void setSizeSettingKey(const QString &key, const QSize &initSize);
    void addOnCloseCallback(const std::function<void()> &func);

};

#endif // CFRAMELESSDIALOG_H

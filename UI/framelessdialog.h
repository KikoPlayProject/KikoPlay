#ifndef CFRAMELESSDIALOG_H
#define CFRAMELESSDIALOG_H

#include "qsystemdetection.h"
#include <QDialog>
#include <QList>
#include <QMargins>
#include <QRect>
#include <QTimer>
class QLabel;
class DialogTip;
class LoadingIcon;
#ifdef Q_OS_WIN
class CFramelessDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CFramelessDialog(const QString &titleStr="", QWidget *parent = 0,
                              bool showAccept=false, bool showClose=true, bool autoPauseVideo=true);
public:
    void setResizeable(bool resizeable=true);
    bool isResizeable(){return m_bResizeable;}

    void setResizeableAreaWidth(int width = 5);

protected:
    virtual void onAccept();
    virtual void onClose();
    void addIgnoreWidget(QWidget* widget);
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result);
public:
    void setContentsMargins(const QMargins &margins);
    void setContentsMargins(int left, int top, int right, int bottom);
    QMargins contentsMargins() const;
    QRect contentsRect() const;
    void getContentsMargins(int *left, int *top, int *right, int *bottom) const;

private:
    QList<QWidget*> m_whiteList;
    int m_borderWidth;

    QMargins m_margins;
    QMargins m_frames;
    bool m_bJustMaximized;

    bool m_bResizeable;
    bool inited;
    QPushButton *closeButton,*acceptButton;
    LoadingIcon *busyLabel;
    QLabel *title;
    QWidget *titleBar, *backWidget;
	bool restorePlayState;
    bool isBusy;

    DialogTip *dialogTip;
    QString sizeSettingKey;
    QVector<std::function<void()>> onCloseCallback;

    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;
    void showBusyState(bool busy);
    void setTitle(const QString &text);
    void showMessage(const QString &msg, int type=1);
    void setSizeSettingKey(const QString &key, const QSize &initSize);
    void addOnCloseCallback(const std::function<void()> &func);
    // QDialog interface
public slots:
    virtual void reject();
};
#else
class CFramelessDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CFramelessDialog(QString titleStr="", QWidget *parent = nullptr,
                              bool showAccept=false,bool showClose=true,bool autoPauseVideo=true);
public:
    void setResizeable(bool resizeable=true);
    bool isResizeable(){return m_bResizeable;}

protected:
    virtual void onAccept();
    virtual void onClose();
public:
    void setContentsMargins(const QMargins &margins);
    void setContentsMargins(int left, int top, int right, int bottom);
    QMargins contentsMargins() const;
    QRect contentsRect() const;
    void getContentsMargins(int *left, int *top, int *right, int *bottom) const;

private:
    int m_borderWidth;

    QMargins m_margins;
    QMargins m_frames;
    bool m_bJustMaximized;

    bool m_bResizeable;
    QPushButton *closeButton,*acceptButton;
    LoadingIcon *busyLabel;
    QLabel *title;
    QWidget *titleBar, *backWidget;
    bool restorePlayState;
    bool isBusy;
    bool isMousePressed;
    QPoint mousePressPos;

    bool resizeMouseDown;
    bool left, right, top, bottom;
    QPoint oldPos;

    DialogTip *dialogTip;
    QString sizeSettingKey;
    QVector<std::function<void()>> onCloseCallback;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;
    void showBusyState(bool busy);
    void setTitle(const QString &text);
    void showMessage(const QString &msg, int type=1);
    void setSizeSettingKey(const QString &key, const QSize &initSize);
    void addOnCloseCallback(const std::function<void()> &func);
    // QDialog interface
public slots:
    virtual void reject();
};
#endif
#endif // CFRAMELESSDIALOG_H

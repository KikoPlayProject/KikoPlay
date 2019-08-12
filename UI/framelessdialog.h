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
#ifdef Q_OS_WIN
class CFramelessDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CFramelessDialog(QString titleStr="", QWidget *parent = 0,
                              bool showAccept=false,bool showClose=true,bool autoPauseVideo=true);
public:
    void setResizeable(bool resizeable=true);
    bool isResizeable(){return m_bResizeable;}

    void setResizeableAreaWidth(int width = 5);

protected:
    virtual void onAccept();
    virtual void onClose();
    void addIgnoreWidget(QWidget* widget);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
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
    QLabel *busyLabel;
    QLabel *title;
    QWidget *titleBar;
	bool restorePlayState;

    DialogTip *dialogTip;

    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    void showBusyState(bool busy);
    void setTitle(const QString &text);
    void showMessage(const QString &msg, int type=0);
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
    QLabel *busyLabel;
    QLabel *title;
    QWidget *titleBar;
    bool restorePlayState;
    bool isMousePressed;
    QPoint mousePressPos;

    bool resizeMouseDown;
    bool left, right, bottom;
    QPoint oldPos;

    DialogTip *dialogTip;

    // QWidget interface
protected:
    bool eventFilter(QObject *obj, QEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    virtual void resizeEvent(QResizeEvent *event);
    void showBusyState(bool busy);
    void setTitle(const QString &text);
    void showMessage(const QString &msg, int type=0);
    // QDialog interface
public slots:
    virtual void reject();
};
#endif
#endif // CFRAMELESSDIALOG_H

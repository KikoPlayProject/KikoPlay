#ifndef CFRAMELESSWINDOW_H
#define CFRAMELESSWINDOW_H
#include "qsystemdetection.h"
#include <QObject>
#include <QMainWindow>

//A nice frameless window for both Windows and OS X
//Author: Bringer-of-Light
//Github: https://github.com/Bringer-of-Light/Qt-Nice-Frameless-Window
// Usage: use "CFramelessWindow" as base class instead of "QMainWindow", and enjoy
#ifdef Q_OS_WIN
#include <QWidget>
#include <QList>
#include <QMargins>
#include <QRect>
class CFramelessWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit CFramelessWindow(QWidget *parent = 0);
public:

    //设置是否可以通过鼠标调整窗口大小
    //if resizeable is set to false, then the window can not be resized by mouse
    //but still can be resized programtically
    void setResizeable(bool resizeable=true);
    bool isResizeable(){return m_bResizeable;}

    //设置可调整大小区域的宽度，在此区域内，可以使用鼠标调整窗口大小
    //set border width, inside this aera, window can be resized by mouse
    void setResizeableAreaWidth(int width = 5);
    void setOnTop(bool on);
protected:
    //设置一个标题栏widget，此widget会被当做标题栏对待
    //set a widget which will be treat as SYSTEM titlebar
    void setTitleBar(QWidget* titlebar);

    //在标题栏控件内，也可以有子控件如标签控件“label1”，此label1遮盖了标题栏，导致不能通过label1拖动窗口
    //要解决此问题，使用addIgnoreWidget(label1)
    //generally, we can add widget say "label1" on titlebar, and it will cover the titlebar under it
    //as a result, we can not drag and move the MainWindow with this "label1" again
    //we can fix this by add "label1" to a ignorelist, just call addIgnoreWidget(label1)
    void addIgnoreWidget(QWidget* widget);

    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
private slots:
    void onTitleBarDestroyed();
public:
    void setContentsMargins(const QMargins &margins);
    void setContentsMargins(int left, int top, int right, int bottom);
    QMargins contentsMargins() const;
    QRect contentsRect() const;
    void getContentsMargins(int *left, int *top, int *right, int *bottom) const;
public slots:
    void showFullScreen();
private:
    QWidget* m_titlebar;
    QList<QWidget*> m_whiteList;
    int m_borderWidth;

    QMargins m_margins;
    QMargins m_frames;
    bool m_bJustMaximized;

    bool m_bResizeable;
};

#else
class CFramelessWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit CFramelessWindow(QWidget *parent = nullptr):QMainWindow(parent){}
protected:
    void setTitleBar(QWidget* ){}
public slots:
    void setOnTop(bool on);
};
#endif

#endif // CFRAMELESSWINDOW_H

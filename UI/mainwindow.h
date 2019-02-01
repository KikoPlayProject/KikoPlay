#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "framelesswindow.h"
#include "player.h"
#include "list.h"
#include "librarywindow.h"
#include "downloadwindow.h"
#include <QToolButton>
#include <QHBoxLayout>
#include <QStackedLayout>
class QSplitter;
class MainWindow : public CFramelessWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    QWidget *widgetTitlebar;
    QToolButton *buttonIcon,*buttonPage_Play,*buttonPage_Library,*buttonPage_Downlaod;
    QToolButton *minButton,*maxButton,*closeButton;
    QSplitter *playSplitter;
    PlayerWindow *playerWindow;
    ListWindow *listWindow;
    LibraryWindow *library;
    DownloadWindow *download;
    int listWindowWidth;
    QStackedLayout *contentStackLayout;
    QRect originalGeo;
    bool listShowState;
    void setupUI();
    void switchToPlay(const QString &fileToPlay);

    QWidget *setupPlayPage();
    QWidget *setupLibraryPage();
    QWidget *setupDownloadPage();

protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void changeEvent(QEvent *);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void moveEvent(QMoveEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // MAINWINDOW_H

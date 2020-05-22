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
class DropableWidget : public QWidget
{
    Q_OBJECT
public:
    using QWidget::QWidget;
signals:
    void fileDrop(const QString &url);
    // QWidget interface
protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void paintEvent(QPaintEvent *event);
};
class MainWindow : public CFramelessWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    DropableWidget *widgetTitlebar;
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
    bool hasBackground;
    QImage background;
    void setupUI();
    void switchToPlay(const QString &fileToPlay);
    void setBackground(const QString &imagePath, bool forceRefreshQSS=false);

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

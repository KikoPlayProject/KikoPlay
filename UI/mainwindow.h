#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "framelesswindow.h"
#include "player.h"
#include "list.h"
#include "librarywindow.h"
#include "downloadwindow.h"
#include "Common/notifier.h"
#include <QToolButton>
#include <QHBoxLayout>
#include <QStackedLayout>
class QSplitter;
class BackgroundWidget;
class LogWindow;
class ScriptPlayground;

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
#ifdef Q_OS_WIN
class QWinTaskbarProgress;
class QWinTaskbarButton;
#endif
class QSystemTrayIcon;
class QProgressBar;
class MainWindow : public CFramelessWindow, public NotifyInterface
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    enum class HideToTrayType
    {
        NONE, MINIMIZE, CLOSE, UNKNOWN
    };

    void setBackground(const QString &path, const QColor &color);
    void setBgDarkness(int val);
    void setThemeColor(const QColor &color);
    void setHideToTrayType(HideToTrayType type);
    HideToTrayType getHideToTrayType() const {return hideToTrayType;}

    ListWindow *getList() const {return listWindow;}

private:
    DropableWidget *widgetTitlebar;
    BackgroundWidget *bgWidget;
    bool hasBackground;
    bool hasCoverBg;
    int curDarkness;
    QImage bgImg;
    QPixmap coverPixmap;
    int curPage;
    QColor themeColor;

#ifdef Q_OS_WIN
     QWinTaskbarButton *winTaskbarButton = nullptr;
     QWinTaskbarProgress *winTaskbarProgress = nullptr;
#endif

    QToolButton *buttonIcon,*buttonPage_Play,*buttonPage_Library,*buttonPage_Downlaod;
    QToolButton *minButton,*maxButton,*closeButton;
    QSplitter *playSplitter;
    PlayerWindow *playerWindow;
    ListWindow *listWindow;
    LibraryWindow *library;
    DownloadWindow *download;
    LogWindow *logWindow;
    ScriptPlayground *scriptPlayground;
    int listWindowWidth;
    QStackedLayout *contentStackLayout;
    QRect originalGeo, miniGeo;
    bool listShowState;
    bool isMini;
    QPoint miniPressPos;
    QSystemTrayIcon *trayIcon;
    HideToTrayType hideToTrayType;
    QProgressBar *downloadToolProgress;

    void setupUI();
    void switchToPlay(const QString &fileToPlay);
    void setBackground(const QString &imagePath, bool forceRefreshQSS=false, bool showAnimation = true);

    QWidget *setupPlayPage();
    QWidget *setupLibraryPage();
    QWidget *setupDownloadPage();
public:
    QVariant showDialog(const QVariant &inputs);

protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void changeEvent(QEvent *);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void moveEvent(QMoveEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void showEvent(QShowEvent *);
#ifndef Q_OS_WIN
    virtual bool eventFilter(QObject *object, QEvent *event);
#endif /* Q_OS_WIN */
};

#endif // MAINWINDOW_H

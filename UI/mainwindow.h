#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Common/notifier.h"
#include <QToolButton>
#include <QHBoxLayout>
#include <QStackedLayout>
#include "UI/widgets/backgroundwidget.h"
#include "ela/ElaAppBar.h"

class PlayerWindow;
class ListWindow;
class LibraryWindow;
class DownloadWindow;
class QSplitter;
class BackgroundWidget;
class LogWindow;
class ScriptPlayground;

#ifdef Q_OS_WIN
class QWinTaskbarProgress;
class QWinTaskbarButton;
#endif
class QSystemTrayIcon;
class QProgressBar;
class AppBar;
class WindowTip;
class MainWindow : public BackgroundMainWindow, public NotifyInterface
{
    Q_OBJECT
    Q_TAKEOVER_NATIVEEVENT_H
public:
    MainWindow(QWidget *parent = nullptr);

    enum class HideToTrayType
    {
        NONE, MINIMIZE, CLOSE, UNKNOWN
    };

    void setBackground(const QString &imagePath, bool showAnimation = true, bool isStartup = false);
    void setHideToTrayType(HideToTrayType type);
    HideToTrayType getHideToTrayType() const {return hideToTrayType;}

    ListWindow *getList() const {return listWindow;}

    double getDefaultBlur() const { return defaultBlurRadius; }
    void setDefaultBlur(double val);

private:
    bool hasBackground;
    bool hasCoverBg;
    QImage bgImg;
    QPixmap coverPixmap;
    int curPage;
    double defaultBlurRadius{0};

#ifdef Q_OS_WIN
     QWinTaskbarButton *winTaskbarButton = nullptr;
     QWinTaskbarProgress *winTaskbarProgress = nullptr;
#endif

    QToolButton *buttonPage_Play, *buttonPage_Library, *buttonPage_Downlaod;
    QToolButton *appButton, *minButton, *maxButton, *closeButton;
    QSplitter *playSplitter{nullptr};
    PlayerWindow *playerWindow{nullptr};
    ListWindow *listWindow{nullptr};
    LibraryWindow *library{nullptr};
    DownloadWindow *download{nullptr};
    LogWindow *logWindow{nullptr};
    ScriptPlayground *scriptPlayground{nullptr};
    int listWindowWidth;
    QStackedLayout *contentStackLayout;
    QRect originalGeo, miniGeo;
    bool listShowState;
    bool isMini;
    QPoint miniPressPos;
    QSystemTrayIcon *trayIcon;
    HideToTrayType hideToTrayType;
    QProgressBar *downloadToolProgress;
    AppBar *appBar;
    WindowTip *windowTip;

    ElaAppBar *elaAppBar;

    void initUI();
    void initIconAction();
    void initTray();
    void initSignals();
    void restoreSize();
    void onClose(bool forceExit = false);

    void switchToPlay(const QString &fileToPlay);


    QWidget *setupPlayPage();
    QWidget *setupLibraryPage();
    QWidget *setupDownloadPage();
public:
    QVariant showDialog(const QVariant &inputs) override;
    void showMessage(const QString &content, int, const QVariant &extra=QVariant()) override;

protected:
    virtual void closeEvent(QCloseEvent *event) override;
    virtual void changeEvent(QEvent *) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    virtual void moveEvent(QMoveEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void showEvent(QShowEvent *) override;
#ifndef Q_OS_WIN
    virtual bool eventFilter(QObject *object, QEvent *event);
#endif /* Q_OS_WIN */
};
#endif // MAINWINDOW_H

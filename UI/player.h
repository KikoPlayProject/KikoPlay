#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include "Play/Danmu/common.h"
#include "Common/notifier.h"
class QAction;
class QFontComboBox;
class QComboBox;
class QActionGroup;
class CFramelessDialog;
class QSpinBox;
class QSlider;
class ClickSlider;
class DanmuLaunch;
class QListView;
class OptionMenuPanel;
class QLineEdit;
class ElidedLabel;
class RecentlyPlayedItem;

class RecentItem : public QWidget
{
    friend class PlayerContent;
public:
    explicit RecentItem(QWidget *parent = nullptr);
    void setData(const RecentlyPlayedItem &item);
    QSize sizeHint() const;
private:
    QLabel *titleLabel{nullptr}, *coverLabel{nullptr}, *timeLabel{nullptr};
    QPushButton *deleteItem;
    QString path;
    static const QPixmap &nullCover();
protected:
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void enterEvent(QEnterEvent *event);
    virtual void leaveEvent(QEvent *event);
};

class PlayerContent : public QWidget
{
    Q_OBJECT
public:
    explicit PlayerContent(QWidget *parent = nullptr);
    void refreshItems();
    bool eventFilter(QObject *watched, QEvent *event);
private:
    QVector<RecentItem *> items;
    QLabel *logo{nullptr}, *recentTip{nullptr};
    QWidget *recentItemsContainer{nullptr};

    void openFile();
    void openUrl();
};

class PlayerWindow : public QWidget, public NotifyInterface
{
    Q_OBJECT
public:
    explicit PlayerWindow(QWidget *parent = nullptr);
    void toggleListCollapseState(int listType);
    void toggleFullScreenState(bool on);
    virtual void showMessage(const QString &msg, int flag = 0, const QVariant &extra = QVariant()) override;
    void showMessage(const QString &msg, const QString &type, int timeout = -1);

private:
    DanmuLaunch *launchWindow;

    //----------------------------------------
    QWidget *playerContent{nullptr};  // logo, recent list
    QObject *playInfo;                // message tip

    // top, bottom, side panels
    QWidget *playControlPanel{nullptr};
    QWidget *playInfoPanel{nullptr};
    QWidget *sidePanel{nullptr};
    QPushButton *playlistBtn{nullptr}, *danmuBtn{nullptr}, *subBtn{nullptr};
    bool autoHideControlPanel{true};

    // preview
    QWidget *progressInfo{nullptr};
    QLabel *timeInfoTip{nullptr};
    QLabel *previewLabel;
    QTimer previewTimer;

    // player widgets
    QListView *liveDanmuList{nullptr};
    ClickSlider *progress{nullptr};
    ClickSlider *miniProgress{nullptr};
    QWidget *danmuStatisBar{nullptr};
    QSlider *volumeSlider{nullptr};
    QLabel *timeLabel{nullptr};
    QLineEdit *launchDanmuEdit{nullptr};
    ElidedLabel *titleLabel{nullptr};
    QPushButton *playPause, *prev, *next, *stop, *mute, *setting, *danmu, *fullscreen, *launch;
    QMenu *contexMenu{nullptr};

    // option menus
    QWidget *danmuSettingPage{nullptr};
    QWidget *playSettingPage{nullptr};
    QWidget *volumeSettingPage{nullptr};
    QWidget *playInfoPage{nullptr};

    // settings
    int resizePercent{-1};
    int jumpForwardTime{5}, jumpBackwardTime{5};
    int liveVRange{40};

    // status
    bool onTopWhilePlaying{false};
    bool isFullscreen{false};
    bool progressPressed{false};
    QSharedPointer<DanmuComment> currentDanmu;
    QString totalTimeStr;
    const int hideCursorTimeout=3000;
    int ctrlPressCount{0}, altPressCount{0};
    QTimer doublePressTimer, hideCursorTimer, volumeHideTimer;
    QVector<int> playerWindowSize{50, 75, 100, 150, 200, -1};
    QStringList playerWindowSizeTip{"50%", "75%", "100%", "150%", "200%", tr("Fix")};
    bool miniModeOn{false}, mouseLPressed{false}, moving{false};
    QPoint pressPos;

    // actions
    QAction *actPlayPause{nullptr}, *actStop{nullptr}, *actPrev{nullptr}, *actNext{nullptr}, *actFullscreen{nullptr};
    QAction *actScreenshotSrc{nullptr}, *actScreenshotAct{nullptr}, *actSnippet{nullptr}, *actGIF{nullptr};
    QAction *actMiniMode{nullptr};
    QAction *ctxText{nullptr}, *ctxCopy{nullptr}, *ctxBlockUser{nullptr}, *ctxBlockText{nullptr}, *ctxBlockColor{nullptr};


private:
    QWidget *initPlayerLayer(QWidget *parent);
    QWidget *initLiveDanmuLayer(QWidget *parent);

    void initSettings();
    void initActions();
    void initSignals();

    void initPlayInfo(QWidget *playInfoPanel);
    void initPlayInfoPage();
    void initSidePanel(QWidget *parent);
    void initProgressPreview(QWidget *parent);
    QLayout *initPlayControl(QWidget *playControlPanel);


    void initPlaySetting();
    void setTrackPage(OptionMenuPanel *rootPanel);
    void setPlaybackRatePage(OptionMenuPanel *rootPanel);
    void setDisplayPage(OptionMenuPanel *rootPanel);
    void setMPVConfPage(OptionMenuPanel *rootPanel);

    void initDanmuSettting();
    void setHidePage(OptionMenuPanel *rootPanel);
    void setDanmuDisplayPage(OptionMenuPanel *rootPanel);
    void setLiveModePage(OptionMenuPanel *rootPanel);
    void setDanmuOptionItems(OptionMenuPanel *rootPanel);

    void initVolumeSetting();

    QPoint getPopupMenuPos(QWidget *ref, QWidget *popup, int topSpace = 2);
    void updateTopStatus(int option);
    void showPlayerRegion(bool on);
    void adjustPlayerSize(int percent);
    void setHideDanmu(bool hide);
    void switchItem(bool prev,const QString &nullMsg);
    void adjustProgressInfoPos();
    void exitMiniMode();

    QWidget *centralWidget() {return  cWidget;}
    void setCentralWidget(QWidget *widget);
    QWidget *cWidget;

signals:
    void toggleListVisibility(int listType);
    void enterFullScreen(bool on);
    void setStayOnTop(bool on);
    void resizePlayer(double w,double h,double aspectRatio);
    void beforeMove(const QPoint &pressPos);
    void moveWindow(const QPoint &pos);
    void miniMode(bool on);
    void refreshPool();
    void showMPVLog();

     // QWidget interface
protected:
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;

     // QObject interface
public:
     virtual bool eventFilter(QObject *watched, QEvent *event) override;


    // QWidget interface
protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    virtual void contextMenuEvent(QContextMenuEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
};

void setAwakeRequired();
void unsetAwakeRequired();

#endif // PLAYERWINDOW_H

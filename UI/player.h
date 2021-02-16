#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include "Play/Danmu/common.h"
#include "Common/notifier.h"
class QAction;
class QCheckBox;
class QFontComboBox;
class QComboBox;
class QActionGroup;
class CFramelessDialog;
class QSpinBox;
class QSlider;
class ClickSlider;
class DanmuLaunch;
class PlayerWindow : public QWidget, public NotifyInterface
{
    Q_OBJECT
public:
    explicit PlayerWindow(QWidget *parent = nullptr);
    void toggleListCollapseState(bool on);
    void toggleFullScreenState(bool on);
    virtual void showMessage(const QString &msg, int flag = 0) override;

private:
     QWidget *playControlPanel,*playInfoPanel;
     QWidget *playInfo,*playerContent,*danmuStatisBar;
     QPushButton *playListCollapseButton;
     QLabel *timeInfoTip, *previewLabel;
     QWidget *progressInfo;
     QTimer previewTimer;
     bool isShowPreview;
     bool autoHideControlPanel;

     const int statisBarHeight=64*logicalDpiY()/96;
     const int playlistCollapseWidth=25*logicalDpiX()/96;
     const int playlistCollapseHeight=80*logicalDpiX()/96;

     ClickSlider *progress, *miniProgress;
     QSlider *volume;
     QLabel *timeLabel;

     QPushButton *play_pause,*prev,*next,*stop,*mute,*setting, *danmu,*fullscreen, *launch;
     QAction *actPlayPause,*actPrev,*actNext,*actFullscreen,*act_screenshotSrc,*act_screenshotAct, *act_MiniMode;
     QTimer doublePressTimer,hideCursorTimer;
     const int hideCursorTimeout=3000;
     int ctrlPressCount,altPressCount;

     QLabel *titleLabel;
     QToolButton *mediaInfo, *windowSize,*screenshot,*stayOnTop;
     bool onTopWhilePlaying;
     QActionGroup *stayOnTopGroup,*windowSizeGroup;
     void initActions();

     bool processInited;
     bool processPressed;
     QString totalTimeStr;
     DanmuLaunch *launchWindow;

     QWidget *danmuSettingPage,*playSettingPage;
     QCheckBox *danmuSwitch,*hideRollingDanmu,*hideTopDanmu,*hideBottomDanmu,*bold,
                *bottomSubtitleProtect,*topSubtitleProtect,*randomSize,
                *enableAnalyze, *enableMerge,*enlargeMerged, *showPreview;
     QSpinBox *mergeInterval,*contentSimCount,*minMergeCount;
     QFontComboBox *fontFamilyCombo;
     QComboBox *aspectRatioCombo,*playSpeedCombo,*clickBehaviorCombo,*dbClickBehaviorCombo,
                *denseLevel,*mergeCountTipPos;
     QSlider *speedSlider,*alphaSlider,*strokeWidthSlider,*fontSizeSlider,*maxDanmuCount,
             *brightnessSlider, *contrastSlider, *saturationSlider, *gammaSlider, *hueSlider, *sharpenSlider;
     bool updatingTrack;
     bool isFullscreen;
     int resizePercent;
     int clickBehavior,dbClickBehaivior;
     int jumpForwardTime, jumpBackwardTime;
     QSharedPointer<DanmuComment> currentDanmu;

     QMenu *contexMenu;
     QAction *ctx_Text,*ctx_Copy,*ctx_BlockUser,*ctx_BlockText,*ctx_BlockColor;

     void setupDanmuSettingPage();
     void setupPlaySettingPage();
     void setupSignals();
     void adjustPlayerSize(int percent);
     void setPlayTime();
     void switchItem(bool prev,const QString &nullMsg);
     void adjustProgressInfoPos();
     void exitMiniMode();

     QWidget *centralWidget() {return  cWidget;}
     void setCentralWidget(QWidget *widget);
     QWidget *cWidget;

     bool miniModeOn, mouseLPressed, moving;
     QPoint pressPos;

signals:
    void toggleListVisibility();
    void showFullScreen(bool on);
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
    virtual void contextMenuEvent(QContextMenuEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
};

#endif // PLAYERWINDOW_H

#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include "Play/Danmu/common.h"
class QAction;
class QCheckBox;
class QFontComboBox;
class QComboBox;
class QActionGroup;
class ClickSlider: public QSlider
{
    Q_OBJECT
public:
    explicit ClickSlider(QWidget *parent=nullptr):QSlider(Qt::Horizontal,parent){}
signals:
    void sliderClick(int position);
    void sliderUp(int position);
    void mouseMove(int x,int y,int pos);
    void mouseEnter();
    void mouseLeave();
protected:
    virtual void mousePressEvent(QMouseEvent *event)
    {
        QSlider::mousePressEvent(event);
        if(!isSliderDown() && event->button() == Qt::LeftButton)
        {
            int dur = maximum()-minimum();
            double x=event->x();
            if(x<0)x=0;
            if(x>maximum())x=maximum();
            int pos = minimum() + dur * (x /width());
            emit sliderClick(pos);
            setValue(pos);
        }
    }
    virtual void mouseReleaseEvent(QMouseEvent *event)
    {
        if(isSliderDown())
        {
            int dur = maximum()-minimum();
            double x=event->x();
            if(x<0)x=0;
            if(x>maximum())x=maximum();
            int pos = minimum() + dur * (x /width());
            emit sliderUp(pos);
            setValue(pos);
        }
        QSlider::mouseReleaseEvent(event);
    }
    virtual void mouseMoveEvent(QMouseEvent *event)
    {
        int dur = maximum()-minimum();
        double x=event->x();
        if(x<0)x=0;
        if(x>maximum())x=maximum();
        int pos = minimum() + dur * (x /width());
        emit mouseMove(event->x(),event->y(),pos);
    }
    virtual void enterEvent(QEvent *)
    {
        emit mouseEnter();
    }
    virtual void leaveEvent(QEvent *)
    {
        emit mouseLeave();
    }
};
class PlayerWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit PlayerWindow(QWidget *parent = nullptr);

private:
     QWidget *playControlPanel,*playInfoPanel;
     QWidget *playInfo,*playerContent,*danmuStatisBar;
     QLabel *timeInfoTip;
     bool autoHideControlPanel;

     const int controlPanelHeight=80*logicalDpiY()/96;
     const int statisBarHeight=64*logicalDpiY()/96;
     const int infoPanelHeight=42*logicalDpiY()/96;

     ClickSlider *process,*volume;
     QLabel *timeLabel;
     QPushButton *play_pause,*prev,*next,*stop,*mute,*setting, *danmu,*fullscreen,*list;
     QAction *actPlayPause,*actPrev,*actNext,*actFullscreen;
     QTimer ctrlPressTimer;
     int ctrlPressCount;

     QLabel *titleLabel;
     QToolButton *mediaInfo, *windowSize,*screenshot,*stayOnTop;
     bool onTopWhilePlaying;
     QActionGroup *stayOnTopGroup,*windowSizeGroup;
     void initActions();

     bool processInited;
     bool processPressed;
     QString totalTimeStr;

     QWidget *danmuSettingPage,*playSettingPage;
     QCheckBox *danmuSwitch,*hideRollingDanmu,*hideTopDanmu,*hideBottomDanmu,*bold,*subtitleProtect,
                *randomSize,*denseLayout;
     QFontComboBox *fontFamilyCombo;
     QComboBox *aspectRatioCombo,*playSpeedCombo;
     QSlider *speedSlider,*alphaSlider,*strokeWidthSlider,*fontSizeSlider,*maxDanmuCount;
     bool updatingTrack;
     bool isFullscreen;
     int resizePercent;
     QSharedPointer<DanmuComment> currentDanmu;

     QMenu *contexMenu;
     QAction *ctx_Text,*ctx_Copy,*ctx_BlockUser,*ctx_BlockText,*ctx_BlockColor;

     void setupDanmuSettingPage();
     void setupPlaySettingPage();
     void setupSignals();
     void adjustPlayerSize(int percent);
     void setPlayTime();
     void showMessage(const QString &msg);
     void switchItem(bool prev,const QString &nullMsg);

signals:
    void toggleListVisibility();
    void showFullScreen(bool on);
    void setStayOnTop(bool on);
    void resizePlayer(double w,double h,double aspectRatio);

     // QWidget interface
protected:
     virtual void mouseMoveEvent(QMouseEvent *event);
     virtual void mouseDoubleClickEvent(QMouseEvent *event);
     virtual void mousePressEvent(QMouseEvent *event);
     virtual void resizeEvent(QResizeEvent *event);
     virtual void leaveEvent(QEvent *event);

     // QObject interface
public:
     virtual bool eventFilter(QObject *watched, QEvent *event);


    // QWidget interface
protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);
    virtual void closeEvent(QCloseEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
};

#endif // PLAYERWINDOW_H

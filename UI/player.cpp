#include "player.h"
#include <QVBoxLayout>
#include <QStackedLayout>
#include <QCheckBox>
#include <QFontComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QFileDialog>
#include <QActionGroup>
#include <QApplication>
#include <QToolTip>
#include <QMenu>
#include <QGraphicsOpacityEffect>
#include <QButtonGroup>
#include <QListView>

#include "widgets/clickslider.h"
#include "widgets/danmustatiswidget.h"
#include "widgets/optionslider.h"
#include "widgets/smoothscrollbar.h"
#include "capture.h"
#include "mediainfo.h"
#include "settings.h"
#include "logwindow.h"
#include "snippetcapture.h"
#include "gifcapture.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/Render/danmurender.h"
#include "Play/Danmu/Render/livedanmulistmodel.h"
#include "Play/Danmu/Render/livedanmuitemdelegate.h"
#include "Play/Danmu/Provider/localprovider.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Danmu/blocker.h"
#include "Play/Danmu/danmuprovider.h"
#include "MediaLibrary/animeworker.h"
#include "Download/util.h"
#include "danmulaunch.h"
#include "globalobjects.h"
namespace
{
class InfoTips;
class InfoTip : public QWidget
{
    friend class InfoTips;
public:
    explicit InfoTip(QWidget *parent=nullptr):QWidget(parent)
    {
        setObjectName(QStringLiteral("PlayInfoBar"));
        infoText=new QLabel(this);
        infoText->setObjectName(QStringLiteral("labelPlayInfo"));
        infoText->setFont(QFont(GlobalObjects::normalFont,10));
        infoText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
        QHBoxLayout *infoBarHLayout=new QHBoxLayout(this);
        infoBarHLayout->addWidget(infoText);
        eff = new QGraphicsOpacityEffect(this);
    }
    void showMessage(const QString &msg)
    {
        infoText->setText(msg);
        infoText->adjustSize();
        adjustSize();
        setGraphicsEffect(eff);
        eff->setOpacity(0);
        show();
        QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");
        a->setDuration(400);
        a->setStartValue(0);
        a->setEndValue(1);
        a->setEasingCurve(QEasingCurve::Linear);
        a->start(QPropertyAnimation::DeleteWhenStopped);
    }
private:
    QGraphicsOpacityEffect *eff;
    QLabel *infoText;
    QString type;
    int timeout;
};
class InfoTips : public QObject
{
public:
    InfoTips(QWidget *parent) : QObject(parent), parentWidget(parent), minBottom(0)
    {
        QObject::connect(&hideTimer,&QTimer::timeout,this, [this](){
            bool changed = false;
            for(auto iter = showTips.begin(); iter != showTips.end();)
            {
                (*iter)->timeout -= timerInterval;
                if((*iter)->timeout <= 0)
                {
                    (*iter)->hide();
                    hideTips.append(*iter);
                    iter = showTips.erase(iter);
                    changed = true;
                }
                else
                {
                    ++iter;
                }
            }
            if(changed)
            {
                updatePosition();
            }
            if(showTips.empty()) hideTimer.stop();
        });
    }
    void showMessage(const QString &msg, const QString &type = "")
    {
        InfoTip *targetInfoTip = nullptr;
        for(InfoTip *tip : showTips)
        {
            if(!type.isEmpty() && tip->type == type)
            {
                targetInfoTip = tip;
                break;
            }
        }
        if(!targetInfoTip && showTips.size() > 3)
        {
            for(InfoTip *tip : showTips)
            {
                if(tip->type.isEmpty())
                {
                    targetInfoTip = tip;
                    break;
                }
            }
        }
        if(!targetInfoTip)
        {
            targetInfoTip = hideTips.empty()? new InfoTip(parentWidget) : hideTips.takeLast();
            targetInfoTip->type = type;
            targetInfoTip->hide();
            showTips.append(targetInfoTip);
        }
        targetInfoTip->timeout = timeout;
        targetInfoTip->showMessage(msg);
        updatePosition();
        if(!hideTimer.isActive()) hideTimer.start(timerInterval);
    }
    void updatePosition()
    {
        int curBottom = minBottom;
        int width = parentWidget->width();
        const int tipSpace = 4 * parentWidget->logicalDpiY() / 96;
        for(InfoTip *tip : showTips)
        {
            int x = (width-tip->width())/2;
            int y = curBottom - tip->height() - tipSpace;
            tip->move(x, y);
            tip->show();
            tip->raise();
            curBottom = y;
        }
    }
    void setBottom(int bottom)
    {
        minBottom = bottom;
    }

private:
    QVector<InfoTip *> showTips, hideTips;
    QWidget *parentWidget;
    int minBottom;
    const int timeout = 2500;
    const int timerInterval = 500;
    QTimer hideTimer;
};

class RecentItem : public QWidget
{
public:
    explicit RecentItem(QWidget *parent=nullptr):QWidget(parent)
    {
        setObjectName(QStringLiteral("RecentItem"));
        titleLabel=new QLabel(this);
        titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
        titleLabel->setObjectName(QStringLiteral("RecentItemLabel"));
        deleteItem=new QPushButton(this);
        deleteItem->setObjectName(QStringLiteral("RecentItemDeleteButton"));
        QObject::connect(deleteItem,&QPushButton::clicked,[this](){
            GlobalObjects::playlist->removeRecentItem(path);
        });
        GlobalObjects::iconfont->setPointSize(10);
        deleteItem->setFont(*GlobalObjects::iconfont);
        deleteItem->setText(QChar(0xe60b));
        deleteItem->hide();
        QHBoxLayout *itemHLayout=new QHBoxLayout(this);
        itemHLayout->addStretch(1);
        itemHLayout->addWidget(titleLabel);
        itemHLayout->addStretch(1);
        itemHLayout->addWidget(deleteItem);
        setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    }
    void setData(const QPair<QString,QString> &pair)
    {
        path=pair.first;
        titleLabel->setText(pair.second);
        titleLabel->adjustSize();
        adjustSize();
        show();
    }
private:
    QLabel *titleLabel;
    QPushButton *deleteItem;
    QString path;
protected:
    virtual void mouseReleaseEvent(QMouseEvent *event)
    {
        if(event->button()==Qt::LeftButton)
        {
            const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(path);
            if (curItem)
            {
                GlobalObjects::danmuPool->reset();
                GlobalObjects::danmuRender->cleanup();
                GlobalObjects::mpvplayer->setMedia(curItem->path);
            }
        }
        event->accept();
    }
    virtual void enterEvent(QEvent *event)
    {
        deleteItem->show();
        QWidget::enterEvent(event);
    }
    virtual void leaveEvent(QEvent *event)
    {
        deleteItem->hide();
        QWidget::leaveEvent(event);
    }
};
class PlayerContent : public QWidget
{
public:
    explicit PlayerContent(QWidget *parent=nullptr):QWidget(parent)
    {
        setObjectName(QStringLiteral("PlayerContent"));
        logo=new QLabel(this);
        logo->setPixmap(QPixmap(":/res/images/kikoplay-5.png"));
        logo->setAlignment(Qt::AlignCenter);
        QVBoxLayout *pcVLayout=new QVBoxLayout(this);
        pcVLayout->addStretch(1);
        pcVLayout->addWidget(logo);
        pcVLayout->addSpacing(10);
        for(int i=0;i<GlobalObjects::playlist->maxRecentItems;++i)
        {
            RecentItem *recentItem=new RecentItem(this);
            pcVLayout->addWidget(recentItem);
            items.append(recentItem);
        }
        pcVLayout->addStretch(1);
        refreshItems();

        logo->installEventFilter(this);
    }
    void refreshItems()
    {
        int i=0;
        const int maxRecentCount=GlobalObjects::playlist->maxRecentItems;
        auto &recent=GlobalObjects::playlist->recent();
        for(;i<maxRecentCount && i<recent.count();++i)
            items[i]->setData(recent[i]);
        while(i<maxRecentCount)
            items[i++]->hide();
    }
    bool eventFilter(QObject *watched, QEvent *event)
    {
        if(watched==logo && event->type() == QEvent::MouseButtonRelease)
        {
            QStringList files = QFileDialog::getOpenFileNames(this,tr("Select one or more media files"),"",
                                                             QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
            if(!files.isEmpty())
            {
                GlobalObjects::playlist->addItems(files,QModelIndex());
                const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(files.first());
                if (curItem)
                {
                    GlobalObjects::danmuPool->reset();
                    GlobalObjects::danmuRender->cleanup();
                    GlobalObjects::mpvplayer->setMedia(curItem->path);
                }

            }

            return true;
        }
        return QWidget::eventFilter(watched,event);
    }
private:
    QList<RecentItem *> items;
    QLabel *logo;
};
class EdgeFadeEffect : public QGraphicsEffect
{
    // QGraphicsEffect interface
public:
    EdgeFadeEffect(QObject *parent = nullptr) : QGraphicsEffect(parent)
    {
        int ptSize = GlobalObjects::appSetting->value("Play/LiveDanmuFontSize", 10).toInt();
        fadeHeight = QFontMetrics(QFont(GlobalObjects::normalFont, ptSize)).height() * 1.5;
    }
private:
    double fadeHeight = 0.0;
protected:
    void draw(QPainter *painter)
    {
        QPixmap source = sourcePixmap();
        QImage composionImg(source.size(), QImage::Format_ARGB32_Premultiplied);
        composionImg.fill(0);
        QLinearGradient fillGradient;
        fillGradient.setColorAt(0, QColor(0, 0, 0, 0));
        fillGradient.setColorAt(1, QColor(0, 0, 0, 255));
        fillGradient.setStart(0, 0);
        fillGradient.setFinalStop(0, qMin(fadeHeight, composionImg.height() * 0.1));
        QPainter tp(&composionImg);
        tp.fillRect(composionImg.rect(), QBrush(fillGradient));
        tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tp.drawPixmap(0, 0, source);
        tp.end();
        painter->drawImage(0, 0, composionImg);
    }
};
}
PlayerWindow::PlayerWindow(QWidget *parent) : QWidget(parent),autoHideControlPanel(true),
    onTopWhilePlaying(false),isFullscreen(false),resizePercent(-1),jumpForwardTime(5),jumpBackwardTime(5),
    autoLoadLocalDanmu(true), hasExternalAudio(false), hasExternalSub(false), miniModeOn(false),
    mouseLPressed(false), moving(false)
{
    Notifier::getNotifier()->addNotify(Notifier::PLAYER_NOTIFY, this);
    setWindowFlags(Qt::FramelessWindowHint);
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setMouseTracking(true);
    setCentralWidget(centralWidget);
    QWidget *contralContainer = new QWidget(centralWidget);

    launchWindow = new DanmuLaunch(this);

    playInfo=new InfoTips(centralWidget);

    progressInfo=new QWidget(centralWidget);
    QStackedLayout *piSLayout = new QStackedLayout(progressInfo);
    piSLayout->setStackingMode(QStackedLayout::StackAll);
    piSLayout->setContentsMargins(0,0,0,0);
    piSLayout->setSpacing(0);
    QWidget *tiWidget = new QWidget(progressInfo);
    QVBoxLayout *piVLayout = new QVBoxLayout(tiWidget);
    piVLayout->setContentsMargins(0,0,0,0);
    piVLayout->setSpacing(0);
    QHBoxLayout *piHLayout = new QHBoxLayout();
    piHLayout->setContentsMargins(0,0,0,0);
    piHLayout->setSpacing(0);

    timeInfoTip=new QLabel(tiWidget);
    timeInfoTip->setObjectName(QStringLiteral("TimeInfoTip"));
    timeInfoTip->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    previewLabel=new QLabel(progressInfo);
    previewLabel->setObjectName(QStringLiteral("ProgressPreview"));
    isShowPreview = GlobalObjects::appSetting->value("Play/ShowPreview",true).toBool();
    piVLayout->addStretch(1);
    piHLayout->addWidget(timeInfoTip);
    piHLayout->addStretch(1);
    piVLayout->addLayout(piHLayout);
    piSLayout->addWidget(tiWidget);
    piSLayout->addWidget(previewLabel);
    previewLabel->hide();
    progressInfo->hide();

    QObject::connect(&previewTimer,&QTimer::timeout,[this](){
        if(isShowPreview && !progressInfo->isHidden() && previewLabel->isHidden())
        {
            QPixmap *pixmap = GlobalObjects::mpvplayer->getPreview(progress->curMousePos()/1000);
            if(pixmap)
            {
                previewLabel->resize(pixmap->width(), pixmap->height());
                previewLabel->setPixmap(*pixmap);
                timeInfoTip->setMaximumWidth(pixmap->width());
                adjustProgressInfoPos();
                previewLabel->show();
            }
        }
        previewTimer.stop();
    });

    playerContent=new PlayerContent(contralContainer);
    //playerContent->show();
    playerContent->raise();

    playInfoPanel=new QWidget(contralContainer);
    playInfoPanel->setObjectName(QStringLiteral("widgetPlayInfo"));
    playInfoPanel->hide();

    playListCollapseButton=new QPushButton(contralContainer);
    playListCollapseButton->setObjectName(QStringLiteral("widgetPlayListCollapse"));
    GlobalObjects::iconfont->setPointSize(12);
    playListCollapseButton->setFont(*GlobalObjects::iconfont);
    playListCollapseButton->setText(QChar(GlobalObjects::appSetting->value("MainWindow/ListVisibility",true).toBool()?0xe945:0xe946));
    playListCollapseButton->hide();

    DanmuStatisWidget *statWidget = new DanmuStatisWidget(contralContainer);
    statWidget->setObjectName(QStringLiteral("DanmuStatisBar"));
    QObject::connect(GlobalObjects::danmuPool,&DanmuPool::statisInfoChange,statWidget,&DanmuStatisWidget::refreshStatis);
    danmuStatisBar = statWidget;
    danmuStatisBar->setMinimumHeight(statisBarHeight);
    danmuStatisBar->hide();

    QFont normalFont;
    normalFont.setFamily(GlobalObjects::normalFont);
    normalFont.setPointSize(12);

    titleLabel=new QLabel(playInfoPanel);
    titleLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    titleLabel->setObjectName(QStringLiteral("labelTitle"));
    titleLabel->setFont(normalFont);

    constexpr const int infoBtnCount = 4;
    QPair<QChar, QString> infoButtonTexts[infoBtnCount] = {
        {QChar(0xe727), tr("Media Info")},
        {QChar(0xe720), tr("Window Size")},
        {QChar(0xe741), tr("Screenshot")},
        {QChar(0xe65a), tr("On Top")}
    };
    QToolButton **infoBtnPtrs[infoBtnCount] = {
        &mediaInfo, &windowSize, &screenshot, &stayOnTop
    };
    GlobalObjects::iconfont->setPointSize(12);
    QHBoxLayout *infoHLayout=new QHBoxLayout(playInfoPanel);
    infoHLayout->setSpacing(0);
    infoHLayout->addWidget(titleLabel);
    infoHLayout->addStretch(1);
    for(int i = 0; i < infoBtnCount; ++i)
    {
        *infoBtnPtrs[i] = new QToolButton(playInfoPanel);
        QToolButton *tb = *infoBtnPtrs[i];
        tb->setFont(*GlobalObjects::iconfont);
        tb->setText(infoButtonTexts[i].first);
        tb->setToolTip(infoButtonTexts[i].second);
        tb->setObjectName(QStringLiteral("ListEditButton"));
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setPopupMode(QToolButton::InstantPopup);
        infoHLayout->addWidget(tb);
#ifdef Q_OS_MACOS
        tb->setProperty("hideMenuIndicator", true);
#endif
    }

    playControlPanel=new QWidget(contralContainer);
    playControlPanel->setObjectName(QStringLiteral("widgetPlayControl"));
    playControlPanel->hide();

    QVBoxLayout *controlVLayout=new QVBoxLayout(playControlPanel);
    controlVLayout->setSpacing(0);
    progress=new ClickSlider(playControlPanel);
    progress->setObjectName(QStringLiteral("widgetProcessSlider"));
    progress->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
    progress->setTracking(false);
    progressPressed=false;
    progress->setMouseTracking(true);
    controlVLayout->addWidget(progress);

    GlobalObjects::iconfont->setPointSize(24);

    timeLabel=new QLabel("00:00/00:00",playControlPanel);
    timeLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    timeLabel->setObjectName(QStringLiteral("labelTime"));
    normalFont.setPointSize(10);
    timeLabel->setFont(normalFont);

    playPause=new QPushButton(playControlPanel);
    playPause->setFont(*GlobalObjects::iconfont);
    playPause->setText(QChar(0xe606));
    //play_pause->setFixedSize(buttonWidth,buttonHeight);
    playPause->setObjectName(QStringLiteral("widgetPlayControlButtons"));
    playPause->setToolTip(tr("Play/Pause(Space)"));

    GlobalObjects::iconfont->setPointSize(20);
    int buttonWidth=36*logicalDpiX()/96,buttonHeight=36*logicalDpiY()/96;

    prev=new QPushButton(playControlPanel);
    prev->setFont(*GlobalObjects::iconfont);
    prev->setText(QChar(0xe69b));
    prev->setFixedSize(buttonWidth,buttonHeight);
    prev->setToolTip(tr("Prev(PageUp)"));
    prev->setObjectName(QStringLiteral("widgetPlayControlButtons"));

    next=new QPushButton(playControlPanel);
    next->setFont(*GlobalObjects::iconfont);
    next->setText(QChar(0xe940));
    next->setFixedSize(buttonWidth,buttonHeight);
    next->setToolTip(tr("Next(PageDown)"));
    next->setObjectName(QStringLiteral("widgetPlayControlButtons"));

    stop=new QPushButton(playControlPanel);
    stop->setFont(*GlobalObjects::iconfont);
    stop->setText(QChar(0xe6fa));
    stop->setFixedSize(buttonWidth,buttonHeight);
    stop->setToolTip(tr("Stop"));
    stop->setObjectName(QStringLiteral("widgetPlayControlButtons"));

    mute=new QPushButton(playControlPanel);
    mute->setFont(*GlobalObjects::iconfont);
    mute->setText(QChar(0xe62c));
    mute->setFixedSize(buttonWidth,buttonHeight);
    mute->setToolTip(tr("Mute/Unmute"));
    mute->setObjectName(QStringLiteral("widgetPlayControlButtons"));

    GlobalObjects::iconfont->setPointSize(18);

    launch=new QPushButton(playControlPanel);
    launch->setFont(*GlobalObjects::iconfont);
    launch->setText(QChar(0xe947));
    launch->setFixedSize(buttonWidth,buttonHeight);
    launch->setToolTip(tr("Launch Danmu"));
    launch->setObjectName(QStringLiteral("widgetPlayControlButtons"));
    launch->hide();

    setting=new QPushButton(playControlPanel);
    setting->setFont(*GlobalObjects::iconfont);
    setting->setText(QChar(0xe607));
    setting->setFixedSize(buttonWidth,buttonHeight);
    setting->setToolTip(tr("Play Setting"));
    setting->setObjectName(QStringLiteral("widgetPlayControlButtons"));

    danmu=new QPushButton(playControlPanel);
    danmu->setFont(*GlobalObjects::iconfont);
    danmu->setText(QChar(0xe622));
    danmu->setFixedSize(buttonWidth,buttonHeight);
    danmu->setToolTip(tr("Danmu Setting"));
    danmu->setObjectName(QStringLiteral("widgetPlayControlButtons"));

    fullscreen=new QPushButton(playControlPanel);
    fullscreen->setFont(*GlobalObjects::iconfont);
    fullscreen->setText(QChar(0xe621));
    fullscreen->setFixedSize(buttonWidth,buttonHeight);
    fullscreen->setToolTip(tr("FullScreen"));
    fullscreen->setObjectName(QStringLiteral("widgetPlayControlButtons"));

    volume=new QSlider(Qt::Horizontal,playControlPanel);
    volume->setObjectName(QStringLiteral("widgetVolumeSlider"));
    volume->setFixedWidth(100*logicalDpiX()/96);
    volume->setMinimum(0);
    volume->setMaximum(150);
    volume->setSingleStep(1);

    ctrlPressCount=0;
    altPressCount=0;
    QObject::connect(&doublePressTimer,&QTimer::timeout,[this](){
        ctrlPressCount=0;
        altPressCount=0;
        doublePressTimer.stop();
    });
    QObject::connect(&hideCursorTimer,&QTimer::timeout,[this](){
        if(isFullscreen)setCursor(Qt::BlankCursor);
    });


    QStackedLayout *controlSLayout = new QStackedLayout();
    controlSLayout->setStackingMode(QStackedLayout::StackAll);
    controlSLayout->setContentsMargins(0,0,0,0);
    controlSLayout->setSpacing(0);

    QWidget *btnContainer = new QWidget(playControlPanel);
    QHBoxLayout *buttonHLayout=new QHBoxLayout(btnContainer);
    //QWidget *Container = new QWidget(playControlPanel);
    //QHBoxLayout *buttonHLayout=new QHBoxLayout(btnContainer);

    //buttonHLayout->addWidget(timeLabel);
    buttonHLayout->addStretch(3);
    buttonHLayout->addWidget(stop);
    buttonHLayout->addWidget(prev);
    buttonHLayout->addWidget(playPause);
    buttonHLayout->addWidget(next);
    buttonHLayout->addWidget(mute);
    buttonHLayout->addWidget(volume);
    buttonHLayout->addStretch(2);
    buttonHLayout->addWidget(launch);
    buttonHLayout->addWidget(setting);
    buttonHLayout->addWidget(danmu);
    buttonHLayout->addWidget(fullscreen);

    controlSLayout->addWidget(btnContainer);
    controlSLayout->addWidget(timeLabel);

    //controlVLayout->addLayout(buttonHLayout);
    controlVLayout->addLayout(controlSLayout);

    miniProgress = new ClickSlider(contralContainer);
    miniProgress->setObjectName(QStringLiteral("MiniProgressSlider"));
    miniProgress->hide();

    GlobalObjects::mpvplayer->setParent(centralWidget);
    setContentsMargins(0,0,0,0);
    GlobalObjects::mpvplayer->setIccProfileOption();

    contralContainer->setMouseTracking(true);
    QVBoxLayout *contralVLayout = new QVBoxLayout(contralContainer);
    contralVLayout->setContentsMargins(0,0,0,0);
    contralVLayout->setSpacing(0);
    contralVLayout->addWidget(playInfoPanel);
    contralVLayout->addStretch(1);
    contralVLayout->addWidget(miniProgress);
    contralVLayout->addWidget(danmuStatisBar);
    contralVLayout->addWidget(playControlPanel);

    QWidget *liveDanmuContainer = new QWidget(centralWidget);
    liveDanmuList = new QListView(liveDanmuContainer);
    liveDanmuList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    liveDanmuList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    liveDanmuList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    liveDanmuList->setObjectName(QStringLiteral("LiveDanmuList"));
    liveDanmuList->setVerticalScrollBar(new SmoothScrollBar(liveDanmuList));
    liveDanmuList->setResizeMode(QListView::ResizeMode::Adjust);
    liveDanmuList->setGraphicsEffect(new EdgeFadeEffect);
    liveDanmuList->setModel(GlobalObjects::danmuRender->liveDanmuModel());
    liveDanmuList->setItemDelegate(new LiveDanmuItemDelegate(this));
    QVBoxLayout *liveVLayout = new QVBoxLayout(liveDanmuContainer);
    liveVLayout->addStretch(1);
    liveVLayout->addWidget(liveDanmuList, 0, Qt::AlignLeft);
    liveDanmuList->hide();

    QStackedLayout *playerSLayout = new QStackedLayout(centralWidget);
    playerSLayout->setStackingMode(QStackedLayout::StackAll);
    playerSLayout->setContentsMargins(0,0,0,0);
    playerSLayout->setSpacing(0);
    playerSLayout->addWidget(contralContainer);
    playerSLayout->addWidget(liveDanmuContainer);
    playerSLayout->addWidget(GlobalObjects::mpvplayer);

    setupDanmuSettingPage();
    setupPlaySettingPage();
    initActions();
    setupSignals();
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
}

void PlayerWindow::toggleListCollapseState(bool on)
{
    playListCollapseButton->setText(on?QChar(0xe945):QChar(0xe946));
}

void PlayerWindow::toggleFullScreenState(bool on)
{
    isFullscreen=on;
    if(isFullscreen) fullscreen->setText(QChar(0xe6ac));
    else fullscreen->setText(QChar(0xe621));
    if(isFullscreen)
    {
        hideCursorTimer.start(hideCursorTimeout);
    }
    else
    {
        hideCursorTimer.stop();
        setCursor(Qt::ArrowCursor);
    }
}

void PlayerWindow::initActions()
{
    windowSizeGroup=new QActionGroup(this);
    QAction *act_size50p = new QAction("50%",this);
    act_size50p->setCheckable(true);
    act_size50p->setData(QVariant(50));
    windowSizeGroup->addAction(act_size50p);
    QAction *act_size75p = new QAction("75%",this);
    act_size75p->setCheckable(true);
    act_size75p->setData(QVariant(75));
    windowSizeGroup->addAction(act_size75p);
    QAction *act_size100p = new QAction("100%",this);
    act_size100p->setCheckable(true);
    act_size100p->setData(QVariant(100));
    windowSizeGroup->addAction(act_size100p);
    QAction *act_size150p = new QAction("150%",this);
    act_size150p->setCheckable(true);
    act_size150p->setData(QVariant(150));
    windowSizeGroup->addAction(act_size150p);
    QAction *act_size200p = new QAction("200%",this);
    act_size200p->setCheckable(true);
    act_size200p->setData(QVariant(200));
    windowSizeGroup->addAction(act_size200p);
    QAction *act_sizeFix = new QAction(tr("Fix"),this);
    act_sizeFix->setCheckable(true);
    act_sizeFix->setData(QVariant(-1));
    windowSizeGroup->addAction(act_sizeFix);

    QObject::connect(windowSizeGroup,&QActionGroup::triggered,[this](QAction *action){
        resizePercent=action->data().toInt();
        if(resizePercent==-1)return;
        adjustPlayerSize(resizePercent);
    });
    windowSizeGroup->actions().at(GlobalObjects::appSetting->value("Play/WindowSize",2).toInt())->trigger();
    actMiniMode = new QAction(tr("Mini Mode"),this);
    QObject::connect(actMiniMode, &QAction::triggered, this, [this](){
        if(GlobalObjects::playlist->getCurrentItem() != nullptr && !miniModeOn)
        {
            if(isFullscreen)
            {
                toggleFullScreenState(!isFullscreen);
                emit showFullScreen(isFullscreen);
            }
            miniModeOn = true;
            playInfoPanel->hide();
            playControlPanel->hide();
            danmuStatisBar->hide();
            danmuSettingPage->hide();
            playSettingPage->hide();
            emit miniMode(true);
        }
    });
    windowSize->addActions(windowSizeGroup->actions());
    windowSize->addAction(actMiniMode);

    actScreenshotSrc = new QAction(tr("Original Video"),this);
    QObject::connect(actScreenshotSrc,&QAction::triggered,[this](){
        QTemporaryFile tmpImg("XXXXXX.jpg");
        if(tmpImg.open())
        {
            GlobalObjects::mpvplayer->screenshot(tmpImg.fileName());
            QImage captureImage(tmpImg.fileName());
            const PlayListItem *curItem=GlobalObjects::playlist->getCurrentItem();
            Capture captureDialog(captureImage,screenshot,curItem);
            QRect geo(captureDialog.geometry());
            geo.moveCenter(this->geometry().center());
            captureDialog.move(geo.topLeft());
            captureDialog.exec();
        }
    });
    actScreenshotAct = new QAction(tr("Actual content"),this);
    QObject::connect(actScreenshotAct,&QAction::triggered,[this](){
        QImage captureImage(GlobalObjects::mpvplayer->grabFramebuffer());
        if(liveDanmuList->isVisible())
        {
            QPainter p(&captureImage);
            p.drawPixmap(liveDanmuList->pos(), liveDanmuList->grab());
        }
        const PlayListItem *curItem=GlobalObjects::playlist->getCurrentItem();
        Capture captureDialog(captureImage,screenshot,curItem);
        QRect geo(captureDialog.geometry());
        geo.moveCenter(this->geometry().center());
        captureDialog.move(geo.topLeft());
        captureDialog.exec();
    });
    actSnippet = new QAction(tr("Snippet Capture"),this);
    QObject::connect(actSnippet,&QAction::triggered,[this](){
        const PlayListItem *curItem=GlobalObjects::playlist->getCurrentItem();
        if(!curItem) return;
        SnippetCapture snippet(this);
        QRect geo(0,0,400*logicalDpiX()/96,600*logicalDpiY()/96);
        geo.moveCenter(this->geometry().center());
        snippet.move(mapToGlobal(geo.topLeft()));
        snippet.exec();
        GlobalObjects::mpvplayer->setMute(GlobalObjects::mpvplayer->getMute());
    });
    actGIF = new QAction(tr("GIF Capture"),this);
    QObject::connect(actGIF,&QAction::triggered,[this](){
        const PlayListItem *curItem=GlobalObjects::playlist->getCurrentItem();
        if(!curItem) return;
        GIFCapture gifCapture("", true, this);
        QRect geo(0,0,400*logicalDpiX()/96,600*logicalDpiY()/96);
        geo.moveCenter(this->geometry().center());
        gifCapture.move(mapToGlobal(geo.topLeft()));
        gifCapture.exec();
        GlobalObjects::mpvplayer->setMute(GlobalObjects::mpvplayer->getMute());
    });
    screenshot->addAction(actScreenshotSrc);
    screenshot->addAction(actScreenshotAct);
    screenshot->addAction(actSnippet);
    screenshot->addAction(actGIF);

    stayOnTopGroup=new QActionGroup(this);
    QAction *act_onTopPlaying = new QAction(tr("While Playing"),this);
    act_onTopPlaying->setCheckable(true);
    act_onTopPlaying->setData(QVariant(0));
    stayOnTopGroup->addAction(act_onTopPlaying);
    QAction *act_onTopAlways = new QAction(tr("Always"),this);
    act_onTopAlways->setCheckable(true);
    act_onTopAlways->setData(QVariant(1));
    stayOnTopGroup->addAction(act_onTopAlways);
    QAction *act_onTopNever = new QAction(tr("Never"),this);
    act_onTopNever->setCheckable(true);
    act_onTopNever->setData(QVariant(2));
    stayOnTopGroup->addAction(act_onTopNever);
    QObject::connect(stayOnTopGroup,&QActionGroup::triggered,[this](QAction *action){
        switch (action->data().toInt())
        {
        case 0:
            onTopWhilePlaying=true;
            if(GlobalObjects::mpvplayer->getState()==MPVPlayer::Play && GlobalObjects::playlist->getCurrentItem() != nullptr)
                emit setStayOnTop(true);
			else
				emit setStayOnTop(false);
            break;
        case 1:
            onTopWhilePlaying=false;
            emit setStayOnTop(true);
            break;
        case 2:
            onTopWhilePlaying=false;
            emit setStayOnTop(false);
            break;
        default:
            break;
        }
    });
    stayOnTopGroup->actions().at(GlobalObjects::appSetting->value("Play/StayOnTop",0).toInt())->trigger();
    stayOnTop->addActions(stayOnTopGroup->actions());

    actPlayPause=new QAction(tr("Play/Pause"),this);
    QObject::connect(actPlayPause,&QAction::triggered,[](){
        MPVPlayer::PlayState state=GlobalObjects::mpvplayer->getState();
        switch(state)
        {
        case MPVPlayer::Play:
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
            break;
        case MPVPlayer::EndReached:
        {
            GlobalObjects::mpvplayer->seek(0);
            GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
            break;
        }
        case MPVPlayer::Pause:
            GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
            break;
        case MPVPlayer::Stop:
            break;
        }
    });
    actFullscreen=new QAction(tr("Fullscreen"),this);
    QObject::connect(actFullscreen,&QAction::triggered,[this](){
        toggleFullScreenState(!isFullscreen);
        emit showFullScreen(isFullscreen);
    });
    actPrev=new QAction(tr("Prev"),this);
    QObject::connect(actPrev,&QAction::triggered,[this](){
        switchItem(true,tr("No prev item"));
    });
    actNext=new QAction(tr("Next"),this);
    QObject::connect(actNext,&QAction::triggered,[this](){
        switchItem(false,tr("No next item"));
    });
    contexMenu=new QMenu(this);
    ctxText=contexMenu->addAction("");
    QObject::connect(ctxText,&QAction::triggered,[this](){
        currentDanmu=nullptr;
    });
    ctxCopy=contexMenu->addAction(tr("Copy Text"));
    QObject::connect(ctxCopy,&QAction::triggered,[this](){
        if(currentDanmu.isNull())return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(currentDanmu->text);
        currentDanmu=nullptr;
    });
    contexMenu->addSeparator();
    ctxBlockText=contexMenu->addAction(tr("Block Text"));
    QObject::connect(ctxBlockText,&QAction::triggered,[this](){
        if(currentDanmu.isNull())return;
        BlockRule *rule = new BlockRule(currentDanmu->text, BlockRule::Field::DanmuText, BlockRule::Relation::Contain);
        rule->name = tr("Text Rule");
        GlobalObjects::blocker->addBlockRule(rule);
        currentDanmu=nullptr;
        showMessage(tr("Block Rule Added"));
    });
    ctxBlockUser=contexMenu->addAction(tr("Block User"));
    QObject::connect(ctxBlockUser,&QAction::triggered,[this](){
        if(currentDanmu.isNull())return;
        BlockRule *rule = new BlockRule(currentDanmu->sender, BlockRule::Field::DanmuSender, BlockRule::Relation::Equal);
        rule->name = tr("User Rule");
        GlobalObjects::blocker->addBlockRule(rule);
        currentDanmu=nullptr;
        showMessage(tr("Block Rule Added"));
    });
    ctxBlockColor=contexMenu->addAction(tr("Block Color"));
    QObject::connect(ctxBlockColor,&QAction::triggered,[this](){
        if(currentDanmu.isNull())return;
        BlockRule *rule = new BlockRule(QString::number(currentDanmu->color,16), BlockRule::Field::DanmuColor, BlockRule::Relation::Equal);
        rule->name = tr("Color Rule");
        GlobalObjects::blocker->addBlockRule(rule);
        currentDanmu=nullptr;
        showMessage(tr("Block Rule Added"));
    });
}

void PlayerWindow::setupDanmuSettingPage()
{
    danmuSettingPage=new QWidget(this->centralWidget());
    danmuSettingPage->setObjectName(QStringLiteral("PopupPage"));
    danmuSettingPage->installEventFilter(this);

    QToolButton *generalPage=new QToolButton(danmuSettingPage);
    generalPage->setText(tr("General"));
    generalPage->setCheckable(true);
    generalPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    generalPage->setObjectName(QStringLiteral("DialogPageButton"));
    generalPage->setChecked(true);

    QToolButton *appearancePage=new QToolButton(danmuSettingPage);
    appearancePage->setText(tr("Appearance"));
    appearancePage->setCheckable(true);
    appearancePage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    appearancePage->setObjectName(QStringLiteral("DialogPageButton"));

    QToolButton *advancedPage=new QToolButton(danmuSettingPage);
    advancedPage->setText(tr("Advanced"));
    advancedPage->setCheckable(true);
    advancedPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    advancedPage->setObjectName(QStringLiteral("DialogPageButton"));

    const int btnH = generalPage->fontMetrics().height() + 10*logicalDpiY()/96;
    generalPage->setFixedHeight(btnH);
    appearancePage->setFixedHeight(btnH);
    advancedPage->setFixedHeight(btnH);

    QStackedLayout *danmuSettingSLayout=new QStackedLayout();
    QButtonGroup *btnGroup=new QButtonGroup(this);
    btnGroup->addButton(generalPage,0);
    btnGroup->addButton(appearancePage,1);
    btnGroup->addButton(advancedPage,2);
    QObject::connect(btnGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[danmuSettingSLayout](int id, bool checked){
        if(checked)
        {
            danmuSettingSLayout->setCurrentIndex(id);
        }
    });

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->addWidget(generalPage);
    pageButtonHLayout->addWidget(appearancePage);
    pageButtonHLayout->addWidget(advancedPage);

    QVBoxLayout *danmuSettingVLayout=new QVBoxLayout(danmuSettingPage);
    danmuSettingVLayout->addLayout(pageButtonHLayout);
    danmuSettingVLayout->addLayout(danmuSettingSLayout);

//General Page
    QWidget *pageGeneral=new QWidget(danmuSettingPage);

    QLabel *hideLabel=new QLabel(tr("Hide Danmu"), pageGeneral);
    danmuSwitch=new QCheckBox(tr("All"),pageGeneral);
    danmuSwitch->setToolTip("Double Press Ctrl");
    QObject::connect(danmuSwitch,&QCheckBox::stateChanged,[this](int state){
        if(state==Qt::Checked)
        {
            GlobalObjects::mpvplayer->hideDanmu(true);
            liveDanmuList->setVisible(false);
            this->danmu->setText(QChar(0xe620));
        }
        else
        {
            GlobalObjects::mpvplayer->hideDanmu(false);
            if(enableLiveMode->isChecked() && GlobalObjects::playlist->getCurrentItem())
            {
                liveDanmuList->setVisible(true);
            }
            this->danmu->setText(QChar(0xe622));
        }
    });
    danmuSwitch->setChecked(GlobalObjects::appSetting->value("Play/HideDanmu",false).toBool());

    hideRollingDanmu=new QCheckBox(tr("Rolling"),pageGeneral);
    QObject::connect(hideRollingDanmu,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuRender->hideDanmu(DanmuComment::Rolling,state==Qt::Checked?true:false);
    });
    hideRollingDanmu->setChecked(GlobalObjects::appSetting->value("Play/HideRolling",false).toBool());

    hideTopDanmu=new QCheckBox(tr("Top"),pageGeneral);
    QObject::connect(hideTopDanmu,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuRender->hideDanmu(DanmuComment::Top,state==Qt::Checked?true:false);
    });
    hideTopDanmu->setChecked(GlobalObjects::appSetting->value("Play/HideTop",false).toBool());

    hideBottomDanmu=new QCheckBox(tr("Bottom"),pageGeneral);
    QObject::connect(hideBottomDanmu,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuRender->hideDanmu(DanmuComment::Bottom,state==Qt::Checked?true:false);
    });
    hideBottomDanmu->setChecked(GlobalObjects::appSetting->value("Play/HideBottom",false).toBool());

    QLabel *subProtectLabel=new QLabel(tr("Sub Protect"), pageGeneral);
    bottomSubtitleProtect=new QCheckBox(tr("Bottom Sub"),pageGeneral);
    bottomSubtitleProtect->setChecked(true);
    QObject::connect(bottomSubtitleProtect,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuRender->setBottomSubtitleProtect(state==Qt::Checked?true:false);
    });
    bottomSubtitleProtect->setChecked(GlobalObjects::appSetting->value("Play/BottomSubProtect",true).toBool());

    topSubtitleProtect=new QCheckBox(tr("Top Sub"),pageGeneral);
    QObject::connect(topSubtitleProtect,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuRender->setTopSubtitleProtect(state==Qt::Checked?true:false);
    });
    topSubtitleProtect->setChecked(GlobalObjects::appSetting->value("Play/TopSubProtect",false).toBool());

    QLabel *speedLabel=new QLabel(tr("Rolling Speed"),pageGeneral);
    speedSlider=new QSlider(Qt::Horizontal,pageGeneral);
    speedSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    speedSlider->setRange(50,500);
    QObject::connect(speedSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::danmuRender->setSpeed(val);
        speedSlider->setToolTip(QString::number(val));
    });
    speedSlider->setValue(GlobalObjects::appSetting->value("Play/DanmuSpeed",200).toInt());

    QLabel *maxDanmuCountLabel=new QLabel(tr("Max Count"),pageGeneral);
    maxDanmuCount=new QSlider(Qt::Horizontal,pageGeneral);
    maxDanmuCount->setObjectName(QStringLiteral("PopupPageSlider"));
    maxDanmuCount->setRange(20,300);
    QObject::connect(maxDanmuCount,&QSlider::valueChanged,[this](int val){
        GlobalObjects::danmuRender->setMaxDanmuCount(val);
        maxDanmuCount->setToolTip(QString::number(val));
    });
    maxDanmuCount->setValue(GlobalObjects::appSetting->value("Play/MaxCount",100).toInt());

    QLabel *denseLabel=new QLabel(tr("Dense Level"),pageGeneral);
    denseLevel = new OptionSlider(pageGeneral);
    denseLevel->setLabels({tr("Uncovered"), tr("General"), tr("Dense")});

    QObject::connect(denseLevel, &OptionSlider::valueChanged, [](int index){
         GlobalObjects::danmuRender->dense=index;
    });
    denseLevel->setValue(GlobalObjects::appSetting->value("Play/Dense",1).toInt());

    QLabel *displayAreaLabel=new QLabel(tr("Display Area"), pageGeneral);
    displayAreaSlider = new OptionSlider(pageGeneral);
    displayAreaSlider->setLabels({tr("1/4"), tr("1/2"), tr("3/4"), tr("Full")});
    displayAreaSlider->setValue(3);
    QObject::connect(displayAreaSlider, &OptionSlider::valueChanged, [](int val){
        GlobalObjects::danmuRender->setDisplayArea((val + 1.0f) / 4.0f);
    });
    displayAreaSlider->setValue(GlobalObjects::appSetting->value("Play/DisplayArea", 3).toInt());


//Appearance Page
    QWidget *pageAppearance=new QWidget(danmuSettingPage);

    QLabel *alphaLabel=new QLabel(tr("Danmu Alpha"),pageAppearance);
    alphaSlider=new QSlider(Qt::Horizontal,pageAppearance);
    alphaSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    alphaSlider->setRange(0,100);
    QObject::connect(alphaSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::danmuRender->setOpacity(val/100.f);
        alphaSlider->setToolTip(QString::number(val));
    });
    alphaSlider->setValue(GlobalObjects::appSetting->value("Play/DanmuAlpha",100).toInt());

    QLabel *strokeWidthLabel=new QLabel(tr("Stroke Width"),pageAppearance);
    strokeWidthSlider=new QSlider(Qt::Horizontal,pageAppearance);
    strokeWidthSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    strokeWidthSlider->setRange(0,80);
    QObject::connect(strokeWidthSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::danmuRender->setStrokeWidth(val/10.f);
        strokeWidthSlider->setToolTip(QString::number(val/10.));
    });
    strokeWidthSlider->setValue(GlobalObjects::appSetting->value("Play/DanmuStroke",35).toInt());
    GlobalObjects::danmuRender->setStrokeWidth(strokeWidthSlider->value() / 10.f);

    QLabel *fontSizeLabel=new QLabel(tr("Font Size"),pageAppearance);
    fontSizeSlider=new QSlider(Qt::Horizontal,pageAppearance);
    fontSizeSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    fontSizeSlider->setRange(4,60);
    QObject::connect(fontSizeSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::danmuRender->setFontSize(val);
        fontSizeSlider->setToolTip(QString::number(val));
    });
    fontSizeSlider->setValue(GlobalObjects::appSetting->value("Play/DanmuFontSize",20).toInt());

    glow = new QCheckBox(tr("Glow"),pageAppearance);
    QObject::connect(glow, &QCheckBox::stateChanged, [](int state){
        GlobalObjects::danmuRender->setGlow(state == Qt::Checked);
    });
    glow->setChecked(GlobalObjects::appSetting->value("Play/DanmuGlow", false).toBool());

    bold=new QCheckBox(tr("Bold"),pageAppearance);
    QObject::connect(bold,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuRender->setBold(state==Qt::Checked?true:false);
    });
    bold->setChecked(GlobalObjects::appSetting->value("Play/DanmuBold",false).toBool());

    randomSize=new QCheckBox(tr("Random Size"),pageAppearance);
    QObject::connect(randomSize,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuRender->setRandomSize(state==Qt::Checked?true:false);
    });
    randomSize->setChecked(GlobalObjects::appSetting->value("Play/RandomSize",false).toBool());

    fontFamilyCombo=new QFontComboBox(pageAppearance);
    fontFamilyCombo->setMaximumWidth(160 *logicalDpiX()/96);
    QLabel *fontLabel=new QLabel(tr("Font"),pageAppearance);
    QObject::connect(fontFamilyCombo,&QFontComboBox::currentFontChanged,[](const QFont &newFont){
        GlobalObjects::danmuRender->setFontFamily(newFont.family());
    });
    fontFamilyCombo->setCurrentFont(QFont(GlobalObjects::appSetting->value("Play/DanmuFont","Microsoft Yahei").toString()));

//Advanced Page
    QWidget *pageAdvanced=new QWidget(danmuSettingPage);

    enableAnalyze=new QCheckBox(tr("Enable Danmu Event Analyze"),pageAdvanced);
    enableAnalyze->setChecked(true);
    QObject::connect(enableAnalyze,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuPool->setAnalyzeEnable(state==Qt::Checked?true:false);
    });
    enableAnalyze->setChecked(GlobalObjects::appSetting->value("Play/EnableAnalyze",true).toBool());

    enableMerge=new QCheckBox(tr("Enable Danmu Merge"),pageAdvanced);
    enableMerge->setChecked(true);
    QObject::connect(enableMerge,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuPool->setMergeEnable(state==Qt::Checked?true:false);
    });
    enableMerge->setChecked(GlobalObjects::appSetting->value("Play/EnableMerge",true).toBool());

    enlargeMerged=new QCheckBox(tr("Enlarge Merged Danmu"),pageAdvanced);
    enlargeMerged->setChecked(true);
    QObject::connect(enlargeMerged,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::danmuRender->setEnlargeMerged(state==Qt::Checked?true:false);
    });
    enlargeMerged->setChecked(GlobalObjects::appSetting->value("Play/EnlargeMerged",true).toBool());

    GlobalObjects::danmuPool->setMergeInterval(GlobalObjects::appSetting->value("Play/MergeInterval", 20).toInt() * 1000);
    GlobalObjects::danmuPool->setMaxUnSimCount(GlobalObjects::appSetting->value("Play/MaxDiffCount", 4).toInt());
    GlobalObjects::danmuPool->setMinMergeCount(GlobalObjects::appSetting->value("Play/MinSimCount", 2).toInt());

    QLabel *mergeCountTipLabel=new QLabel(tr("Merge Count Tip Position"),pageAdvanced);
    mergeCountTipPos=new QComboBox(pageAdvanced);
    mergeCountTipPos->addItems(QStringList()<<tr("Hidden")<<tr("Forward")<<tr("Backward"));
    QObject::connect(mergeCountTipPos,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[](int index){
        GlobalObjects::danmuRender->setMergeCountPos(index);
    });
    mergeCountTipPos->setCurrentIndex(GlobalObjects::appSetting->value("Play/MergeCountTip",1).toInt());

    enableLiveMode = new QCheckBox(tr("Enable Live Mode"), pageAdvanced);
    QObject::connect(enableLiveMode,&QCheckBox::stateChanged,[=](int state){
        GlobalObjects::danmuRender->setLiveMode(state == Qt::Checked, true);
        if(GlobalObjects::playlist->getCurrentItem() && !GlobalObjects::mpvplayer->getDanmuHide())
        {
            liveDanmuList->setVisible(state == Qt::Checked);
        }
    });
    enableLiveMode->setChecked(GlobalObjects::appSetting->value("Play/EnableLiveMode", false).toBool());

    liveModeOnlyRolling = new QCheckBox(tr("Live Only Rolling"), pageAdvanced);
    QObject::connect(liveModeOnlyRolling,&QCheckBox::stateChanged,[=](int state){
        GlobalObjects::danmuRender->setLiveMode(enableLiveMode->isChecked(), state == Qt::Checked);
    });
    liveModeOnlyRolling->setChecked(GlobalObjects::appSetting->value("Play/LiveModeOnlyRolling", true).toBool());
    GlobalObjects::danmuRender->setLiveMode(enableLiveMode->isChecked(), liveModeOnlyRolling->isChecked());

    liveDanmuAlignRight = new QCheckBox(tr("Live Align Right"), pageAdvanced);
    QObject::connect(liveDanmuAlignRight,&QCheckBox::stateChanged,[=](int state){
        if(state == Qt::Checked)
        {
            GlobalObjects::danmuRender->liveDanmuModel()->setAlignment(Qt::AlignRight);
            liveDanmuList->parentWidget()->layout()->setAlignment(liveDanmuList, Qt::AlignRight);
        }
        else
        {
            GlobalObjects::danmuRender->liveDanmuModel()->setAlignment(Qt::AlignLeft);
            liveDanmuList->parentWidget()->layout()->setAlignment(liveDanmuList, Qt::AlignLeft);
        }
    });
    liveDanmuAlignRight->setChecked(GlobalObjects::appSetting->value("Play/LiveModeAlignRight", false).toBool());

    liveShowSender = new QCheckBox(tr("Live Show Sender"), pageAdvanced);
    QObject::connect(liveShowSender, &QCheckBox::stateChanged, [=](int state){
        GlobalObjects::danmuRender->liveDanmuModel()->setShowSender(state == Qt::Checked);
    });
    liveShowSender->setChecked(GlobalObjects::appSetting->value("Play/LiveShowSender", true).toBool());
    GlobalObjects::danmuRender->liveDanmuModel()->setShowSender(liveShowSender->isChecked());

    QLabel *liveDanmuSizeLable = new QLabel(tr("Live Danmu Size"), pageAdvanced);
    liveSizeSlider = new QSlider(Qt::Horizontal, pageAdvanced);
    liveSizeSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    liveSizeSlider->setRange(6, 20);
    QObject::connect(liveSizeSlider, &QSlider::valueChanged, [this](int val){
        GlobalObjects::danmuRender->liveDanmuModel()->setFontSize(val);
        liveSizeSlider->setToolTip(QString::number(val));
    });
    liveSizeSlider->setValue(GlobalObjects::appSetting->value("Play/LiveDanmuFontSize", 10).toInt());

    QLabel *liveVRangeLable = new QLabel(tr("Live Vertical Range"), pageAdvanced);
    liveVRangeSlider = new QSlider(Qt::Horizontal, pageAdvanced);
    liveVRangeSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    liveVRangeSlider->setRange(20, 95);
    QObject::connect(liveVRangeSlider, &QSlider::valueChanged, [this](int val){
        liveDanmuList->setFixedHeight(static_cast<float>(val) / 100 * height());
        liveVRangeSlider->setToolTip(QString::number(val));
    });
    liveVRangeSlider->setValue(GlobalObjects::appSetting->value("Play/LiveVRange", 40).toInt());

    QGridLayout *generalGLayout=new QGridLayout(pageGeneral);
    generalGLayout->setContentsMargins(0,0,0,0);
    QHBoxLayout *hideHLayout = new QHBoxLayout();
    hideHLayout->setContentsMargins(0,0,0,0);
    hideHLayout->setSpacing(6*logicalDpiX()/96);
    hideHLayout->addWidget(danmuSwitch);
    hideHLayout->addWidget(hideRollingDanmu);
    hideHLayout->addWidget(hideTopDanmu);
    hideHLayout->addWidget(hideBottomDanmu);
    hideHLayout->addStretch(1);
    generalGLayout->addWidget(hideLabel, 0, 0);
    generalGLayout->addItem(hideHLayout, 0, 1);
    generalGLayout->addWidget(denseLabel, 1, 0);
    generalGLayout->addWidget(denseLevel, 1, 1);
    generalGLayout->addWidget(speedLabel, 2, 0);
    generalGLayout->addWidget(speedSlider, 2, 1);
    generalGLayout->addWidget(maxDanmuCountLabel, 3, 0);
    generalGLayout->addWidget(maxDanmuCount, 3, 1);
    generalGLayout->addWidget(displayAreaLabel, 4, 0);
    generalGLayout->addWidget(displayAreaSlider, 4, 1);
    generalGLayout->addWidget(subProtectLabel, 5, 0);
    QHBoxLayout *subProtectHLayout = new QHBoxLayout();
    subProtectHLayout->setContentsMargins(0,0,0,0);
    subProtectHLayout->setSpacing(6*logicalDpiX()/96);
    subProtectHLayout->addWidget(bottomSubtitleProtect);
    subProtectHLayout->addWidget(topSubtitleProtect);
    subProtectHLayout->addStretch(1);
    generalGLayout->addItem(subProtectHLayout, 5, 1);

    QGridLayout *appearanceGLayout=new QGridLayout(pageAppearance);
    appearanceGLayout->setContentsMargins(0,0,0,0);
    appearanceGLayout->setColumnStretch(0,1);
    appearanceGLayout->setColumnStretch(1,1);
    appearanceGLayout->addWidget(fontLabel,0,0);
    appearanceGLayout->addWidget(fontFamilyCombo,1,0);
    appearanceGLayout->addWidget(fontSizeLabel,2,0);
    appearanceGLayout->addWidget(fontSizeSlider,3,0);
    appearanceGLayout->addWidget(strokeWidthLabel,4,0);
    appearanceGLayout->addWidget(strokeWidthSlider,5,0);
    appearanceGLayout->addWidget(alphaLabel,0,1);
    appearanceGLayout->addWidget(alphaSlider,1,1);
    appearanceGLayout->addWidget(glow,2,1);
    appearanceGLayout->addWidget(bold,3,1);
    appearanceGLayout->addWidget(randomSize,4,1);

    QGridLayout *mergeGLayout=new QGridLayout(pageAdvanced);
    mergeGLayout->setContentsMargins(0,0,0,0);
    mergeGLayout->setColumnStretch(0,1);
    mergeGLayout->setColumnStretch(1,1);
    mergeGLayout->addWidget(enableAnalyze,0,0);
    mergeGLayout->addWidget(enableMerge,1,0);
    mergeGLayout->addWidget(enlargeMerged,2,0);
    mergeGLayout->addWidget(mergeCountTipLabel, 3, 0);
    mergeGLayout->addWidget(mergeCountTipPos, 4, 0);
    mergeGLayout->addWidget(enableLiveMode, 0, 1);
    mergeGLayout->addWidget(liveModeOnlyRolling, 1, 1);
    mergeGLayout->addWidget(liveDanmuAlignRight, 2, 1);
    mergeGLayout->addWidget(liveShowSender, 3, 1);
    mergeGLayout->addWidget(liveDanmuSizeLable, 4, 1);
    mergeGLayout->addWidget(liveSizeSlider, 5, 1);
    mergeGLayout->addWidget(liveVRangeLable, 5, 0);
    mergeGLayout->addWidget(liveVRangeSlider, 6, 0);

    danmuSettingSLayout->setContentsMargins(4,4,4,4);
    danmuSettingSLayout->addWidget(pageGeneral);
    danmuSettingSLayout->addWidget(pageAppearance);
    danmuSettingSLayout->addWidget(pageAdvanced);
    danmuSettingPage->hide();
}

void PlayerWindow::setupPlaySettingPage()
{
    playSettingPage=new QWidget(this->centralWidget());
    playSettingPage->setObjectName(QStringLiteral("PopupPage"));
    playSettingPage->installEventFilter(this);

    QToolButton *playPage=new QToolButton(playSettingPage);
    playPage->setText(tr("Play"));
    playPage->setCheckable(true);
    playPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    playPage->setObjectName(QStringLiteral("DialogPageButton"));
    playPage->setChecked(true);

    QToolButton *behaviorPage=new QToolButton(playSettingPage);
    behaviorPage->setText(tr("Behavior"));
    behaviorPage->setCheckable(true);
    behaviorPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    behaviorPage->setObjectName(QStringLiteral("DialogPageButton"));

    QToolButton *colorPage=new QToolButton(playSettingPage);
    colorPage->setText(tr("Color"));
    colorPage->setCheckable(true);
    colorPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    colorPage->setObjectName(QStringLiteral("DialogPageButton"));

    const int btnH = playPage->fontMetrics().height() + 10*logicalDpiY()/96;
    playPage->setFixedHeight(btnH);
    behaviorPage->setFixedHeight(btnH);
    colorPage->setFixedHeight(btnH);

    QStackedLayout *playSettingSLayout=new QStackedLayout();
    QButtonGroup *btnGroup = new QButtonGroup(this);
    btnGroup->addButton(playPage, 0);
    btnGroup->addButton(behaviorPage, 1);
    btnGroup->addButton(colorPage, 2);
    QObject::connect(btnGroup, (void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled, [=](int id, bool checked) {
        if (checked)
        {
            playSettingSLayout->setCurrentIndex(id);
        }
    });


    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->addWidget(playPage);
    pageButtonHLayout->addWidget(behaviorPage);
    pageButtonHLayout->addWidget(colorPage);

    QVBoxLayout *playSettingVLayout=new QVBoxLayout(playSettingPage);
    playSettingVLayout->addLayout(pageButtonHLayout);
    playSettingVLayout->addLayout(playSettingSLayout);

//Play Page
    QWidget *pagePlay=new QWidget(playSettingPage);

    QLabel *audioTrackLabel=new QLabel(tr("Audio Track"),pagePlay);
    QComboBox *audioTrackCombo=new QComboBox(pagePlay);
    audioTrackCombo->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    QObject::connect(audioTrackCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[audioTrackCombo](int index){
        if(index==audioTrackCombo->count() - 1)
        {
            GlobalObjects::playlist->clearCurrentAudio();
            GlobalObjects::mpvplayer->clearExternalAudio();
            audioTrackCombo->blockSignals(true);
            audioTrackCombo->setCurrentIndex(GlobalObjects::mpvplayer->getCurrentTrack(MPVPlayer::TrackType::AudioTrack));
            audioTrackCombo->blockSignals(false);
        }
        else
        {
            GlobalObjects::mpvplayer->setTrackId(MPVPlayer::TrackType::AudioTrack, index);
            GlobalObjects::playlist->setCurrentAudioIndex(index);
        }
    });
    QPushButton *addAudioTrackButton=new QPushButton(tr("Add"),pagePlay);
    QObject::connect(addAudioTrackButton,&QPushButton::clicked,[this](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        QString file(QFileDialog::getOpenFileName(this,tr("Select Audio File"),"",tr("Audio (%0);;All Files(*)").arg(GlobalObjects::mpvplayer->audioFormats.join(" "))));
        if(!file.isEmpty())
        {
            GlobalObjects::mpvplayer->addAudioTrack(file);
        }
        if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });

    QLabel *subtitleTrackLabel=new QLabel(tr("Subtitle Track"),pagePlay);
    QComboBox *subtitleTrackCombo=new QComboBox(pagePlay);
    subtitleTrackCombo->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    QObject::connect(subtitleTrackCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[subtitleTrackCombo](int index){
        if(index==subtitleTrackCombo->count() - 1)
        {
            GlobalObjects::playlist->clearCurrentSub();
            GlobalObjects::mpvplayer->clearExternalSub();
            subtitleTrackCombo->setCurrentIndex(GlobalObjects::mpvplayer->getCurrentTrack(MPVPlayer::TrackType::SubTrack));
        }
        else if(index==subtitleTrackCombo->count() - 2)
        {
            GlobalObjects::mpvplayer->hideSubtitle(true);
        }
        else
        {
            GlobalObjects::mpvplayer->hideSubtitle(false);
            GlobalObjects::mpvplayer->setTrackId(MPVPlayer::TrackType::SubTrack, index);
            GlobalObjects::playlist->setCurrentSubIndex(index);
        }
    });
    QPushButton *addSubtitleButton=new QPushButton(tr("Add"),pagePlay);
    QObject::connect(addSubtitleButton,&QPushButton::clicked,[this](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        QString file(QFileDialog::getOpenFileName(this,tr("Select Sub File"),"",tr("Subtitle (%0);;All Files(*)").arg(GlobalObjects::mpvplayer->subtitleFormats.join(" "))));
        if(!file.isEmpty())
        {
            GlobalObjects::mpvplayer->addSubtitle(file);
        }
        if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });
    QLabel *subtitleDelayLabel=new QLabel(tr("Subtitle Delay(s)"),pagePlay);
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::trackInfoChange, [=](MPVPlayer::TrackType type){
        if(type == MPVPlayer::TrackType::AudioTrack)
        {
            audioTrackCombo->blockSignals(true);
            audioTrackCombo->clear();
            for(auto &t : GlobalObjects::mpvplayer->getTrackList(MPVPlayer::TrackType::AudioTrack))
            {
                if(t.isExternal && !t.externalFile.isEmpty())
                {
                    GlobalObjects::playlist->addCurrentAudio(t.externalFile);
                }
                audioTrackCombo->addItem(t.title);
            }
            audioTrackCombo->addItem(tr("Clear External Audio Files"));
            audioTrackCombo->setCurrentIndex(GlobalObjects::mpvplayer->getCurrentTrack(MPVPlayer::TrackType::AudioTrack));
            audioTrackCombo->blockSignals(false);
        }
        else if(type == MPVPlayer::TrackType::SubTrack)
        {
            subtitleTrackCombo->blockSignals(true);
            subtitleTrackCombo->clear();
            for(auto &t : GlobalObjects::mpvplayer->getTrackList(MPVPlayer::TrackType::SubTrack))
            {
                if(t.isExternal && !t.externalFile.isEmpty())
                {
                    GlobalObjects::playlist->addCurrentSub(t.externalFile);
                }
                subtitleTrackCombo->addItem(t.title);
            }
            subtitleTrackCombo->addItem(tr("Hide"));
            subtitleTrackCombo->addItem(tr("Clear External Sub Files"));
            subtitleTrackCombo->setCurrentIndex(GlobalObjects::mpvplayer->getCurrentTrack(MPVPlayer::TrackType::SubTrack));
            subtitleTrackCombo->blockSignals(false);
        }
    });
    QSpinBox *delaySpinBox=new QSpinBox(pagePlay);
    delaySpinBox->setRange(INT_MIN,INT_MAX);
    delaySpinBox->setObjectName(QStringLiteral("Delay"));
    delaySpinBox->setAlignment(Qt::AlignCenter);
    QObject::connect(delaySpinBox,&QSpinBox::editingFinished,[delaySpinBox](){
       GlobalObjects::mpvplayer->setSubDelay(delaySpinBox->value());
       GlobalObjects::playlist->setCurrentSubDelay(delaySpinBox->value());
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::subDelayChanged, delaySpinBox, &QSpinBox::setValue);

    QLabel *aspectLabel=new QLabel(tr("Aspect Ratio"),pagePlay);
    aspectRatioCombo=new QComboBox(playSettingPage);
    aspectRatioCombo->addItems(GlobalObjects::mpvplayer->videoAspect);
    QObject::connect(aspectRatioCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[](int index){
        GlobalObjects::mpvplayer->setVideoAspect(index);
    });
    aspectRatioCombo->setCurrentIndex(GlobalObjects::appSetting->value("Play/VidoeAspectRatio",0).toInt());

    QLabel *speedLabel=new QLabel(tr("Play Speed"),pagePlay);
    playSpeedCombo=new QComboBox(pagePlay);
    playSpeedCombo->setInsertPolicy(QComboBox::NoInsert);
	playSpeedCombo->setEditable(true);
    playSpeedCombo->addItems(GlobalObjects::mpvplayer->speedLevel);
    QObject::connect(playSpeedCombo,&QComboBox::currentTextChanged,[](const QString &text){
        bool ok = false;
        double speed = text.toDouble(&ok);
        if(ok) GlobalObjects::mpvplayer->setSpeed(speed);
    });
    playSpeedCombo->setCurrentText(GlobalObjects::appSetting->value("Play/PlaySpeed", "1").toString());
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::speedChanged, [this](double val){
        playSpeedCombo->setCurrentText(QString::number(val));
    });

    QComboBox *mpvParaGroupCombo = new QComboBox(pagePlay);
    mpvParaGroupCombo->setInsertPolicy(QComboBox::NoInsert);
    mpvParaGroupCombo->setEditable(false);
    mpvParaGroupCombo->addItems(GlobalObjects::mpvplayer->allOptionGroups());
    mpvParaGroupCombo->setCurrentText(GlobalObjects::mpvplayer->currentOptionGroup());
    QObject::connect(mpvParaGroupCombo, &QComboBox::currentTextChanged,[](const QString &group){
        if(GlobalObjects::mpvplayer->setOptionGroup(group))
        {
            Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, tr("Switch to option group \"%1\"").arg(GlobalObjects::mpvplayer->currentOptionGroup()));
        }
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::optionGroupChanged, [=](){
        mpvParaGroupCombo->blockSignals(true);
        mpvParaGroupCombo->clear();
        mpvParaGroupCombo->addItems(GlobalObjects::mpvplayer->allOptionGroups());
        mpvParaGroupCombo->setCurrentText(GlobalObjects::mpvplayer->currentOptionGroup());
        mpvParaGroupCombo->blockSignals(false);
    });

    QPushButton *editMpvOptions=new QPushButton(tr("MPV Parameter Settings"), pagePlay);
    QObject::connect(editMpvOptions,&QPushButton::clicked,[this](){
        Settings settings(Settings::PAGE_MPV, this);
        QRect geo(0,0,420*logicalDpiX()/96,600*logicalDpiY()/96);
        geo.moveCenter(this->geometry().center());
        settings.move(mapToGlobal(geo.topLeft()));
        settings.exec();
    });

    QPushButton *showMpvLog=new QPushButton(tr("MPV Log"), pagePlay);
    QObject::connect(showMpvLog,&QPushButton::clicked, this, &PlayerWindow::showMPVLog);

//Behavior Page
    QWidget *pageBehavior=new QWidget(playSettingPage);

    QLabel *clickBehaivorLabel=new QLabel(tr("Click Behavior"),pageBehavior);
    clickBehaviorCombo=new QComboBox(playSettingPage);
    clickBehaviorCombo->addItem(tr("Play/Pause"));
    clickBehaviorCombo->addItem(tr("Show/Hide PlayControl"));
    QObject::connect(clickBehaviorCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[this](int index){
        clickBehavior=index;
        GlobalObjects::appSetting->setValue("Play/ClickBehavior",index);
    });
    clickBehaviorCombo->setCurrentIndex(GlobalObjects::appSetting->value("Play/ClickBehavior",0).toInt());
    clickBehavior=clickBehaviorCombo->currentIndex();

    QLabel *dbClickBehaivorLabel=new QLabel(tr("Double Click Behavior"),pageBehavior);
    dbClickBehaviorCombo=new QComboBox(playSettingPage);
    dbClickBehaviorCombo->addItem(tr("FullScreen"));
    dbClickBehaviorCombo->addItem(tr("Play/Pause"));
    QObject::connect(dbClickBehaviorCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[this](int index){
        dbClickBehaivior=index;
        GlobalObjects::appSetting->setValue("Play/DBClickBehavior",index);
    });
    dbClickBehaviorCombo->setCurrentIndex(GlobalObjects::appSetting->value("Play/DBClickBehavior",0).toInt());
    dbClickBehaivior=dbClickBehaviorCombo->currentIndex();

    QLabel *forwardTimeLabel=new QLabel(tr("Forward Jump Time(s)"),pageBehavior);
    QSpinBox *forwardTimeSpin=new QSpinBox(pageBehavior);
    forwardTimeSpin->setRange(1,20);
    forwardTimeSpin->setAlignment(Qt::AlignCenter);
    forwardTimeSpin->setObjectName(QStringLiteral("Delay"));
    QObject::connect(forwardTimeSpin,&QSpinBox::editingFinished,this,[this,forwardTimeSpin](){
        jumpForwardTime=forwardTimeSpin->value();
        GlobalObjects::appSetting->setValue("Play/ForwardJump",jumpForwardTime);
    });
    forwardTimeSpin->setValue(GlobalObjects::appSetting->value("Play/ForwardJump",5).toInt());
    jumpForwardTime=forwardTimeSpin->value();

    QLabel *backwardTimeLabel=new QLabel(tr("Backward Jump Time(s)"),pageBehavior);
    QSpinBox *backwardTimeSpin=new QSpinBox(pageBehavior);
    backwardTimeSpin->setRange(1,20);
    backwardTimeSpin->setAlignment(Qt::AlignCenter);
    backwardTimeSpin->setObjectName(QStringLiteral("Delay"));
    QObject::connect(backwardTimeSpin,&QSpinBox::editingFinished,this,[this,backwardTimeSpin](){
        jumpBackwardTime=backwardTimeSpin->value();
        GlobalObjects::appSetting->setValue("Play/BackwardJump",jumpBackwardTime);
    });
    backwardTimeSpin->setValue(GlobalObjects::appSetting->value("Play/BackwardJump",5).toInt());
    jumpBackwardTime=backwardTimeSpin->value();

    showPreview = new QCheckBox(tr("Show Preview Over ProgressBar(Need Restart)"), pageBehavior);
    showPreview->setChecked(isShowPreview);

    autoLoadDanmuCheck = new QCheckBox(tr("Auto Load Local Danmu"), pageBehavior);
    QObject::connect(autoLoadDanmuCheck,&QCheckBox::stateChanged,[this](int state){
        autoLoadLocalDanmu = state==Qt::Checked;
    });
    autoLoadLocalDanmu = GlobalObjects::appSetting->value("Play/AutoLoadLocalDanmu", true).toBool();
    autoLoadDanmuCheck->setChecked(autoLoadLocalDanmu);

//Color Page
    QWidget *pageColor=new QWidget(playSettingPage);

    QLabel *brightnessLabel=new QLabel(tr("Brightness"),pageColor);
    brightnessSlider=new QSlider(Qt::Horizontal,pageColor);
    brightnessSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    brightnessSlider->setRange(-100, 100);
    QObject::connect(brightnessSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::mpvplayer->setBrightness(val);
        brightnessSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::brightnessChanged, brightnessSlider, &QSlider::setValue);
    brightnessSlider->setValue(GlobalObjects::appSetting->value("Play/Brightness",0).toInt());

    QLabel *contrastLabel=new QLabel(tr("Contrast"),pageColor);
    contrastSlider=new QSlider(Qt::Horizontal,pageColor);
    contrastSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    contrastSlider->setRange(-100, 100);
    QObject::connect(contrastSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::mpvplayer->setContrast(val);
        contrastSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::contrastChanged, contrastSlider, &QSlider::setValue);
    contrastSlider->setValue(GlobalObjects::appSetting->value("Play/Contrast",0).toInt());

    QLabel *saturationLabel=new QLabel(tr("Saturation"),pageColor);
    saturationSlider=new QSlider(Qt::Horizontal,pageColor);
    saturationSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    saturationSlider->setRange(-100, 100);
    QObject::connect(saturationSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::mpvplayer->setSaturation(val);
        saturationSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::saturationChanged, saturationSlider, &QSlider::setValue);
    saturationSlider->setValue(GlobalObjects::appSetting->value("Play/Saturation",0).toInt());

    QLabel *gammaLabel=new QLabel(tr("Gamma"),pageColor);
    gammaSlider=new QSlider(Qt::Horizontal,pageColor);
    gammaSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    gammaSlider->setRange(-100, 100);
    QObject::connect(gammaSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::mpvplayer->setGamma(val);
        gammaSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::gammaChanged, gammaSlider, &QSlider::setValue);
    gammaSlider->setValue(GlobalObjects::appSetting->value("Play/Gamma",0).toInt());

    QLabel *hueLabel=new QLabel(tr("Hue"),pageColor);
    hueSlider=new QSlider(Qt::Horizontal,pageColor);
    hueSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    hueSlider->setRange(-100, 100);
    QObject::connect(hueSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::mpvplayer->setHue(val);
        hueSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::hueChanged, hueSlider, &QSlider::setValue);
    hueSlider->setValue(GlobalObjects::appSetting->value("Play/Hue",0).toInt());

    QLabel *sharpenLabel=new QLabel(tr("Sharpen"),pageColor);
    sharpenSlider=new QSlider(Qt::Horizontal,pageColor);
    sharpenSlider->setObjectName(QStringLiteral("PopupPageSlider"));
    sharpenSlider->setRange(-200, 200);
    QObject::connect(sharpenSlider,&QSlider::valueChanged,[this](int val){
        GlobalObjects::mpvplayer->setSharpen(val);
        sharpenSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::sharpenChanged, [=](double val){
        sharpenSlider->setValue(val*100);
    });
    sharpenSlider->setValue(GlobalObjects::appSetting->value("Play/Sharpen",0).toInt());

    QPushButton *colorReset = new QPushButton(tr("Reset"), pageColor);
    QObject::connect(colorReset, &QPushButton::clicked, [this](){
        brightnessSlider->setValue(0);
        contrastSlider->setValue(0);
        saturationSlider->setValue(0);
        gammaSlider->setValue(0);
        hueSlider->setValue(0);
        sharpenSlider->setValue(0);
    });

    playSettingSLayout->addWidget(pagePlay);
    playSettingSLayout->addWidget(pageBehavior);
    playSettingSLayout->addWidget(pageColor);
    playSettingSLayout->setContentsMargins(4,4,4,4);

    QGridLayout *playSettingGLayout=new QGridLayout(pagePlay);
    playSettingGLayout->setContentsMargins(0,0,0,0);
    playSettingGLayout->setColumnStretch(1, 1);
    //playSettingGLayout->setRowStretch(0, 1);
    //playSettingGLayout->setRowStretch(7, 1);
    playSettingGLayout->addWidget(audioTrackLabel,1,0);
    playSettingGLayout->addWidget(audioTrackCombo,1,1);
    playSettingGLayout->addWidget(addAudioTrackButton,1,2);
    playSettingGLayout->addWidget(subtitleTrackLabel,2,0);
    playSettingGLayout->addWidget(subtitleTrackCombo,2,1);
    playSettingGLayout->addWidget(addSubtitleButton,2,2);
    playSettingGLayout->addWidget(subtitleDelayLabel,3,0);
    playSettingGLayout->addWidget(delaySpinBox,3,1, 1,2);
    playSettingGLayout->addWidget(aspectLabel,4,0);
    playSettingGLayout->addWidget(aspectRatioCombo,4,1,1,2);
    playSettingGLayout->addWidget(speedLabel,5,0);
    playSettingGLayout->addWidget(playSpeedCombo,5,1,1,2);
    QHBoxLayout *bottomBtnHLayout=new QHBoxLayout();
    bottomBtnHLayout->addWidget(mpvParaGroupCombo);
    bottomBtnHLayout->addWidget(editMpvOptions);
    bottomBtnHLayout->addWidget(showMpvLog);
    playSettingGLayout->addLayout(bottomBtnHLayout,6,0,1,3);


    QGridLayout *appearanceGLayout=new QGridLayout(pageBehavior);
    appearanceGLayout->setContentsMargins(0,0,0,0);
    appearanceGLayout->setColumnStretch(1, 1);
    //appearanceGLayout->setRowStretch(0,1);
    //appearanceGLayout->setRowStretch(6,1);
    appearanceGLayout->addWidget(clickBehaivorLabel,1,0);
    appearanceGLayout->addWidget(clickBehaviorCombo,1,1);
    appearanceGLayout->addWidget(dbClickBehaivorLabel,2,0);
    appearanceGLayout->addWidget(dbClickBehaviorCombo,2,1);
    appearanceGLayout->addWidget(forwardTimeLabel,3,0);
    appearanceGLayout->addWidget(forwardTimeSpin,3,1);
    appearanceGLayout->addWidget(backwardTimeLabel,4,0);
    appearanceGLayout->addWidget(backwardTimeSpin,4,1);
    appearanceGLayout->addWidget(showPreview,5,0,1,2);
    appearanceGLayout->addWidget(autoLoadDanmuCheck,6,0,1,2);


    QGridLayout *colorGLayout=new QGridLayout(pageColor);
    colorGLayout->setContentsMargins(0,0,0,0);
    colorGLayout->setColumnStretch(1, 1);
    //colorGLayout->setRowStretch(0,1);
    //colorGLayout->setRowStretch(8,1);
    colorGLayout->addWidget(brightnessLabel,1,0);
    colorGLayout->addWidget(brightnessSlider,1,1);
    colorGLayout->addWidget(contrastLabel,2,0);
    colorGLayout->addWidget(contrastSlider,2,1);
    colorGLayout->addWidget(saturationLabel,3,0);
    colorGLayout->addWidget(saturationSlider,3,1);
    colorGLayout->addWidget(gammaLabel,4,0);
    colorGLayout->addWidget(gammaSlider,4,1);
    colorGLayout->addWidget(hueLabel,5,0);
    colorGLayout->addWidget(hueSlider,5,1);
    colorGLayout->addWidget(sharpenLabel,6,0);
    colorGLayout->addWidget(sharpenSlider,6,1);
    colorGLayout->addWidget(colorReset,7,0,1,2);

    playSettingPage->hide();
}

void PlayerWindow::setupSignals()
{
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::durationChanged,[this](int duration){
        QCoreApplication::processEvents();
        int ts=duration/1000;
        int lmin=ts/60;
        int ls=ts-lmin*60;
        progress->setRange(0,duration);
        progress->setSingleStep(1);
        miniProgress->setRange(0,duration);
        miniProgress->setSingleStep(1);
        totalTimeStr=QString("/%1:%2").arg(lmin,2,10,QChar('0')).arg(ls,2,10,QChar('0'));
        timeLabel->setText("00:00"+this->totalTimeStr);
        static_cast<DanmuStatisWidget *>(danmuStatisBar)->setDuration(ts);
        const PlayListItem *currentItem=GlobalObjects::playlist->getCurrentItem();
        if(currentItem && currentItem->type == PlayListItem::ItemType::LOCAL_FILE)
        {
            if(currentItem->playTime>15 && currentItem->playTime<ts-15)
            {
                GlobalObjects::mpvplayer->seek(currentItem->playTime*1000);
                showMessage(tr("Jumped to the last play position"));
            }
        }
#ifdef QT_DEBUG
        qDebug()<<"Duration Changed";
#endif
    });
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::fileChanged,[this](){
        const PlayListItem *currentItem=GlobalObjects::playlist->getCurrentItem();
        progress->setEventMark(QVector<DanmuEvent>());
        progress->setChapterMark(QVector<MPVPlayer::ChapterInfo>());
        QString mediaTitle(GlobalObjects::mpvplayer->getMediaTitle());
        if(!currentItem)
        {
            titleLabel->setText(mediaTitle);
            launch->hide();
            GlobalObjects::danmuPool->setPoolID("");
            if(resizePercent!=-1 && !miniModeOn) adjustPlayerSize(resizePercent);
#ifdef QT_DEBUG
            qDebug()<<"File Changed(external item), Current File: " << GlobalObjects::mpvplayer->getCurrentFile();
#endif
            return;
        }
        if(currentItem->animeTitle.isEmpty())
        {
            if(currentItem->type == PlayListItem::ItemType::WEB_URL)
            {
                titleLabel->setText(currentItem->title != currentItem->path? currentItem->title : mediaTitle);
            }
            else
            {
                titleLabel->setText(mediaTitle.isEmpty()?currentItem->title:mediaTitle);
            }
        }
        else
        {
            titleLabel->setText(QString("%1-%2").arg(currentItem->animeTitle,currentItem->title));
        }
        if(currentItem->hasPool())
        {
            GlobalObjects::danmuPool->setPoolID(currentItem->poolID);
            GlobalObjects::danmuProvider->checkSourceToLaunch(currentItem->poolID);
        }
        else
        {
            launch->hide();
            GlobalObjects::danmuPool->setPoolID("");
            if(currentItem->type == PlayListItem::ItemType::LOCAL_FILE)
            {
                showMessage(tr("File is not associated with Danmu Pool"));
            }
        }
        if(autoLoadLocalDanmu && currentItem->type == PlayListItem::ItemType::LOCAL_FILE)
        {
            QString danmuFile(currentItem->path.mid(0, currentItem->path.lastIndexOf('.'))+".xml");
            QFileInfo fi(danmuFile);
            if(fi.exists())
            {
                GlobalObjects::danmuPool->addLocalDanmuFile(danmuFile);
            }
        }
        if(enableLiveMode->isChecked())
        {
            liveDanmuList->show();
        }
        hasExternalAudio = false;
        hasExternalSub = false;
        if(currentItem->trackInfo)
        {
            for(const QString &audioFile : currentItem->trackInfo->audioFiles)
            {
                GlobalObjects::mpvplayer->addAudioTrack(audioFile);
            }
            for(const QString &subFile : currentItem->trackInfo->subFiles)
            {
                GlobalObjects::mpvplayer->addSubtitle(subFile);
            }
            GlobalObjects::mpvplayer->setSubDelay(currentItem->trackInfo->subDelay);
            if(currentItem->trackInfo->audioIndex > -1)
            {
                GlobalObjects::mpvplayer->setTrackId(MPVPlayer::TrackType::AudioTrack, currentItem->trackInfo->audioIndex);
            }
            if(currentItem->trackInfo->subIndex > -1)
            {
                GlobalObjects::mpvplayer->setTrackId(MPVPlayer::TrackType::SubTrack, currentItem->trackInfo->subIndex);
                GlobalObjects::mpvplayer->hideSubtitle(false);
            }
        }
        else
        {
            GlobalObjects::mpvplayer->setSubDelay(0);
        }
        if(resizePercent!=-1 && !miniModeOn) adjustPlayerSize(resizePercent);
#ifdef QT_DEBUG
        qDebug()<<"File Changed,Current Item: "<<currentItem->title;
#endif
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::chapterChanged, [this](){
        progress->setChapterMark(GlobalObjects::mpvplayer->getChapters());
    });
    QObject::connect(GlobalObjects::playlist,&PlayList::currentMatchChanged,[this](){
        const PlayListItem *currentItem=GlobalObjects::playlist->getCurrentItem();
        if(currentItem->animeTitle.isEmpty())
            titleLabel->setText(currentItem->title);
        else
            titleLabel->setText(QString("%1-%2").arg(currentItem->animeTitle, currentItem->title));
    });
    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::eventAnalyzeFinished,progress,&ClickSlider::setEventMark);
    QObject::connect(GlobalObjects::playlist,&PlayList::recentItemsUpdated,[this](){
        static_cast<PlayerContent *>(playerContent)->refreshItems();
    });
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::positionChanged,[this](int newtime){
        int cs=newtime/1000;
        int cmin=cs/60;
        int cls=cs-cmin*60;
        if(!progressPressed) progress->setValue(newtime);
        if(miniModeOn) miniProgress->setValue(newtime);
        timeLabel->setText(QString("%1:%2%3").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')).arg(this->totalTimeStr));
    });
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::bufferingStateChanged,[this](int val){
        timeLabel->setText(tr("Buffering: %1%, %2").arg(val).arg(formatSize(true, GlobalObjects::mpvplayer->getCacheSpeed())));
    });
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::refreshPreview,[this](int timePos, QPixmap *pixmap){
        int cp = progress->curMousePos() / 1000;
        if(!progressInfo->isHidden() && qAbs(timePos-cp)<3)
        {
            previewLabel->resize(pixmap->width(), pixmap->height());
            previewLabel->setPixmap(*pixmap);
            progressInfo->adjustSize();
            timeInfoTip->setMaximumWidth(pixmap->width());
            adjustProgressInfoPos();
            previewLabel->show();
            //previewLabel->adjustSize();
        }
    });
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::stateChanged,[this](MPVPlayer::PlayState state){
        if(GlobalObjects::playlist->getCurrentItem()!=nullptr)
        {
#ifdef Q_OS_WIN
            if(state==MPVPlayer::Play)
            {
                SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
            }
            else
            {
                SetThreadExecutionState(ES_CONTINUOUS);
            }

#endif
            if(onTopWhilePlaying)
            {
                emit setStayOnTop(state==MPVPlayer::Play);
            }
        }
        switch(state)
        {
        case MPVPlayer::Play:
			if (GlobalObjects::playlist->getCurrentItem() != nullptr)
            {
                this->playPause->setText(QChar(0xe6fb));
                playerContent->hide();
            }
            break;
        case MPVPlayer::Pause:
			if (GlobalObjects::playlist->getCurrentItem() != nullptr)
                this->playPause->setText(QChar(0xe606));
            break;
        case MPVPlayer::EndReached:
		{
            this->playPause->setText(QChar(0xe606));
			GlobalObjects::danmuPool->reset();
			GlobalObjects::danmuRender->cleanup();
            //GlobalObjects::playlist->setCurrentPlayTime(0);
            setPlayTime();
			PlayList::LoopMode loopMode = GlobalObjects::playlist->getLoopMode();
			if (loopMode == PlayList::Loop_One)
			{
				GlobalObjects::mpvplayer->seek(0);
				GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
			}
            else if (loopMode != PlayList::NO_Loop_One)
			{
                actNext->trigger();
			}
			break;
		}
        case MPVPlayer::Stop:
        {
            QCoreApplication::processEvents();
            setPlayTime();
            launch->hide();
            playPause->setText(QChar(0xe606));
            progress->setValue(0);
            progress->setRange(0, 0);
            progress->setEventMark(QVector<DanmuEvent>());
            progress->setChapterMark(QVector<MPVPlayer::ChapterInfo>());
            progress->setVisible(true);
            miniProgress->setValue(0);
            miniProgress->setRange(0, 0);
            previewLabel->clear();
            previewLabel->hide();
            progressInfo->adjustSize();
            this->timeLabel->setText("00:00/00:00");
            titleLabel->setText(QString());
            GlobalObjects::playlist->cleanCurrentItem();
            GlobalObjects::danmuPool->cleanUp();
			GlobalObjects::danmuRender->cleanup();			
            GlobalObjects::mpvplayer->update();
            playerContent->raise();
            playerContent->show();
            liveDanmuList->hide();
            break;
        }
        }
    });
    QObject::connect(playPause,&QPushButton::clicked,actPlayPause,&QAction::trigger);

    QObject::connect(stop,&QPushButton::clicked,[](){
        QCoreApplication::processEvents();
        GlobalObjects::mpvplayer->setState(MPVPlayer::Stop);
    });
    QObject::connect(progress,&ClickSlider::sliderClick,[](int pos){
        MPVPlayer::PlayState state=GlobalObjects::mpvplayer->getState();
        switch(state)
        {
        case MPVPlayer::Play:
        case MPVPlayer::Pause:
            GlobalObjects::mpvplayer->seek(pos);
            break;
        default:
            break;
        }
    });
    QObject::connect(progress,&ClickSlider::sliderUp,[this](int pos){
        this->progressPressed=false;
        MPVPlayer::PlayState state=GlobalObjects::mpvplayer->getState();
        switch(state)
        {
        case MPVPlayer::Play:
        case MPVPlayer::Pause:
            GlobalObjects::mpvplayer->seek(pos);
            break;
        default:
            break;
        }
    });
    QObject::connect(progress,&ClickSlider::sliderPressed,[this](){
        this->progressPressed=true;
    });
    QObject::connect(progress,&ClickSlider::sliderReleased,[this](){
        this->progressPressed=false;
    });
    QObject::connect(progress,&ClickSlider::mouseMove,[this](int, int ,int pos, const QString &desc){
        int cs=pos/1000;
        int cmin=cs/60;
        int cls=cs-cmin*60;
        QString timeTip(QString("%1:%2").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')));
        if(!desc.isEmpty())
            timeTip+="\n"+desc;
        timeInfoTip->setText(timeTip);
        timeInfoTip->adjustSize();
        if(previewTimer.isActive()) previewTimer.stop();
        if(isShowPreview && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            previewLabel->clear();
            QPixmap *pixmap = GlobalObjects::mpvplayer->getPreview(cs, false);
            if(pixmap)
            {
                previewLabel->resize(pixmap->width(), pixmap->height());
                previewLabel->setPixmap(*pixmap);
                previewLabel->show();
                timeInfoTip->setMaximumWidth(pixmap->width());
            }
            else
            {
                previewLabel->hide();
                if (GlobalObjects::mpvplayer->isLocalFile())
                {
                    previewTimer.start(80);
                }
            }
        }
        progressInfo->adjustSize();
        adjustProgressInfoPos();
        progressInfo->show();
        progressInfo->raise();
    });
    QObject::connect(progress,&ClickSlider::mouseEnter,[this](){
		if (GlobalObjects::playlist->getCurrentItem() != nullptr && GlobalObjects::danmuPool->totalCount()>0)
			danmuStatisBar->show();
    });
    QObject::connect(progress,&ClickSlider::mouseLeave,[this](){
        danmuStatisBar->hide();
        progressInfo->hide();
    });
    QObject::connect(miniProgress,&ClickSlider::sliderClick,[](int pos){
        MPVPlayer::PlayState state=GlobalObjects::mpvplayer->getState();
        switch(state)
        {
        case MPVPlayer::Play:
        case MPVPlayer::Pause:
            GlobalObjects::mpvplayer->seek(pos);
            break;
        default:
            break;
        }
    });
    QObject::connect(volume,&QSlider::valueChanged,[this](int val){
        GlobalObjects::mpvplayer->setVolume(val);
        volume->setToolTip(QString::number(val));
    });
    volume->setValue(GlobalObjects::appSetting->value("Play/Volume", 50).toInt());
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::volumeChanged, [this](int value){
        volume->setValue(value);
    });

    QObject::connect(mute,&QPushButton::clicked,[this](){
        if(GlobalObjects::mpvplayer->getMute())
        {
            GlobalObjects::mpvplayer->setMute(false);
            this->mute->setText(QChar(0xe62c));
        }
        else
        {
            GlobalObjects::mpvplayer->setMute(true);
            this->mute->setText(QChar(0xe61e));
        }
    });
    if(GlobalObjects::appSetting->value("Play/Mute",false).toBool())
        mute->click();
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::muteChanged, [this](bool mute){
        if(mute)
        {
            this->mute->setText(QChar(0xe61e));
        }
        else
        {
            this->mute->setText(QChar(0xe62c));
        }
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::seekableChanged, [this](bool seekable){
        this->progress->setVisible(seekable);
        actSnippet->setEnabled(seekable);
        actGIF->setEnabled(seekable);
        if(!seekable) {
            progressInfo->hide();
        }
        if(this->miniModeOn && this->miniProgress->isVisible() && !seekable) {
            this->miniProgress->setVisible(false);
        }
    });


    QObject::connect(launch, &QPushButton::clicked, [this]() {
        launchWindow->exec();
    });
    QObject::connect(GlobalObjects::danmuProvider, &DanmuProvider::sourceCheckDown, this, [=](const QString &poolId, const QStringList &supportedScripts){
        const PlayListItem *currentItem=GlobalObjects::playlist->getCurrentItem();
        if(currentItem && currentItem->hasPool() && poolId == currentItem->poolID && supportedScripts.size()>0)
        {
            launch->show();
        }
        else
        {
            launch->hide();
        }
    });

	QObject::connect(danmu, &QPushButton::clicked, [this]() {
        if (!playSettingPage->isHidden() ||!danmuSettingPage->isHidden())
		{
            playSettingPage->hide();
			danmuSettingPage->hide();
			return;
		}
        danmuSettingPage->resize(danmuSettingPage->layout()->sizeHint() + QSize(40*logicalDpiX()/96, 0));
        danmuSettingPage->show();
		danmuSettingPage->raise();
        QPoint leftTop(width() - danmuSettingPage->width() - 10, height() - playControlPanel->height() - danmuSettingPage->height() - 2);
        danmuSettingPage->move(leftTop);
    });
    QObject::connect(setting,&QPushButton::clicked,[this](){
        if (!playSettingPage->isHidden() || !danmuSettingPage->isHidden())
		{
			playSettingPage->hide();
            danmuSettingPage->hide();
			return;
		}
        playSettingPage->resize(playSettingPage->layout()->sizeHint() + QSize(40*logicalDpiX()/96, 0));
        playSettingPage->show();
		playSettingPage->raise();
        QPoint leftTop(width() - playSettingPage->width() - 10, height() - playControlPanel->height() - playSettingPage->height() - 2);
        playSettingPage->move(leftTop);
    });
    QObject::connect(next,&QPushButton::clicked,actNext,&QAction::trigger);
    QObject::connect(prev,&QPushButton::clicked,actPrev,&QAction::trigger);
    QObject::connect(fullscreen,&QPushButton::clicked,actFullscreen,&QAction::trigger);

    QObject::connect(playListCollapseButton,&QPushButton::clicked,this,[this](){
        if(playListCollapseButton->text()==QChar(0xe946))
            playListCollapseButton->setText(QChar(0xe945));
        else
            playListCollapseButton->setText(QChar(0xe946));
        emit toggleListVisibility();
    });

    QObject::connect(mediaInfo,&QToolButton::clicked,[this](){
		if (GlobalObjects::playlist->getCurrentItem() == nullptr)return;
        MediaInfo mediaInfoDialog(this);
        QRect geo(0,0,400*logicalDpiX()/96,600*logicalDpiY()/96);
        geo.moveCenter(this->geometry().center());
        mediaInfoDialog.move(mapToGlobal(geo.topLeft()));
        mediaInfoDialog.exec();
    });
    QObject::connect(GlobalObjects::danmuRender->liveDanmuModel(), &LiveDanmuListModel::danmuAppend, this, [this](){
        SmoothScrollBar *liveDanmuVScroll = static_cast<SmoothScrollBar*>(liveDanmuList->verticalScrollBar());
        liveDanmuVScroll->setValue(liveDanmuVScroll->maximum());
        liveDanmuList->graphicsEffect()->setEnabled(liveDanmuVScroll->value() > 0);
    }, Qt::QueuedConnection);
}

void PlayerWindow::adjustPlayerSize(int percent)
{
    MPVPlayer::VideoSizeInfo videoSize=GlobalObjects::mpvplayer->getVideoSizeInfo();
    if(videoSize.width == 0 || videoSize.height == 0)return;
    double aspectRatio;
    if(videoSize.dwidth == 0 || videoSize.dheight == 0)
        aspectRatio = double(videoSize.width)/videoSize.height;
    else
        aspectRatio = double(videoSize.dwidth)/videoSize.dheight;
    double scale = percent / 100.0;
    emit resizePlayer(videoSize.width * scale,videoSize.height * scale,aspectRatio);
}

void PlayerWindow::setPlayTime()
{
    int playTime=progress->value()/1000;
    GlobalObjects::playlist->setCurrentPlayTime(playTime);
}

void PlayerWindow::showMessage(const QString &msg, int flag)
{
    Q_UNUSED(flag)
    showMessage(msg, "");
}

void PlayerWindow::showMessage(const QString &msg, const QString &type)
{
    int y = height() - 20*logicalDpiY()/96 - (miniModeOn? miniProgress->height() : playControlPanel->height());
    static_cast<InfoTips *>(playInfo)->setBottom(y);
    static_cast<InfoTips *>(playInfo)->showMessage(msg, type);
}

void PlayerWindow::switchItem(bool prev, const QString &nullMsg)
{
    setPlayTime();
    while(true)
    {
        const PlayListItem *item = GlobalObjects::playlist->playPrevOrNext(prev);
        if (item)
        {
            if(item->type == PlayListItem::ItemType::LOCAL_FILE)
            {
                QFileInfo fi(item->path);
                if(!fi.exists())
                {
                    showMessage(tr("File not exist: %0").arg(item->title));
                    continue;
                }
            }
            QCoreApplication::processEvents();
            GlobalObjects::danmuPool->reset();
            GlobalObjects::danmuRender->cleanup();
            GlobalObjects::mpvplayer->setMedia(item->path);
            break;
        }
        else
        {
            showMessage(nullMsg);
            break;
        }
    }
}

void PlayerWindow::adjustProgressInfoPos()
{
    int ty=danmuStatisBar->isHidden()?height()-playControlPanel->height()-progressInfo->height():
                                      height()-playControlPanel->height()-progressInfo->height()-statisBarHeight;
    int nx = progress->curMouseX()-progressInfo->width()/3;
    if(nx+progressInfo->width()>width()) nx = width()-progressInfo->width();
    progressInfo->move(nx<0?0:nx,ty);
}

void PlayerWindow::exitMiniMode()
{
    miniModeOn = false;
    miniProgress->hide();
    emit miniMode(false);
}

void PlayerWindow::setCentralWidget(QWidget *widget)
{
    QHBoxLayout *cHBoxLayout = new QHBoxLayout(this);
    cHBoxLayout->setContentsMargins(0,0,0,0);
    cHBoxLayout->addWidget(widget);
    cWidget = widget;
}

void PlayerWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(miniModeOn)
    {
        if(mouseLPressed && pressPos!=event->globalPos())
        {
            emit moveWindow(event->globalPos());
            event->accept();
            moving = true;
        }
        else if(GlobalObjects::mpvplayer->getSeekable())
        {
            if(height()-event->pos().y()<miniProgress->height()+10)
            {
                miniProgress->show();
            }
            else
            {
                miniProgress->hide();
            }
        }
        return;
    }
    if(!autoHideControlPanel) return;
    if(isFullscreen)
    {
        setCursor(Qt::ArrowCursor);
        hideCursorTimer.start(hideCursorTimeout);
    }
    if(clickBehavior==1)return;
    const QPoint pos=event->pos();
    if(height()-pos.y()<playControlPanel->height()+40)
    {
        playControlPanel->show();
        playInfoPanel->show();
        hideCursorTimer.stop();
    }
    else if(pos.y()<playInfoPanel->height())
    {
         playInfoPanel->show();
         playControlPanel->hide();
         hideCursorTimer.stop();
    } 
    else
    {
        playInfoPanel->hide();
        playControlPanel->hide();
        danmuStatisBar->hide();
    }
    if(this->width()-pos.x()<playlistCollapseWidth+20)
    {
        playListCollapseButton->show();
    }
    else
    {
        playListCollapseButton->hide();
    }
}

void PlayerWindow::mouseDoubleClickEvent(QMouseEvent *)
{
    if(miniModeOn)
    {
        exitMiniMode();
        return;
    }
    dbClickBehaivior==0?actFullscreen->trigger():actPlayPause->trigger();
}

void PlayerWindow::mousePressEvent(QMouseEvent *event)
{
    mouseLPressed = miniModeOn && event->button()==Qt::LeftButton;
    if(mouseLPressed)
    {
        pressPos = event->globalPos();
        emit beforeMove(pressPos);
    }
}

void PlayerWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(miniModeOn && event->button()==Qt::LeftButton)
    {
        if(mouseLPressed && moving)
            moving = false;
        else
            actPlayPause->trigger();
        mouseLPressed = false;
        return;
    }
    if(event->button()==Qt::LeftButton)
    {
        if(!danmuSettingPage->isHidden()||!playSettingPage->isHidden())
        {
            if(!danmuSettingPage->underMouse())
                danmuSettingPage->hide();
            if(!playSettingPage->underMouse())
                playSettingPage->hide();
        }
        else
        {
            if(clickBehavior==1)
            {
                if(playControlPanel->isHidden())
                {
                    playControlPanel->show();
                    playInfoPanel->show();
                    playListCollapseButton->show();
                }
                else
                {
                    playInfoPanel->hide();
                    playControlPanel->hide();
                    playListCollapseButton->hide();
                }
            }
            else
            {
                actPlayPause->trigger();
            }
        }
    }
}

void PlayerWindow::resizeEvent(QResizeEvent *)
{
    playInfoPanel->setMaximumWidth(width());
    playListCollapseButton->setGeometry(width()-playlistCollapseWidth,(height()-playlistCollapseHeight)/2,playlistCollapseWidth,playlistCollapseHeight);
    if(!playSettingPage->isHidden())
    {
        playSettingPage->resize(playSettingPage->layout()->sizeHint() + QSize(40*logicalDpiX()/96, 0));
        QPoint leftTop(width() - playSettingPage->width() - 10, height() - playControlPanel->height() - playSettingPage->height() - 2);
        playSettingPage->move(leftTop);
    }
	if (!danmuSettingPage->isHidden())
	{
        danmuSettingPage->resize(danmuSettingPage->layout()->sizeHint() + QSize(40*logicalDpiX()/96, 0));
        QPoint leftTop(width() - danmuSettingPage->width() - 10, height() - playControlPanel->height() - danmuSettingPage->height() - 2);
		danmuSettingPage->move(leftTop);
	}
    playerContent->resize(400*logicalDpiX()/96, 400*logicalDpiY()/96);
    playerContent->move((width()-playerContent->width())/2,(height()-playerContent->height())/2);
    int y = height() - 20*logicalDpiY()/96 - (miniModeOn? miniProgress->height() : playControlPanel->height());
    static_cast<InfoTips *>(playInfo)->setBottom(y);
    static_cast<InfoTips *>(playInfo)->updatePosition();
    liveDanmuList->setFixedSize(width() * 0.45, height() * static_cast<float>(liveVRangeSlider->value()) / 100);
    liveDanmuList->scrollToBottom();
}

void PlayerWindow::leaveEvent(QEvent *)
{
    if(autoHideControlPanel)
    {
        QTimer::singleShot(500, [this](){
            this->playControlPanel->hide();
            this->playInfoPanel->hide();
            this->playListCollapseButton->hide();
            this->danmuStatisBar->hide();
        });
    }
    if(miniModeOn && !miniProgress->isHidden())
    {
        QTimer::singleShot(500, [this](){
            miniProgress->hide();
        });
    }
}

bool PlayerWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(watched==danmuSettingPage || watched==playSettingPage)
    {
        if (event->type() == QEvent::Show)
        {
            autoHideControlPanel=false;
            return true;
        }
        else if(event->type() == QEvent::Hide)
        {
            autoHideControlPanel=true;
            return true;
        }
        return false;
    }
    return QWidget::eventFilter(watched,event);
}

void PlayerWindow::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    switch (key)
    {
    case Qt::Key_Control:
    {
        if(altPressCount>0)
        {
            altPressCount=0;
            doublePressTimer.stop();
        }
        if(!doublePressTimer.isActive())
        {
            doublePressTimer.start(500);
        }
        ctrlPressCount++;
        if(ctrlPressCount==2)
        {
            danmuSwitch->click();
            doublePressTimer.stop();
            ctrlPressCount=0;
        }
        break;
    }
    case Qt::Key_Alt:
    {
        if(ctrlPressCount>0)
        {
            ctrlPressCount=0;
            doublePressTimer.stop();
        }
        if(!doublePressTimer.isActive())
        {
            doublePressTimer.start(500);
        }
        altPressCount++;
        if(altPressCount==2)
        {
            doublePressTimer.stop();
            altPressCount=0;
            const PlayListItem *curItem=GlobalObjects::playlist->getCurrentItem();
            if(curItem && !curItem->animeTitle.isEmpty())
            {
                int curTime=GlobalObjects::mpvplayer->getTime();
                int cmin=curTime/60;
                int cls=curTime-cmin*60;
                QString info=QString("%1:%2 - %3").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')).arg(curItem->title);
                QTemporaryFile tmpImg("XXXXXX.jpg");
                if(tmpImg.open())
                {
                    GlobalObjects::mpvplayer->screenshot(tmpImg.fileName());
                    QImage captureImage(tmpImg.fileName());
                    AnimeWorker::instance()->saveCapture(curItem->animeTitle, info, captureImage);
                    showMessage(tr("Capture has been add to library: %1").arg(curItem->animeTitle));
                }
            }
            else
            {
                actScreenshotSrc->trigger();
            }
        }
        break;
    }
    case Qt::Key_Space:
        actPlayPause->trigger();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if(miniModeOn)
        {
            exitMiniMode();
            break;
        }
        actFullscreen->trigger();
        break;
    case Qt::Key_Escape:
        if(isFullscreen)
            actFullscreen->trigger();
        else
            miniModeOn?exitMiniMode():actMiniMode->trigger();
        break;
    case Qt::Key_Down:
    case Qt::Key_Up:
        QApplication::sendEvent(volume, event);
        showMessage(tr("Volume: %0").arg(volume->value()), "playerInfo");
        break;
    case Qt::Key_Right:
        if (event->modifiers() == Qt::ControlModifier)
        {
            GlobalObjects::mpvplayer->frameStep();
            showMessage(tr("Frame Step:Forward"), "playerInfo");
        }
        else
            GlobalObjects::mpvplayer->seek(jumpForwardTime, true);
        break;
    case Qt::Key_Left:
        if (event->modifiers() == Qt::ControlModifier)
        {
            GlobalObjects::mpvplayer->frameStep(false);
            showMessage(tr("Frame Step:Backward"), "playerInfo");
        }
        else
            GlobalObjects::mpvplayer->seek(-jumpBackwardTime, true);
        break;
    case Qt::Key_PageUp:
        actPrev->trigger();
        break;
    case Qt::Key_PageDown:
        actNext->trigger();
        break;
    case Qt::Key_F5:
        emit refreshPool();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
    if(event->key()==Qt::Key_Control || event->key()==Qt::Key_Shift || event->key()==Qt::Key_Alt)
        return;
    QString pressKeyStr = QKeySequence(event->modifiers()|event->key()).toString();
    if(GlobalObjects::mpvplayer->enableDirectKey())
    {
        GlobalObjects::mpvplayer->runShortcut(pressKeyStr.toLower(), 1);
    }
    else
    {
        const auto &shortcuts = GlobalObjects::mpvplayer->getShortcuts();
        if(shortcuts.contains(pressKeyStr))
        {
            int ret = GlobalObjects::mpvplayer->runShortcut(pressKeyStr);
            showMessage(tr("%1 Command: %2, ret = %3").arg(pressKeyStr, shortcuts[pressKeyStr].second).arg(ret));
        }
    }
}

void PlayerWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key()==Qt::Key_Control || event->key()==Qt::Key_Shift || event->key()==Qt::Key_Alt)
    {
        QWidget::keyReleaseEvent(event);
        return;
    }
    QString pressKeyStr = QKeySequence(event->modifiers()|event->key()).toString();
    if(GlobalObjects::mpvplayer->enableDirectKey())
    {
        GlobalObjects::mpvplayer->runShortcut(pressKeyStr.toLower(), 2);
    }
    else
    {
        QWidget::keyReleaseEvent(event);
    }
}

void PlayerWindow::contextMenuEvent(QContextMenuEvent *)
{
    currentDanmu = GlobalObjects::danmuRender->danmuAt(mapFromGlobal(QCursor::pos()));
    if(!currentDanmu && liveDanmuList->isVisible())
    {
        QModelIndex index = liveDanmuList->indexAt(liveDanmuList->mapFromGlobal(QCursor::pos()));
        currentDanmu = GlobalObjects::danmuRender->liveDanmuModel()->getDanmu(index);
    }
    if(currentDanmu.isNull()) return;
    ctxText->setText(currentDanmu->text);
    ctxBlockUser->setText(tr("Block User %1").arg(currentDanmu->sender));
    contexMenu->exec(QCursor::pos());
}

void PlayerWindow::closeEvent(QCloseEvent *)
{
    setPlayTime();
    GlobalObjects::appSetting->beginGroup("Play");
    GlobalObjects::appSetting->setValue("ShowPreview",showPreview->isChecked());
    GlobalObjects::appSetting->setValue("AutoLoadLocalDanmu",autoLoadDanmuCheck->isChecked());    
    GlobalObjects::appSetting->setValue("EnableLiveMode", enableLiveMode->isChecked());
    GlobalObjects::appSetting->setValue("LiveModeOnlyRolling", liveModeOnlyRolling->isChecked());
    GlobalObjects::appSetting->setValue("LiveModeAlignRight", liveDanmuAlignRight->isChecked());
    GlobalObjects::appSetting->setValue("LiveShowSender", liveShowSender->isChecked());
    GlobalObjects::appSetting->setValue("LiveDanmuFontSize", liveSizeSlider->value());
    GlobalObjects::appSetting->setValue("LiveVRange", liveVRangeSlider->value());
    GlobalObjects::appSetting->setValue("HideDanmu",danmuSwitch->isChecked());
    GlobalObjects::appSetting->setValue("HideRolling",hideRollingDanmu->isChecked());
    GlobalObjects::appSetting->setValue("HideTop",hideTopDanmu->isChecked());
    GlobalObjects::appSetting->setValue("HideBottom",hideBottomDanmu->isChecked());
    GlobalObjects::appSetting->setValue("DanmuSpeed",speedSlider->value());
    GlobalObjects::appSetting->setValue("DanmuAlpha",alphaSlider->value());
    GlobalObjects::appSetting->setValue("DanmuStroke",strokeWidthSlider->value());
    GlobalObjects::appSetting->setValue("DanmuFontSize",fontSizeSlider->value());
    GlobalObjects::appSetting->setValue("DanmuBold",bold->isChecked());
    GlobalObjects::appSetting->setValue("DanmuGlow", glow->isChecked());
    GlobalObjects::appSetting->setValue("BottomSubProtect",bottomSubtitleProtect->isChecked());
    GlobalObjects::appSetting->setValue("TopSubProtect",topSubtitleProtect->isChecked());
    GlobalObjects::appSetting->setValue("RandomSize",randomSize->isChecked());
    GlobalObjects::appSetting->setValue("DanmuFont",fontFamilyCombo->currentFont().family());
    GlobalObjects::appSetting->setValue("VidoeAspectRatio",aspectRatioCombo->currentIndex());
    GlobalObjects::appSetting->setValue("PlaySpeed",playSpeedCombo->currentText());
    GlobalObjects::appSetting->setValue("WindowSize",windowSizeGroup->actions().indexOf(windowSizeGroup->checkedAction()));
    GlobalObjects::appSetting->setValue("StayOnTop",stayOnTopGroup->actions().indexOf(stayOnTopGroup->checkedAction()));
    GlobalObjects::appSetting->setValue("Volume",volume->value());
    GlobalObjects::appSetting->setValue("Mute",GlobalObjects::mpvplayer->getMute());
    GlobalObjects::appSetting->setValue("MaxCount",maxDanmuCount->value());
    GlobalObjects::appSetting->setValue("Dense",denseLevel->value());
    GlobalObjects::appSetting->setValue("DisplayArea",displayAreaSlider->value());
    GlobalObjects::appSetting->setValue("EnableMerge",enableMerge->isChecked());
    GlobalObjects::appSetting->setValue("EnableAnalyze", enableAnalyze->isChecked());
    GlobalObjects::appSetting->setValue("EnlargeMerged",enlargeMerged->isChecked());
    GlobalObjects::appSetting->setValue("MergeCountTip",mergeCountTipPos->currentIndex());
    GlobalObjects::appSetting->setValue("Brightness",brightnessSlider->value());
    GlobalObjects::appSetting->setValue("Contrast",contrastSlider->value());
    GlobalObjects::appSetting->setValue("Saturation",saturationSlider->value());
    GlobalObjects::appSetting->setValue("Gamma",gammaSlider->value());
    GlobalObjects::appSetting->setValue("Hue",hueSlider->value());
    GlobalObjects::appSetting->setValue("Sharpen",sharpenSlider->value());
    GlobalObjects::appSetting->endGroup();
}

void PlayerWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void PlayerWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls(event->mimeData()->urls());
    QStringList fileList,dirList;
    for(QUrl &url:urls)
    {
        if(url.isLocalFile())
        {
            QFileInfo fi(url.toLocalFile());
            if(fi.isFile())
            {
                if(GlobalObjects::mpvplayer->videoFileFormats.contains("*."+fi.suffix().toLower()))
                {
                    fileList.append(fi.filePath());
                }
                else if(GlobalObjects::mpvplayer->subtitleFormats.contains("*."+fi.suffix().toLower()))
                {
                    if(GlobalObjects::playlist->getCurrentItem()!=nullptr)
                    {
                        GlobalObjects::mpvplayer->addSubtitle(fi.filePath());
                        showMessage(tr("Subtitle has been added"));
                    }
                }
                else if("xml"==fi.suffix())
                {
					QVector<DanmuComment *> tmplist;
                    LocalProvider::LoadXmlDanmuFile(fi.filePath(),tmplist);
                    DanmuSource sourceInfo;
                    sourceInfo.scriptData = fi.filePath();
                    sourceInfo.title=fi.fileName();
                    sourceInfo.count=tmplist.count();
                    Pool *pool=GlobalObjects::danmuPool->getPool();
                    if(pool->addSource(sourceInfo,tmplist,true)>=0)
                        showMessage(tr("Danmu has been added"));
                    else
                    {
                        qDeleteAll(tmplist);
                        showMessage(tr("Add Faied: Pool is busy"));
                    }
                }
            }
            else
            {
                dirList.append(fi.filePath());
            }
        }
    }
    if(!fileList.isEmpty())
    {
        GlobalObjects::playlist->addItems(fileList,QModelIndex());
        const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(fileList.first());
        if (curItem)
        {
            GlobalObjects::danmuPool->reset();
            GlobalObjects::danmuRender->cleanup();
            GlobalObjects::mpvplayer->setMedia(curItem->path);
        }

    }
    for(QString dir:dirList)
    {
        GlobalObjects::playlist->addFolder(dir,QModelIndex());
    }
}

void PlayerWindow::wheelEvent(QWheelEvent *event)
{
	int val = volume->value();
    if ((val > 0 && val < volume->maximum()) || (val == 0 && event->angleDelta().y()>0) ||
            (val == volume->maximum() && event->angleDelta().y() < 0))
	{
		static bool inProcess = false;
		if (!inProcess)
		{
			inProcess = true;
			QApplication::sendEvent(volume, event);
			inProcess = false;
		}
	}
    showMessage(tr("Volume: %0").arg(volume->value()), "playerInfo");
}


#include "player.h"
#include <QVBoxLayout>
#include <QStackedLayout>
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
#include <QLineEdit>

#include "Play/Danmu/Manager/danmumanager.h"
#include "UI/dialogs/mpvconfediror.h"
#include "UI/ela/ElaMenu.h"
#include "UI/inputdialog.h"
#include "UI/widgets/colorpicker.h"
#include "UI/widgets/elidedlabel.h"
#include "UI/widgets/klineedit.h"
#include "qwidgetaction.h"
#include "widgets/clickslider.h"
#include "widgets/danmustatiswidget.h"
#include "widgets/smoothscrollbar.h"
#include "capture.h"
#include "mediainfo.h"
#include "settings.h"
#include "snippetcapture.h"
#include "gifcapture.h"
#include "Play/playcontext.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/Render/danmurender.h"
#include "Play/Danmu/Render/livedanmulistmodel.h"
#include "Play/Danmu/Render/livedanmuitemdelegate.h"
#include "Play/Danmu/Provider/localprovider.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Danmu/blocker.h"
#include "MediaLibrary/animeworker.h"
#include "Download/util.h"
#include "globalobjects.h"
#include "Common/eventbus.h"
#include "Common/keyactionmodel.h"
#include "widgets/optionmenu.h"
#include "ela/ElaToolButton.h"
#include "ela/ElaSpinBox.h"
#include "ela/ElaSlider.h"
#include "ela/ElaToggleSwitch.h"
#include "ela/ElaTheme.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#ifdef KSERVICE
#include "Service/kservice.h"
#endif

#define SETTING_KEY_FORWARD_JUMP "Play/ForwardJump"
#define SETTING_KEY_BACKWARD_JUMP "Play/BackwardJump"
#define SETTING_KEY_CUSTOM_PLAYBACK_RATE "Play/CustomPlaybackRate"
#define SETTING_KEY_LAUNCH_CUSTOM_COLOR "Play/LaunchCustomColor"

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
    void showMessage(const QString &msg, const QString &type = "", int timeout = 2500)
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
        if (!type.isEmpty() && msg.isEmpty())
        {
            if (targetInfoTip) targetInfoTip->timeout = 0;
            return;
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
    const int timerInterval = 500;
    QTimer hideTimer;
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
        composionImg.setDevicePixelRatio(source.devicePixelRatio());
        composionImg.fill(0);
        QLinearGradient fillGradient;
        fillGradient.setColorAt(0, QColor(0, 0, 0, 0));
        fillGradient.setColorAt(1, QColor(0, 0, 0, 255));
        fillGradient.setStart(0, 0);
        fillGradient.setFinalStop(0, qMin(fadeHeight, composionImg.height() * 0.1));
        QPainter tp(&composionImg);
        tp.setRenderHints(QPainter::Antialiasing);
        tp.fillRect(composionImg.rect(), QBrush(fillGradient));
        tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
        tp.drawPixmap(0, 0, source);
        tp.end();
        painter->drawImage(0, 0, composionImg);
    }
};

class SidePanel : public QWidget
{
public:
    using QWidget::QWidget;
protected:
    virtual void mouseMoveEvent(QMouseEvent *event) override
    {
        const QPoint pos = event->pos();
        if (pos.y() < height() * 0.2 || pos.y() > height() * 0.8)
        {
            if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty()) hide();
        }
        QWidget::mouseMoveEvent(event);
    }
};

}

PlayerWindow::PlayerWindow(QWidget *parent) : QWidget(parent)
{
    Notifier::getNotifier()->addNotify(Notifier::PLAYER_NOTIFY, this);
    setWindowFlags(Qt::FramelessWindowHint);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setMouseTracking(true);
    setCentralWidget(centralWidget);
    setContentsMargins(0, 0, 0, 0);

    initActions();
    initSettings();

    QWidget *playerLayer = initPlayerLayer(centralWidget);
    QWidget *liveDanmuLayer = initLiveDanmuLayer(centralWidget);
    GlobalObjects::mpvplayer->setParent(centralWidget);
    GlobalObjects::mpvplayer->setIccProfileOption();

    QStackedLayout *playerSLayout = new QStackedLayout(centralWidget);
    playerSLayout->setStackingMode(QStackedLayout::StackAll);
    playerSLayout->setContentsMargins(0,0,0,0);
    playerSLayout->setSpacing(0);
    playerSLayout->addWidget(playerLayer);
    playerSLayout->addWidget(liveDanmuLayer);
    playerSLayout->addWidget(GlobalObjects::mpvplayer);

    initSignals();
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    showPlayerRegion(false);
}

void PlayerWindow::toggleListCollapseState(int listType)
{
    const QVector<QPushButton *> sideBtns = { playlistBtn, danmuBtn, subBtn };
    for (QPushButton *btn : sideBtns)
    {
        btn->setChecked(false);
    }
    if (listType >= 0 && listType < sideBtns.size())
    {
        sideBtns[listType]->setChecked(true);
    }
}

void PlayerWindow::toggleFullScreenState(bool on)
{
    if (miniModeOn)
    {
        exitMiniMode();
        return;
    }
    isFullscreen=on;
    if(isFullscreen) fullscreen->setText(QChar(0xe82b));
    else fullscreen->setText(QChar(0xe649));
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
    actMiniMode = new QAction(tr("Mini Mode"), this);
    QObject::connect(actMiniMode, &QAction::triggered, this, [=](){
        if (miniModeOn)
        {
            exitMiniMode();
            return;
        }
        if (isFullscreen)
        {
            toggleFullScreenState(!isFullscreen);
            emit enterFullScreen(isFullscreen);
            return;
        }
        if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty() && !miniModeOn)
        {
            if (isFullscreen)
            {
                toggleFullScreenState(!isFullscreen);
                emit enterFullScreen(isFullscreen);
            }
            miniModeOn = true;
            playInfoPanel->hide();
            playControlPanel->hide();
            if (volumeSettingPage) volumeSettingPage->hide();
            if (playInfoPage) playInfoPage->hide();
            if (sidePanel) sidePanel->hide();
            if (danmuSettingPage) danmuSettingPage->hide();
            if (playSettingPage) playSettingPage->hide();
            emit miniMode(true);
        }
    });

    actScreenshotSrc = new QAction(tr("Original Video"), this);
    QObject::connect(actScreenshotSrc, &QAction::triggered, this, [=](){
        QTemporaryFile tmpImg("XXXXXX.jpg");
        if (tmpImg.open())
        {
            GlobalObjects::mpvplayer->screenshot(tmpImg.fileName());
            QImage captureImage(tmpImg.fileName());
            const PlayListItem *curItem = GlobalObjects::playlist->getCurrentItem();
            Capture captureDialog(captureImage,playPause,curItem);
            QRect geo(captureDialog.geometry());
            geo.moveCenter(this->geometry().center());
            captureDialog.move(geo.topLeft());
            captureDialog.exec();
        }
    });
    actScreenshotAct = new QAction(tr("Actual content"), this);
    QObject::connect(actScreenshotAct, &QAction::triggered, this, [=](){
        QImage captureImage(GlobalObjects::mpvplayer->grabFramebuffer());
        if (liveDanmuList->isVisible())
        {
            QPainter p(&captureImage);
            p.drawPixmap(liveDanmuList->pos(), liveDanmuList->grab());
        }
        const PlayListItem *curItem = GlobalObjects::playlist->getCurrentItem();
        Capture captureDialog(captureImage,playPause,curItem);
        QRect geo(captureDialog.geometry());
        geo.moveCenter(this->geometry().center());
        captureDialog.move(geo.topLeft());
        captureDialog.exec();
    });
    actSnippet = new QAction(tr("Snippet Capture"), this);
    QObject::connect(actSnippet, &QAction::triggered, this, [=](){
        const PlayListItem *curItem = GlobalObjects::playlist->getCurrentItem();
        if (!curItem) return;
        SnippetCapture snippet(this);
        QRect geo(0,0,400*logicalDpiX()/96,600*logicalDpiY()/96);
        geo.moveCenter(this->geometry().center());
        snippet.move(mapToGlobal(geo.topLeft()));
        snippet.exec();
        GlobalObjects::mpvplayer->setMute(GlobalObjects::mpvplayer->getMute());
    });
    actGIF = new QAction(tr("GIF Capture"), this);
    QObject::connect(actGIF, &QAction::triggered, this, [=](){
        const PlayListItem *curItem = GlobalObjects::playlist->getCurrentItem();
        if (!curItem) return;
        GIFCapture gifCapture("", true, this);
        QRect geo(0,0,400*logicalDpiX()/96,600*logicalDpiY()/96);
        geo.moveCenter(this->geometry().center());
        gifCapture.move(mapToGlobal(geo.topLeft()));
        gifCapture.exec();
        GlobalObjects::mpvplayer->setMute(GlobalObjects::mpvplayer->getMute());
    });

    actPlayPause = new QAction(tr("Play/Pause"), this);
    QObject::connect(actPlayPause, &QAction::triggered, this, [](){
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

    actStop = new QAction(tr("Stop"), this);
    QObject::connect(actStop, &QAction::triggered, this, [=](){
        if (isFullscreen) actFullscreen->trigger();
        QCoreApplication::processEvents();
        GlobalObjects::playlist->cleanCurrentItem();
        GlobalObjects::mpvplayer->setState(MPVPlayer::Stop);
    });

    actFullscreen = new QAction(tr("Fullscreen"),this);
    QObject::connect(actFullscreen, &QAction::triggered, this, [=](){
        toggleFullScreenState(!isFullscreen);
        emit enterFullScreen(isFullscreen);
    });
    actPrev = new QAction(tr("Prev"),this);
    QObject::connect(actPrev, &QAction::triggered, this, [=](){
        switchItem(true,tr("No prev item"));
    });
    actNext = new QAction(tr("Next"),this);
    QObject::connect(actNext, &QAction::triggered, this, [=](){
        switchItem(false,tr("No next item"));
    });
    contexMenu = new ElaMenu(this);
    ctxText = contexMenu->addAction("");
    QObject::connect(ctxText, &QAction::triggered, this, [=](){
        currentDanmu=nullptr;
    });
    ctxCopy = contexMenu->addAction(tr("Copy Text"));
    QObject::connect(ctxCopy, &QAction::triggered, this, [=](){
        if(currentDanmu.isNull())return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(currentDanmu->text);
        currentDanmu=nullptr;
    });
    contexMenu->addSeparator();
    ctxBlockText = contexMenu->addAction(tr("Block Text"));
    QObject::connect(ctxBlockText, &QAction::triggered, this, [=](){
        if (currentDanmu.isNull())return;
        BlockRule *rule = new BlockRule(currentDanmu->text, BlockRule::Field::DanmuText, BlockRule::Relation::Contain);
        rule->name = tr("Text Rule");
        GlobalObjects::blocker->addBlockRule(rule);
        currentDanmu=nullptr;
        showMessage(tr("Block Rule Added"));
    });
    ctxBlockUser = contexMenu->addAction(tr("Block User"));
    QObject::connect(ctxBlockUser, &QAction::triggered, this, [=](){
        if (currentDanmu.isNull())return;
        BlockRule *rule = new BlockRule(currentDanmu->sender, BlockRule::Field::DanmuSender, BlockRule::Relation::Equal);
        rule->name = tr("User Rule");
        GlobalObjects::blocker->addBlockRule(rule);
        currentDanmu=nullptr;
        showMessage(tr("Block Rule Added"));
    });
    ctxBlockColor = contexMenu->addAction(tr("Block Color"));
    QObject::connect(ctxBlockColor, &QAction::triggered, this, [=](){
        if (currentDanmu.isNull())return;
        BlockRule *rule = new BlockRule(QString::number(currentDanmu->color,16), BlockRule::Field::DanmuColor, BlockRule::Relation::Equal);
        rule->name = tr("Color Rule");
        GlobalObjects::blocker->addBlockRule(rule);
        currentDanmu=nullptr;
        showMessage(tr("Block Rule Added"));
    });
}


void PlayerWindow::initPlaySetting()
{
    playSettingPage = new OptionMenu(this->centralWidget());
    playSettingPage->installEventFilter(this);
    OptionMenu *menu = static_cast<OptionMenu *>(playSettingPage);
    OptionMenuPanel *rootPanel = menu->defaultMenuPanel();
    setTrackPage(rootPanel);
    setPlaybackRatePage(rootPanel);
    setDisplayPage(rootPanel);
    setMPVConfPage(rootPanel);
}

void PlayerWindow::setTrackPage(OptionMenuPanel *rootPanel)
{
    ElaSelfTheme *btnSelfTheme = new ElaSelfTheme(rootPanel);
    btnSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicText, Qt::white);
    btnSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicText, Qt::white);

    ElaSelfTheme *spinSelfTheme = new ElaSelfTheme(rootPanel);
    spinSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicBase, QColor(255, 255, 255, 40));
    spinSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicBase, QColor(255, 255, 255, 40));
    spinSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicText, Qt::white);
    spinSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicText, Qt::white);
    spinSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicBaseDeep, QColor(255, 255, 255, 60));
    spinSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicBaseDeep, QColor(255, 255, 255, 60));
    spinSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicHoverAlpha, QColor(255, 255, 255, 80));
    spinSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicHoverAlpha, QColor(255, 255, 255, 80));
    spinSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicPressAlpha, QColor(255, 255, 255, 60));
    spinSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicPressAlpha, QColor(255, 255, 255, 60));
    spinSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicBorder, QColor(255, 255, 255, 60));
    spinSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicBorder, QColor(255, 255, 255, 60));

    OptionMenuItem *audioTrackMenu = rootPanel->addMenu(tr("Audio Track"));
    OptionMenuPanel *audioPanel = audioTrackMenu->addSubMenuPanel();
    ElaToolButton *addAudioTrack = new ElaToolButton(audioPanel);
    addAudioTrack->setSelfTheme(btnSelfTheme);
    addAudioTrack->setText(tr("Add"));
    audioPanel->addTitleMenu(tr("Audio Track"))->setWidget(addAudioTrack);
    audioPanel->addSpliter();
    OptionMenuItem *clearAudioMenu = audioPanel->addMenu(tr("Clear External Audio Files"));

    QObject::connect(addAudioTrack, &ElaToolButton::clicked, this, [=](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        QString dialogPath;
        if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            dialogPath = QFileInfo(GlobalObjects::mpvplayer->getCurrentFile()).absolutePath();
        }
        QString file(QFileDialog::getOpenFileName(this,tr("Select Audio File"), dialogPath, tr("Audio (%0);;All Files(*)").arg(GlobalObjects::mpvplayer->audioFormats.join(" "))));
        if (!file.isEmpty())
        {
            GlobalObjects::mpvplayer->addAudioTrack(file);
        }
        if (restorePlayState) GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });

    QObject::connect(clearAudioMenu, &OptionMenuItem::click, this, [](){
        GlobalObjects::playlist->clearCurrentAudio();
        GlobalObjects::mpvplayer->clearExternalAudio();
    });


    OptionMenuItem *subTrackMenu = rootPanel->addMenu(tr("Sub Track"));
    OptionMenuPanel *subPanel = subTrackMenu->addSubMenuPanel();
    ElaToolButton *addSubTrack = new ElaToolButton(subPanel);
    addSubTrack->setSelfTheme(btnSelfTheme);
    addSubTrack->setText(tr("Add"));
    subPanel->addTitleMenu(tr("Sub Track"))->setWidget(addSubTrack);
    subPanel->addSpliter();
    OptionMenuItem *subDelayMenu = subPanel->addMenu(tr("Subtitle Delay(s)"));
    subDelayMenu->setSelectAble(false);
    ElaSpinBox *delaySpinBox = new ElaSpinBox(subDelayMenu);
    delaySpinBox->setSelfTheme(spinSelfTheme);
    delaySpinBox->getLineEdit()->setObjectName(QStringLiteral("DelaySpinLineEdit"));
    delaySpinBox->setRange(INT_MIN,INT_MAX);
    subDelayMenu->setWidget(delaySpinBox);
    OptionMenuItem *hideSubMenu = subPanel->addMenu(tr("Hide Sub"));
    OptionMenuItem *clearSubMenu = subPanel->addMenu(tr("Clear External Sub Files"));

    QObject::connect(addSubTrack, &ElaToolButton::clicked, this, [=](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        QString dialogPath;
        if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            dialogPath = QFileInfo(GlobalObjects::mpvplayer->getCurrentFile()).absolutePath();
        }
        QString file(QFileDialog::getOpenFileName(this, tr("Select Sub File"), dialogPath ,tr("Subtitle (%0);;All Files(*)").arg(GlobalObjects::mpvplayer->subtitleFormats.join(" "))));
        if (!file.isEmpty())
        {
            GlobalObjects::mpvplayer->addSubtitle(file);
        }
        if (restorePlayState) GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });

    QObject::connect(delaySpinBox,&QSpinBox::editingFinished,[delaySpinBox](){
        GlobalObjects::mpvplayer->setSubDelay(delaySpinBox->value());
        GlobalObjects::playlist->setCurrentSubDelay(delaySpinBox->value());
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::subDelayChanged, delaySpinBox, &QSpinBox::setValue);

    QObject::connect(hideSubMenu, &OptionMenuItem::click, this, [=](){
        GlobalObjects::mpvplayer->hideSubtitle(true);
        subPanel->clearCheckGroup("sub_track_group");
        subTrackMenu->setInfoText("");
    });

    QObject::connect(clearSubMenu, &OptionMenuItem::click, this, [](){
        GlobalObjects::playlist->clearCurrentSub();
        GlobalObjects::mpvplayer->clearExternalSub();
    });

    auto trackRefresh = [=](MPVPlayer::TrackType type){
        if (type == MPVPlayer::TrackType::AudioTrack)
        {
            const QString audioTrackFlag = "audio_track";
            audioPanel->removeFlag(audioTrackFlag);
            audioTrackMenu->setInfoText("");
            auto &audioTrackList = GlobalObjects::mpvplayer->getTrackList(MPVPlayer::TrackType::AudioTrack);
            const int trackIdx = GlobalObjects::mpvplayer->getCurrentTrack(MPVPlayer::TrackType::AudioTrack);
            int curIdx = 0;
            for (auto &t : audioTrackList)
            {
                if (t.isExternal && !t.externalFile.isEmpty())
                {
                    GlobalObjects::playlist->addCurrentAudio(t.externalFile);
                }
                OptionMenuItem *item = audioPanel->addMenu(t.title);
                item->setCheckable(true);
                item->setCheckGroup("audio_track_group");
                item->setFlag(audioTrackFlag);
                if (curIdx == trackIdx)
                {
                    item->setChecking(true);
                    audioTrackMenu->setInfoText(t.title);
                }
                QObject::connect(item, &OptionMenuItem::click, this, [=](){
                    GlobalObjects::mpvplayer->setTrackId(MPVPlayer::TrackType::AudioTrack, curIdx);
                    GlobalObjects::playlist->setCurrentAudioIndex(curIdx);
                    audioTrackMenu->setInfoText(t.title);
                });
                ++curIdx;
            }
        }
        else if (type == MPVPlayer::TrackType::SubTrack)
        {
            const QString subTrackFlag = "sub_track";
            subPanel->removeFlag(subTrackFlag);
            subTrackMenu->setInfoText("");
            auto &subTrackList = GlobalObjects::mpvplayer->getTrackList(MPVPlayer::TrackType::SubTrack);
            int trackIdx = GlobalObjects::mpvplayer->getCurrentTrack(MPVPlayer::TrackType::SubTrack);
            int curIdx = 0;
            for (auto &t : subTrackList)
            {
                if (t.isExternal && !t.externalFile.isEmpty())
                {
                    GlobalObjects::playlist->addCurrentSub(t.externalFile);
                }
                OptionMenuItem *item = subPanel->addMenu(t.title);
                item->setCheckable(true);
                item->setCheckGroup("sub_track_group");
                item->setFlag(subTrackFlag);
                if (curIdx == trackIdx)
                {
                    item->setChecking(true);
                    subTrackMenu->setInfoText(t.title);
                }
                QObject::connect(item, &OptionMenuItem::click, this, [=](){
                    GlobalObjects::mpvplayer->hideSubtitle(false);
                    GlobalObjects::mpvplayer->setTrackId(MPVPlayer::TrackType::SubTrack, curIdx);
                    GlobalObjects::playlist->setCurrentSubIndex(curIdx);
                    subTrackMenu->setInfoText(t.title);
                });
                ++curIdx;
            }
        }
    };

    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::trackInfoChange, this, trackRefresh);
    trackRefresh(MPVPlayer::TrackType::AudioTrack);
    trackRefresh(MPVPlayer::TrackType::SubTrack);
}

void PlayerWindow::setPlaybackRatePage(OptionMenuPanel *rootPanel)
{
    ElaSelfTheme *btnSelfTheme = new ElaSelfTheme(rootPanel);
    btnSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicText, Qt::white);
    btnSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicText, Qt::white);

    OptionMenuItem *playRateMenu = rootPanel->addMenu(tr("Playback Rate"));
    OptionMenuPanel *ratePanel = playRateMenu->addSubMenuPanel();
    ElaToolButton *customRateBtn = new ElaToolButton(ratePanel);
    customRateBtn->setSelfTheme(btnSelfTheme);
    customRateBtn->setText(tr("Custom"));
    OptionMenuItem *titleMenu = ratePanel->addTitleMenu(tr("Playback Rate"));
    titleMenu->setWidget(customRateBtn);
    OptionMenuPanel *customPanel = titleMenu->addSubMenuPanel();
    QObject::connect(customRateBtn, &ElaToolButton::clicked, titleMenu, &OptionMenuItem::switchTo);
    customPanel->addTitleMenu(tr("Custom"));
    OptionMenuItem *customSliderMenu = customPanel->addMenu("");
    customSliderMenu->setSelectAble(false);
    ElaSlider *customSlider = new ElaSlider(Qt::Orientation::Horizontal, customSliderMenu);
    customSlider->setRange(0, 198);
    customSlider->setSingleStep(1);
    customSliderMenu->setWidget(customSlider, true);
    OptionMenuItem *tipMenu = customPanel->addMenu("");
    tipMenu->setTextCenter(true);
    tipMenu->setSelectAble(false);
    ratePanel->addSpliter();

    auto rateSetter = [](const QString &data){
        bool ok = false;
        double speed = data.toDouble(&ok);
        if (speed <= 0) return;
        if (ok) GlobalObjects::mpvplayer->setSpeed(speed);
    };

    const QString checkGroup = "playback_rate";
    OptionMenuItem *customMenu = ratePanel->addMenu("");
    customMenu->setCheckable(true);
    customMenu->setCheckGroup(checkGroup);

    bool matchRate = false;
    double curRate = GlobalObjects::mpvplayer->getPlaySpeed();
    playRateMenu->setInfoText(QString::number(curRate));
    for (const QString &rate : GlobalObjects::mpvplayer->speedLevel)
    {
        OptionMenuItem *m = ratePanel->addMenu(rate);
        m->setCheckable(true);
        m->setCheckGroup(checkGroup);
        m->setData(rate);
        if (rate.toDouble() == curRate)
        {
            m->setChecking(true);
            matchRate = true;
        }
        QObject::connect(m, &OptionMenuItem::click, this, [=](){
            rateSetter(m->menuData().toString());
        });
    }
    if (matchRate)
    {
        const QString settingRate = GlobalObjects::appSetting->value(SETTING_KEY_CUSTOM_PLAYBACK_RATE, "1.0").toString();
        customMenu->setText(tr("Custom(%1)").arg(settingRate));
        customMenu->setData(settingRate);
    }
    else
    {
        customMenu->setText(tr("Custom(%1)").arg(QString::number(curRate)));
        customMenu->setChecking(true);
        customMenu->setData(QString::number(curRate));
    }
    tipMenu->setText(customMenu->menuData().toString() + "x");
    customSlider->setValue((customMenu->menuData().toString().toDouble() - 0.1) / 0.05);
    QObject::connect(customMenu, &OptionMenuItem::click, this, [=](){
        rateSetter(customMenu->menuData().toString());
    });

    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::speedChanged, this, [=](double val){
        playRateMenu->setInfoText(QString::number(val));
        if (!customMenu->isChecking())
        {
            QVector<OptionMenuItem *> items = ratePanel->getCheckGroupMenus(checkGroup);
            for (int i = items.size() - 1; i >= 0; --i)
            {
                if (items[i]->menuData().toString().toDouble() == val)
                {
                    items[i]->setChecking(true);
                    return;
                }
            }
        }
        customMenu->setText(tr("Custom(%1)").arg(QString::number(val)));
        customMenu->setChecking(true);
        customMenu->setData(QString::number(val));
    });

    QObject::connect(customSlider, &ElaSlider::valueChanged, this, [=](int val){
        const QString rate = QString::number(val * 0.05 + 0.1);
        tipMenu->setText(rate + "x");
        customMenu->setData(rate);
        customMenu->setText(tr("Custom(%1)").arg(rate));
        if (customMenu->isChecking())
        {
            rateSetter(rate);
        }
        GlobalObjects::appSetting->setValue(SETTING_KEY_CUSTOM_PLAYBACK_RATE, rate);
    });

}

void PlayerWindow::setDisplayPage(OptionMenuPanel *rootPanel)
{
    ElaSelfTheme *btnSelfTheme = new ElaSelfTheme(rootPanel);
    btnSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicText, Qt::white);
    btnSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicText, Qt::white);

    OptionMenuItem *displayMenu = rootPanel->addMenu(tr("Display/Window Settings"));
    OptionMenuPanel *displayPanel = displayMenu->addSubMenuPanel();
    displayPanel->addTitleMenu(tr("Display Settings"));
    displayPanel->addSpliter();

    OptionMenuItem *aspectMenu = displayPanel->addMenu(tr("Aspect Ratio"));
    OptionMenuPanel *aspectPanel = aspectMenu->addSubMenuPanel();
    aspectPanel->addTitleMenu(aspectMenu->text());
    aspectPanel->addSpliter();

    OptionMenuItem *colorMenu = displayPanel->addMenu(tr("Color"));
    OptionMenuPanel *colorPanel = colorMenu->addSubMenuPanel();
    ElaToolButton *colorResetBtn = new ElaToolButton(colorPanel);
    colorResetBtn->setSelfTheme(btnSelfTheme);
    colorResetBtn->setText(tr("Reset"));
    colorPanel->addTitleMenu(colorMenu->text())->setWidget(colorResetBtn);
    colorPanel->addSpliter();

    OptionMenuItem *windowSizeMenu = displayPanel->addMenu(tr("Window Size"));
    OptionMenuPanel *windowSizePanel = windowSizeMenu->addSubMenuPanel();
    windowSizePanel->addTitleMenu(windowSizeMenu->text());
    windowSizePanel->addSpliter();

    OptionMenuItem *topMenu = displayPanel->addMenu(tr("On Top"));
    OptionMenuPanel *topPanel = topMenu->addSubMenuPanel();
    topPanel->addTitleMenu(topMenu->text());
    topPanel->addSpliter();

    const QStringList &aspectSettings = GlobalObjects::mpvplayer->videoAspect;
    int curAspectIdx = GlobalObjects::mpvplayer->getVideoAspectIndex();
    const QString aspectCheckGroup = "video_aspect";
    for (int i = 0; i < aspectSettings.size(); ++i)
    {
        OptionMenuItem *m = aspectPanel->addMenu(aspectSettings[i]);
        m->setCheckable(true);
        m->setCheckGroup(aspectCheckGroup);
        m->setData(i);
        if (i == curAspectIdx)
        {
            m->setChecking(true);
            aspectMenu->setInfoText(aspectSettings[i]);
        }
        QObject::connect(m, &OptionMenuItem::click, this, [=](){
            int idx = m->menuData().toInt();
            GlobalObjects::mpvplayer->setVideoAspect(idx);
            aspectMenu->setInfoText(GlobalObjects::mpvplayer->videoAspect[idx]);
        });
    }

    const int sliderMinW = 150;
    OptionMenuItem *brightnessMenu = colorPanel->addMenu(tr("Brightness"));
    brightnessMenu->setSelectAble(false);
    ElaSlider *brightnessSlider = new ElaSlider(Qt::Orientation::Horizontal, brightnessMenu);
    brightnessSlider->setRange(-100, 100);
    brightnessSlider->setValue(GlobalObjects::mpvplayer->getBrightness());
    brightnessSlider->setMinimumWidth(sliderMinW);
    brightnessMenu->setWidget(brightnessSlider);
    QObject::connect(brightnessSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::mpvplayer->setBrightness(val);
        brightnessSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::brightnessChanged, brightnessSlider, [=](int val){
        brightnessSlider->blockSignals(true);
        brightnessSlider->setValue(val);
        brightnessSlider->blockSignals(false);
    });

    OptionMenuItem *contrastMenu = colorPanel->addMenu(tr("Contrast"));
    contrastMenu->setSelectAble(false);
    ElaSlider *contrastSlider = new ElaSlider(Qt::Horizontal, contrastMenu);
    contrastSlider->setRange(-100, 100);
    contrastSlider->setValue(GlobalObjects::mpvplayer->getContrast());
    contrastSlider->setMinimumWidth(sliderMinW);
    contrastMenu->setWidget(contrastSlider);
    QObject::connect(contrastSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::mpvplayer->setContrast(val);
        contrastSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::contrastChanged, contrastSlider, [=](int val){
        contrastSlider->blockSignals(true);
        contrastSlider->setValue(val);
        contrastSlider->blockSignals(false);
    });

    OptionMenuItem *saturationMenu = colorPanel->addMenu(tr("Saturation"));
    saturationMenu->setSelectAble(false);
    ElaSlider *saturationSlider = new ElaSlider(Qt::Horizontal, saturationMenu);
    saturationSlider->setRange(-100, 100);
    saturationSlider->setValue(GlobalObjects::mpvplayer->getSaturation());
    saturationSlider->setMinimumWidth(sliderMinW);
    saturationMenu->setWidget(saturationSlider);
    QObject::connect(saturationSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::mpvplayer->setSaturation(val);
        saturationSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::saturationChanged, saturationSlider, [=](int val){
        saturationSlider->blockSignals(true);
        saturationSlider->setValue(val);
        saturationSlider->blockSignals(false);
    });

    OptionMenuItem *gammaMenu = colorPanel->addMenu(tr("Gamma"));
    gammaMenu->setSelectAble(false);
    ElaSlider *gammaSlider = new ElaSlider(Qt::Horizontal, gammaMenu);
    gammaSlider->setRange(-100, 100);
    gammaSlider->setValue(GlobalObjects::mpvplayer->getGamma());
    gammaSlider->setMinimumWidth(sliderMinW);
    gammaMenu->setWidget(gammaSlider);
    QObject::connect(gammaSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::mpvplayer->setGamma(val);
        gammaSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::gammaChanged, gammaSlider, [=](int val){
        gammaSlider->blockSignals(true);
        gammaSlider->setValue(val);
        gammaSlider->blockSignals(false);
    });

    OptionMenuItem *hueMenu = colorPanel->addMenu(tr("Hue"));
    hueMenu->setSelectAble(false);
    ElaSlider *hueSlider = new ElaSlider(Qt::Horizontal, hueMenu);
    hueSlider->setRange(-100, 100);
    hueSlider->setValue(GlobalObjects::mpvplayer->getHue());
    hueSlider->setMinimumWidth(sliderMinW);
    hueMenu->setWidget(hueSlider);
    QObject::connect(hueSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::mpvplayer->setHue(val);
        hueSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::hueChanged, hueSlider, [=](int val){
        hueSlider->blockSignals(true);
        hueSlider->setValue(val);
        hueSlider->blockSignals(false);
    });

    OptionMenuItem *sharpenMenu = colorPanel->addMenu(tr("Sharpen"));
    sharpenMenu->setSelectAble(false);
    ElaSlider *sharpenSlider = new ElaSlider(Qt::Horizontal, sharpenMenu);
    sharpenSlider->setRange(-200, 200);
    sharpenSlider->setValue(GlobalObjects::mpvplayer->getSharpen());
    sharpenSlider->setMinimumWidth(sliderMinW);
    sharpenMenu->setWidget(sharpenSlider);
    QObject::connect(sharpenSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::mpvplayer->setSharpen(val);
        sharpenSlider->setToolTip(QString::number(val));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::sharpenChanged, this, [=](double val){
        sharpenSlider->blockSignals(true);
        sharpenSlider->setValue(val * 100);
        sharpenSlider->blockSignals(false);
    });

    QObject::connect(colorResetBtn, &ElaToolButton::clicked, this, [=](){
        brightnessSlider->setValue(0);
        contrastSlider->setValue(0);
        saturationSlider->setValue(0);
        gammaSlider->setValue(0);
        hueSlider->setValue(0);
        sharpenSlider->setValue(0);
    });

    const int defaultSizeIndex = GlobalObjects::appSetting->value("Play/WindowSize", 2).toInt();
    const QString windowSizeCheckGroup = "window_size";
    for (int i = 0; i < playerWindowSize.size(); ++i)
    {
        OptionMenuItem *m = windowSizePanel->addMenu(playerWindowSizeTip[i]);
        m->setCheckable(true);
        m->setCheckGroup(windowSizeCheckGroup);
        m->setData(i);
        if (i == defaultSizeIndex)
        {
            m->setChecking(true);
            windowSizeMenu->setInfoText(playerWindowSizeTip[i]);
        }
        QObject::connect(m, &OptionMenuItem::click, this, [=](){
            int idx = m->menuData().toInt();
            windowSizeMenu->setInfoText(playerWindowSizeTip[i]);
            resizePercent = playerWindowSize[idx];
            if (resizePercent != -1)
            {
                adjustPlayerSize(resizePercent);
            }
            GlobalObjects::appSetting->setValue("Play/WindowSize", idx);
        });
    }

    const int defaultPin = GlobalObjects::appSetting->value("Play/StayOnTop", 0).toInt();
    QStringList pinTip{tr("While Playing"), tr("Always"), tr("Never")};
    const QString pinCheckGroup = "on_top";
    for (int i = 0; i < pinTip.size(); ++i)
    {
        OptionMenuItem *m = topPanel->addMenu(pinTip[i]);
        m->setCheckable(true);
        m->setCheckGroup(pinCheckGroup);
        if (i == defaultPin)
        {
            m->setChecking(true);
            topMenu->setInfoText(pinTip[i]);
        }
        QObject::connect(m, &OptionMenuItem::click, this, [=](){
            updateTopStatus(i);
            topMenu->setInfoText(m->text());
        });
    }
}

void PlayerWindow::setMPVConfPage(OptionMenuPanel *rootPanel)
{
    ElaSelfTheme *btnSelfTheme = new ElaSelfTheme(rootPanel);
    btnSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicText, Qt::white);
    btnSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicText, Qt::white);

    OptionMenuItem *confMenu = rootPanel->addMenu(tr("MPV Conf"));
    OptionMenuPanel *confPanel = confMenu->addSubMenuPanel();
    ElaToolButton *addConfBtn = new ElaToolButton(confPanel);
    addConfBtn->setSelfTheme(btnSelfTheme);
    addConfBtn->setText(tr("Add"));
    confPanel->addTitleMenu(tr("MPV Conf"))->setWidget(addConfBtn);
    confPanel->addSpliter();

    QObject::connect(addConfBtn, &ElaToolButton::clicked, this, [=](){
        MPVConfEdiror editor(this);
        QRect geo(0, 0, 600, 500);
        geo.moveCenter(this->geometry().center());
        editor.move(mapToGlobal(geo.topLeft()));
        editor.exec();
    });

    auto refreshConf = [=](){
        const QString confFlag = "mpv_conf";
        confPanel->removeFlag(confFlag);
        const QStringList &confList = GlobalObjects::mpvplayer->allOptionGroups();
        const QString curConf = GlobalObjects::mpvplayer->currentOptionGroup();
        const QString aspectCheckGroup = "mpv_conf";
        for (int i = 0; i < confList.size(); ++i)
        {
            OptionMenuItem *m = confPanel->addMenu(confList[i]);
            m->setCheckable(true);
            m->setCheckGroup(aspectCheckGroup);
            m->setFlag(confFlag);
            m->setData(confList[i]);
            if (confList[i] == curConf)
            {
                m->setChecking(true);
                confMenu->setInfoText(confList[i]);
            }
            QObject::connect(m, &OptionMenuItem::click, this, [=](){
                if(GlobalObjects::mpvplayer->setOptionGroup(m->menuData().toString()))
                {
                    confMenu->setInfoText(m->menuData().toString());
                    Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, tr("Switch to option group \"%1\"").arg(GlobalObjects::mpvplayer->currentOptionGroup()));
                }
            });
        }
    };

    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::optionGroupChanged, this, refreshConf);
    refreshConf();
}

void PlayerWindow::initDanmuSettting()
{
    danmuSettingPage = new OptionMenu(this->centralWidget());
    danmuSettingPage->installEventFilter(this);
    OptionMenu *menu = static_cast<OptionMenu *>(danmuSettingPage);
    OptionMenuPanel *rootPanel = menu->defaultMenuPanel();

    setHidePage(rootPanel);
    setDanmuDisplayPage(rootPanel);
    setLiveModePage(rootPanel);
    setDanmuOptionItems(rootPanel);
}

void PlayerWindow::setHidePage(OptionMenuPanel *rootPanel)
{
    ElaSelfTheme *switchSelfTheme = new ElaSelfTheme(rootPanel);
    switchSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicBase, QColor(255, 255, 255, 40));
    switchSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicBase, QColor(255, 255, 255, 40));
    switchSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicHover, QColor(255, 255, 255, 80));
    switchSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicHover, QColor(255, 255, 255, 80));
    switchSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicBorder, QColor(255, 255, 255, 60));
    switchSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicBorder, QColor(255, 255, 255, 60));
    switchSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::ToggleSwitchNoToggledCenter, QColor(255, 255, 255, 90));
    switchSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::ToggleSwitchNoToggledCenter, QColor(255, 255, 255, 90));

    OptionMenuItem *hideDanmuMenu = rootPanel->addMenu(tr("Hide Danmu"));
    OptionMenuPanel *hidePanel = hideDanmuMenu->addSubMenuPanel();
    hidePanel->addTitleMenu(tr("Hide Danmu"));
    hidePanel->addSpliter();

    ElaToggleSwitch *hideSwitch = new ElaToggleSwitch(hideDanmuMenu);
    hideSwitch->setSelfTheme(switchSelfTheme);
    hideSwitch->setIsToggled(GlobalObjects::mpvplayer->getDanmuHide());
    hideDanmuMenu->setWidget(hideSwitch);

    OptionMenuItem *hideAllMenu = hidePanel->addMenu(tr("All"));
    hideAllMenu->setCheckable(true);
    QObject::connect(hideSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked) {
        setHideDanmu(checked);
        hideAllMenu->setChecking(checked);
    });
    QObject::connect(hideAllMenu, &OptionMenuItem::click, this, [=](){
        setHideDanmu(hideAllMenu->isChecking());
        hideSwitch->setIsToggled(hideAllMenu->isChecking());
    });

    OptionMenuItem *hideRollingMenu = hidePanel->addMenu(tr("Rolling"));
    hideRollingMenu->setCheckable(true);
    hideRollingMenu->setChecking(GlobalObjects::danmuRender->isHidden(DanmuComment::Rolling));
    QObject::connect(hideRollingMenu, &OptionMenuItem::click, this, [=](){
        GlobalObjects::danmuRender->hideDanmu(DanmuComment::Rolling, hideRollingMenu->isChecking());
    });

    OptionMenuItem *hideTopMenu = hidePanel->addMenu(tr("Top"));
    hideTopMenu->setCheckable(true);
    hideTopMenu->setChecking(GlobalObjects::danmuRender->isHidden(DanmuComment::Top));
    QObject::connect(hideTopMenu, &OptionMenuItem::click, this, [=](){
        GlobalObjects::danmuRender->hideDanmu(DanmuComment::Top, hideTopMenu->isChecking());
    });

    OptionMenuItem *hideBottomMenu = hidePanel->addMenu(tr("Bottom"));
    hideBottomMenu->setCheckable(true);
    hideBottomMenu->setChecking(GlobalObjects::danmuRender->isHidden(DanmuComment::Bottom));
    QObject::connect(hideBottomMenu, &OptionMenuItem::click, this, [=](){
        GlobalObjects::danmuRender->hideDanmu(DanmuComment::Bottom, hideBottomMenu->isChecking());
    });
}

void PlayerWindow::setDanmuDisplayPage(OptionMenuPanel *rootPanel)
{
    OptionMenuItem *displayMenu = rootPanel->addMenu(tr("Display Settings"));
    OptionMenuPanel *displayPanel = displayMenu->addSubMenuPanel();
    displayPanel->addTitleMenu(displayMenu->text());
    displayPanel->addSpliter();

    OptionMenuItem *denseLevelMenu = displayPanel->addMenu(tr("Dense Level"));
    OptionMenuPanel *denseLevelPanel = denseLevelMenu->addSubMenuPanel();
    denseLevelPanel->addTitleMenu(tr("Dense Level"));
    denseLevelPanel->addSpliter();
    const QStringList denseLevels = {tr("Uncovered"), tr("General"), tr("Dense")};
    for (int i = 0; i < denseLevels.size(); ++i)
    {
        OptionMenuItem *m = denseLevelPanel->addMenu(denseLevels[i]);
        m->setCheckable(true);
        if (i == GlobalObjects::danmuRender->dense)
        {
            m->setChecking(true);
            denseLevelMenu->setInfoText(denseLevels[i]);
        }
        m->setCheckGroup("dense_level");
        QObject::connect(m, &OptionMenuItem::click, this, [=](){
            GlobalObjects::danmuRender->setDenseLevel(i);
            denseLevelMenu->setInfoText(m->text());
        });
    }


    OptionMenuItem *displayAreaMenu = displayPanel->addMenu(tr("Display Area"));
    OptionMenuPanel *displayAreaPanel = displayAreaMenu->addSubMenuPanel();
    displayAreaPanel->addTitleMenu(displayAreaMenu->text());
    displayAreaPanel->addSpliter();
    const QStringList displayAreas = {tr("1/8"), tr("1/4"), tr("1/2"), tr("3/4"), tr("Full")};
    for (int i = 0; i < displayAreas.size(); ++i)
    {
        OptionMenuItem *m = displayAreaPanel->addMenu(displayAreas[i]);
        m->setCheckable(true);
        if (i == GlobalObjects::danmuRender->getDisplayAreaIndex())
        {
            m->setChecking(true);
            displayAreaMenu->setInfoText(displayAreas[i]);
        }
        m->setCheckGroup("display_area");
        QObject::connect(m, &OptionMenuItem::click, this, [=](){
            GlobalObjects::danmuRender->setDisplayArea(i);
            displayAreaMenu->setInfoText(m->text());
        });
    }

    OptionMenuItem *subProtectMenu = displayPanel->addMenu(tr("Sub Protect"));
    OptionMenuPanel *subProtectPanel = subProtectMenu->addSubMenuPanel();
    subProtectPanel->addTitleMenu(subProtectMenu->text());
    subProtectPanel->addSpliter();
    OptionMenuItem *bottomSubProtectMenu = subProtectPanel->addMenu(tr("Bottom Sub"));
    bottomSubProtectMenu->setCheckable(true);
    bottomSubProtectMenu->setChecking(GlobalObjects::danmuRender->isBottomSubProtect());
    OptionMenuItem *topSubProtectMenu = subProtectPanel->addMenu(tr("Top Sub"));
    topSubProtectMenu->setCheckable(true);
    topSubProtectMenu->setChecking(GlobalObjects::danmuRender->isTopSubProtect());
    QString subProtectInfoText;
    if (bottomSubProtectMenu->isChecking()) subProtectInfoText = tr("Bottom");
    if (topSubProtectMenu->isChecking())
    {
        subProtectInfoText += subProtectInfoText.isEmpty()? tr("Top") : "|" + tr("Top");
    }
    subProtectMenu->setInfoText(subProtectInfoText);
    QObject::connect(bottomSubProtectMenu, &OptionMenuItem::click, this, [=](){
        GlobalObjects::danmuRender->setBottomSubtitleProtect(bottomSubProtectMenu->isChecking());
        if (bottomSubProtectMenu->isChecking())
        {
            subProtectMenu->setInfoText(topSubProtectMenu->isChecking()? tr("Bottom") + "|" + tr("Top") : tr("Bottom"));
        }
        else
        {
            subProtectMenu->setInfoText(topSubProtectMenu->isChecking()? tr("Top") : "");
        }
    });
    QObject::connect(topSubProtectMenu, &OptionMenuItem::click, this, [=](){
        GlobalObjects::danmuRender->setTopSubtitleProtect(topSubProtectMenu->isChecking());
        if (bottomSubProtectMenu->isChecking())
        {
            subProtectMenu->setInfoText(topSubProtectMenu->isChecking()? tr("Bottom") + "|" + tr("Top") : tr("Bottom"));
        }
        else
        {
            subProtectMenu->setInfoText(topSubProtectMenu->isChecking()? tr("Top") : "");
        }
    });

    OptionMenuItem *danmuCountMenu = displayPanel->addMenu(tr("Max Count"));
    OptionMenuPanel *danmuCountPanel = danmuCountMenu->addSubMenuPanel();
    danmuCountPanel->addTitleMenu(danmuCountMenu->text());
    OptionMenuItem *danmuCountSliderMenu = danmuCountPanel->addMenu("");
    danmuCountSliderMenu->setSelectAble(false);
    ElaSlider *danmuCountSlider = new ElaSlider(Qt::Orientation::Horizontal, danmuCountSliderMenu);
    danmuCountSlider->setRange(20, 300);
    danmuCountSlider->setValue(GlobalObjects::danmuRender->getMaxDanmuCount());
    danmuCountSliderMenu->setWidget(danmuCountSlider, true);
    OptionMenuItem *tipMenu = danmuCountPanel->addMenu(QString::number(danmuCountSlider->value()));
    tipMenu->setTextCenter(true);
    tipMenu->setSelectAble(false);
    danmuCountMenu->setInfoText(QString::number(danmuCountSlider->value()));
    QObject::connect(danmuCountSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::danmuRender->setMaxDanmuCount(val);
        tipMenu->setText(QString::number(val));
        danmuCountMenu->setInfoText(QString::number(val));
    });

    OptionMenuItem *danmuSpeedMenu = displayPanel->addMenu(tr("Rolling Speed"));
    OptionMenuPanel *danmuSpeedPanel = danmuSpeedMenu->addSubMenuPanel();
    danmuSpeedPanel->addTitleMenu(danmuSpeedMenu->text());
    OptionMenuItem *danmuSpeedSliderMenu = danmuSpeedPanel->addMenu("");
    danmuSpeedSliderMenu->setSelectAble(false);
    ElaSlider *danmuSpeedSlider = new ElaSlider(Qt::Orientation::Horizontal, danmuSpeedSliderMenu);
    danmuSpeedSlider->setRange(50, 500);
    danmuSpeedSlider->setValue(GlobalObjects::danmuRender->getSpeed());
    danmuSpeedSliderMenu->setWidget(danmuSpeedSlider, true);
    OptionMenuItem *tipSpeedMenu = danmuSpeedPanel->addMenu(QString::number(danmuSpeedSlider->value()));
    tipSpeedMenu->setTextCenter(true);
    tipSpeedMenu->setSelectAble(false);
    danmuSpeedMenu->setInfoText(QString::number(danmuSpeedSlider->value()));
    QObject::connect(danmuSpeedSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::danmuRender->setSpeed(val);
        tipSpeedMenu->setText(QString::number(val));
        danmuSpeedMenu->setInfoText(QString::number(val));
    });
}

void PlayerWindow::setLiveModePage(OptionMenuPanel *rootPanel)
{
    ElaSelfTheme *switchSelfTheme = new ElaSelfTheme(rootPanel);
    switchSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicBase, QColor(255, 255, 255, 40));
    switchSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicBase, QColor(255, 255, 255, 40));
    switchSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicHover, QColor(255, 255, 255, 80));
    switchSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicHover, QColor(255, 255, 255, 80));
    switchSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicBorder, QColor(255, 255, 255, 60));
    switchSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicBorder, QColor(255, 255, 255, 60));
    switchSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::ToggleSwitchNoToggledCenter, QColor(255, 255, 255, 90));
    switchSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::ToggleSwitchNoToggledCenter, QColor(255, 255, 255, 90));

    OptionMenuItem *liveModeMenu = rootPanel->addMenu(tr("Live Mode"));
    OptionMenuPanel *liveModePanel = liveModeMenu->addSubMenuPanel();
    liveModePanel->addTitleMenu(tr("Live Mode"));
    liveModePanel->addSpliter();

    ElaToggleSwitch *liveSwitch = new ElaToggleSwitch(liveModeMenu);
    liveSwitch->setSelfTheme(switchSelfTheme);
    liveSwitch->setIsToggled(GlobalObjects::danmuRender->isLiveModeOn());
    liveModeMenu->setWidget(liveSwitch);

    QObject::connect(liveSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked) {
        GlobalObjects::danmuRender->setLiveMode(checked);
        if(GlobalObjects::playlist->getCurrentItem() && !GlobalObjects::mpvplayer->getDanmuHide())
        {
            liveDanmuList->setVisible(checked);
        }
    });

    OptionMenuItem *liveRollingMenu = liveModePanel->addMenu(tr("Only Rolling Danmu"));
    liveRollingMenu->setCheckable(true);
    liveRollingMenu->setChecking(GlobalObjects::danmuRender->isLiveOnlyRolling());
    QObject::connect(liveRollingMenu, &OptionMenuItem::click, this, [=](){
        GlobalObjects::danmuRender->setLiveMode(GlobalObjects::danmuRender->isLiveModeOn(), liveRollingMenu->isChecking()? 1: 0);
    });

    OptionMenuItem *liveAlignRightMenu = liveModePanel->addMenu(tr("Align Right"));
    liveAlignRightMenu->setCheckable(true);
    liveAlignRightMenu->setChecking(GlobalObjects::danmuRender->liveDanmuModel()->alignment() == Qt::AlignRight);
    QObject::connect(liveAlignRightMenu, &OptionMenuItem::click, this, [=](){
        if (liveAlignRightMenu->isChecking())
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

    OptionMenuItem *liveShowSenderMenu = liveModePanel->addMenu(tr("Show Sender"));
    liveShowSenderMenu->setCheckable(true);
    liveShowSenderMenu->setChecking(GlobalObjects::danmuRender->liveDanmuModel()->isShowSender());
    QObject::connect(liveShowSenderMenu, &OptionMenuItem::click, this, [=](){
        GlobalObjects::danmuRender->liveDanmuModel()->setShowSender(liveShowSenderMenu->isChecking());
    });

    const int sliderMinW = 150;
    OptionMenuItem *liveDanmuSizeMenu = liveModePanel->addMenu(tr("Live Danmu Size"));
    liveDanmuSizeMenu->setSelectAble(false);
    ElaSlider *liveSizeSlider = new ElaSlider(Qt::Horizontal, liveDanmuSizeMenu);
    liveSizeSlider->setRange(6, 20);
    liveSizeSlider->setValue(GlobalObjects::danmuRender->liveDanmuModel()->getFontSize());
    liveSizeSlider->setToolTip(QString::number(liveSizeSlider->value()));
    liveSizeSlider->setMinimumWidth(sliderMinW);
    liveDanmuSizeMenu->setWidget(liveSizeSlider);
    QObject::connect(liveSizeSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::danmuRender->liveDanmuModel()->setFontSize(val);
        liveSizeSlider->setToolTip(QString::number(val));
    });

    OptionMenuItem *liveVRangeMenu = liveModePanel->addMenu(tr("Vertical Range"));
    ElaSlider *liveVRangeSlider = new ElaSlider(Qt::Horizontal, liveVRangeMenu);
    liveVRangeSlider->setRange(20, 95);
    liveVRangeSlider->setMinimumWidth(sliderMinW);
    liveVRangeMenu->setWidget(liveVRangeSlider);
    QObject::connect(liveVRangeSlider, &QSlider::valueChanged, this, [=](int val){
        liveDanmuList->setFixedHeight(static_cast<float>(val) / 100 * height());
        liveVRangeSlider->setToolTip(QString::number(val));
        liveVRange = val;
    });
    liveVRangeSlider->setValue(this->liveVRange);

}

void PlayerWindow::setDanmuOptionItems(OptionMenuPanel *rootPanel)
{
    const int sliderMinW = 120;
    OptionMenuItem *alphaMenu = rootPanel->addMenu(tr("Opacity"));
    alphaMenu->setSelectAble(false);
    ElaSlider *alphaSlider = new ElaSlider(Qt::Horizontal, alphaMenu);
    alphaSlider->setRange(0, 100);
    alphaSlider->setValue(GlobalObjects::danmuRender->getOpacity() * 100);
    alphaSlider->setToolTip(QString::number(alphaSlider->value()));
    alphaSlider->setMinimumWidth(sliderMinW);
    alphaMenu->setWidget(alphaSlider);
    QObject::connect(alphaSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::danmuRender->setOpacity(val/100.f);
        alphaSlider->setToolTip(QString::number(val));
    });

    OptionMenuItem *fontSizeMenu = rootPanel->addMenu(tr("Font Size"));
    fontSizeMenu->setSelectAble(false);
    ElaSlider *fontSizeSlider = new ElaSlider(Qt::Horizontal, fontSizeMenu);
    fontSizeSlider->setRange(4, 80);
    fontSizeSlider->setValue(GlobalObjects::danmuRender->getFontSize());
    fontSizeSlider->setToolTip(QString::number(fontSizeSlider->value()));
    fontSizeSlider->setMinimumWidth(sliderMinW);
    fontSizeMenu->setWidget(fontSizeSlider);
    QObject::connect(fontSizeSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::danmuRender->setFontSize(val);
        fontSizeSlider->setToolTip(QString::number(val));
    });

    OptionMenuItem *moreMenu = rootPanel->addMenu(tr("More Settings"));
    QObject::connect(moreMenu, &OptionMenuItem::click, this, [=](){
        Settings settings(Settings::PAGE_DANMU, this);
        QRect geo(0, 0, 420, 600);
        geo.moveCenter(this->geometry().center());
        settings.move(mapToGlobal(geo.topLeft()));
        danmuSettingPage->hide();
        settings.exec();
    });

}

void PlayerWindow::initVolumeSetting()
{
    volumeSettingPage = new OptionMenu(this->centralWidget());
    volumeSettingPage->installEventFilter(this);
    OptionMenu *menu = static_cast<OptionMenu *>(volumeSettingPage);
    OptionMenuPanel *rootPanel = menu->defaultMenuPanel();

    OptionMenuItem *volumeMenu = rootPanel->addMenu("");
    volumeMenu->setFixHeight(140);
    volumeMenu->setMinWidth(30);
    volumeMenu->setSelectAble(false);
    volumeSlider = new ElaSlider(Qt::Vertical, volumeMenu);
    volumeSlider->setMinimum(0);
    volumeSlider->setMaximum(150);
    volumeSlider->setSingleStep(1);

    QObject::connect(volumeSlider, &QSlider::valueChanged, this, [=](int val){
        GlobalObjects::mpvplayer->setVolume(val);
        volumeSlider->setToolTip(QString::number(val));
    });
    volumeSlider->setValue(GlobalObjects::appSetting->value("Play/Volume", 50).toInt());
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::volumeChanged, volumeSlider, [=](int value){
        volumeSlider->blockSignals(true);
        volumeSlider->setValue(value);
        volumeSlider->blockSignals(false);
    });

    volumeMenu->setMarginRight(14);
    volumeMenu->setWidget(volumeSlider, true);
    volumeSettingPage->hide();

    QObject::connect(&volumeHideTimer, &QTimer::timeout, this, [=](){
        volumeSettingPage->hide();
    });

}

void PlayerWindow::initLaunchOptionSetting()
{
    launchOptionPage = new OptionMenu(this->centralWidget());
    launchOptionPage->installEventFilter(this);
    OptionMenu *menu = static_cast<OptionMenu *>(launchOptionPage);
    OptionMenuPanel *rootPanel = menu->defaultMenuPanel();

    {
        OptionMenuItem *colorMenu = rootPanel->addMenu("");
        colorMenu->setFixHeight(54);
        colorMenu->setSelectAble(false);
        ColorSelector *colorSelector = new ColorSelector(QList<QColor>{
            QColor(255, 255, 255),
            QColor(0xFE, 0x03, 0x02),
            QColor(0xFF, 0xFF, 0x00),
            QColor(0x00, 0xCD, 0x00),
            QColor(0x42, 0x66, 0xBE),
            QColor(0xCC, 0x02, 0x73),
        }, colorMenu);
        QColor customColor = GlobalObjects::appSetting->value(SETTING_KEY_LAUNCH_CUSTOM_COLOR, QColor(255, 255, 255)).value<QColor>();
        if (customColor.isValid())
        {
            colorSelector->setColor(customColor);
            launchColor = customColor;
        }
        QObject::connect(colorSelector, &ColorSelector::colorChanged, this, [=](const QColor &c){
            launchColor = c;
            GlobalObjects::appSetting->setValue(SETTING_KEY_LAUNCH_CUSTOM_COLOR, launchColor);
        });

        colorMenu->setWidget(colorSelector, true);
    }

    {
        OptionMenuItem *posMenu = rootPanel->addMenu("");
        posMenu->setSelectAble(false);
        QButtonGroup *posBtnGroup = new QButtonGroup(posMenu);
        QWidget *container = new QWidget(posMenu);
        QHBoxLayout *cHLayout = new QHBoxLayout(container);
        cHLayout->setContentsMargins(0, 0, 0, 0);
        const QStringList posTips = { tr("Rolling"), tr("Top"), tr("Bottom") };
        for (int i = 0; i < DanmuComment::DanmuType::UNKNOW; ++i)
        {
            QPushButton *posBtn = new QPushButton(posTips[i], container);
            posBtn->setCheckable(true);
            if (i == 0) posBtn->setChecked(true);
            posBtn->setObjectName(QStringLiteral("MenuCheckButton"));
            cHLayout->addWidget(posBtn);
            posBtnGroup->addButton(posBtn, i);
        }
        QObject::connect(posBtnGroup, &QButtonGroup::idToggled, this, [=](int id, bool checked){
            if (checked) launchType = DanmuComment::DanmuType(id);
        });

        cHLayout->addSpacing(10);
        QButtonGroup *sizeBtnGroup = new QButtonGroup(posMenu);
        const QStringList sizeTips = { tr("Normal"), tr("Small"), tr("Large") };
        for (int i = 0; i < 3; ++i)
        {
            QPushButton *sizeBtn = new QPushButton(sizeTips[i], container);
            sizeBtn->setCheckable(true);
            if (i == 0) sizeBtn->setChecked(true);
            sizeBtn->setObjectName(QStringLiteral("MenuCheckButton"));
            cHLayout->addWidget(sizeBtn);
            sizeBtnGroup->addButton(sizeBtn, i);
        }
        QObject::connect(sizeBtnGroup, &QButtonGroup::idToggled, this, [=](int id, bool checked){
            if (checked) launchSize = DanmuComment::FontSizeLevel(id);
        });

        posMenu->setWidget(container, true);
    }

}

QPoint PlayerWindow::getPopupMenuPos(QWidget *ref, QWidget *popup, int topSpace)
{
    QPoint refCenter = ref->mapToGlobal(QPoint(ref->width() / 2, 0));
    QPoint playerRefCenter = this->mapFromGlobal(refCenter);
    int x = playerRefCenter.x() - popup->width() / 2;
    int y = playerRefCenter.y() - popup->height() - topSpace;
    if (x + popup->width() > this->width()) x = this->width() - popup->width() - 10;
    return QPoint(x, y);
}

void PlayerWindow::updateTopStatus(int option)
{
    switch (option)
    {
    case 0:
        onTopWhilePlaying = true;
        emit setStayOnTop(GlobalObjects::mpvplayer->getState()==MPVPlayer::Play && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty());
        break;
    case 1:
        onTopWhilePlaying = false;
        emit setStayOnTop(true);
        break;
    case 2:
        onTopWhilePlaying = false;
        emit setStayOnTop(false);
        break;
    default:
        break;
    }
    GlobalObjects::appSetting->setValue("Play/StayOnTop", option);
}

void PlayerWindow::showPlayerRegion(bool on)
{
    playInfoPanel->setVisible(on);
    progress->setVisible(on);
    timeLabel->setVisible(on);
    launchDanmuEdit->setVisible(on);
}

QWidget *PlayerWindow::initPlayerLayer(QWidget *parent)
{
    // flow tip & preview
    playInfo = new InfoTips(parent);
    initProgressPreview(parent);

    QWidget *playerLayerContainer = new QWidget(parent);

    // logo, recent list
    playerContent = new PlayerContent(playerLayerContainer);
    playerContent->raise();

    // side panel
    initSidePanel(playerLayerContainer);

    // title info
    playInfoPanel = new QWidget(playerLayerContainer);
    playInfoPanel->setObjectName(QStringLiteral("widgetPlayInfo"));
    playInfoPanel->hide();
    initPlayInfo(playInfoPanel);

    // control area
    playControlPanel = new QWidget(playerLayerContainer);
    playControlPanel->setObjectName(QStringLiteral("widgetPlayControl"));
    playControlPanel->hide();

    progress = new ClickSlider(playControlPanel);
    //progress->setObjectName(QStringLiteral("ProgressSlider"));
    progress->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
    progress->setTracking(false);
    progress->setMouseTracking(true);

    DanmuStatisWidget *statWidget = new DanmuStatisWidget(playControlPanel);
    statWidget->setObjectName(QStringLiteral("DanmuStatisBar"));
    statWidget->setRefSlider(progress);
    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::statisInfoChange, statWidget, &DanmuStatisWidget::refreshStatis);
    danmuStatisBar = statWidget;
    const int statisBarHeight = 50;
    danmuStatisBar->setMinimumHeight(statisBarHeight);
    danmuStatisBar->hide();

    QVBoxLayout *controlVLayout = new QVBoxLayout(playControlPanel);
    controlVLayout->setSpacing(0);
    controlVLayout->addStretch(1);
    controlVLayout->addSpacing(20);
    controlVLayout->addWidget(statWidget);
    controlVLayout->addSpacing(2);
    controlVLayout->addWidget(progress);
    controlVLayout->addSpacing(2);
    controlVLayout->addLayout(initPlayControl(playControlPanel));

    miniProgress = new ClickSlider(playerLayerContainer);
    // miniProgress->setObjectName(QStringLiteral("MiniProgressSlider"));
    miniProgress->hide();

    playerLayerContainer->setMouseTracking(true);
    QVBoxLayout *contralVLayout = new QVBoxLayout(playerLayerContainer);
    contralVLayout->setContentsMargins(0,0,0,0);
    contralVLayout->setSpacing(0);
    contralVLayout->addWidget(playInfoPanel);
    contralVLayout->addStretch(1);
    contralVLayout->addWidget(miniProgress);
    contralVLayout->addWidget(playControlPanel);

    return playerLayerContainer;
}

QWidget *PlayerWindow::initLiveDanmuLayer(QWidget *parent)
{
    QWidget *liveDanmuContainer = new QWidget(parent);
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
    liveVLayout->addWidget(liveDanmuList, 0, GlobalObjects::danmuRender->liveDanmuModel()->alignment());
    liveDanmuList->hide();
    liveVRange = GlobalObjects::appSetting->value("Play/LiveVRange", 40).toInt();
    return liveDanmuContainer;
}

void PlayerWindow::initSettings()
{
    const int defaultSize = GlobalObjects::appSetting->value("Play/WindowSize", 2).toInt();
    if (defaultSize >= 0 && defaultSize < playerWindowSize.size())
    {
        resizePercent = playerWindowSize[defaultSize];
        if (resizePercent != -1)
        {
            adjustPlayerSize(resizePercent);
        }
    }

    const int defaultPin = GlobalObjects::appSetting->value("Play/StayOnTop", 0).toInt();
    updateTopStatus(defaultPin);

    jumpForwardTime = GlobalObjects::appSetting->value(SETTING_KEY_FORWARD_JUMP, 5).toInt();
    jumpBackwardTime = GlobalObjects::appSetting->value(SETTING_KEY_BACKWARD_JUMP, 5).toInt();
}

void PlayerWindow::initProgressPreview(QWidget *parent)
{
    progressInfo = new QWidget(parent);
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

    timeInfoTip = new QLabel(tiWidget);
    timeInfoTip->setObjectName(QStringLiteral("TimeInfoTip"));
    timeInfoTip->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    previewLabel = new QLabel(progressInfo);
    previewLabel->setObjectName(QStringLiteral("ProgressPreview"));
    piVLayout->addStretch(1);
    piHLayout->addWidget(timeInfoTip);
    piHLayout->addStretch(1);
    piVLayout->addLayout(piHLayout);
    piSLayout->addWidget(tiWidget);
    piSLayout->addWidget(previewLabel);
    previewLabel->hide();
    progressInfo->hide();

    QObject::connect(&previewTimer, &QTimer::timeout, this, [=](){
        if (GlobalObjects::mpvplayer->getShowPreview() && !progressInfo->isHidden() && previewLabel->isHidden())
        {
            QPixmap *pixmap = GlobalObjects::mpvplayer->getPreview(progress->curMousePos()/1000);
            if (pixmap)
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
}

void PlayerWindow::initSignals()
{
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::durationChanged, this, [this](int duration){
        // QCoreApplication::processEvents();
        int ts=duration/1000;
        int lmin=ts/60;
        int ls=ts-lmin*60;
        PlayContext::context()->duration = ts;
        progress->setRange(0,duration);
        progress->setSingleStep(1);
        miniProgress->setRange(0,duration);
        miniProgress->setSingleStep(1);
        totalTimeStr=QString("/%1:%2").arg(lmin,2,10,QChar('0')).arg(ls,2,10,QChar('0'));
        timeLabel->setText("00:00"+this->totalTimeStr);
        static_cast<DanmuStatisWidget *>(danmuStatisBar)->setDuration(ts);
        const PlayListItem *currentItem=GlobalObjects::playlist->getCurrentItem();
        if (currentItem)
        {
            if (currentItem->playTime > 15 && currentItem->playTime < ts - 15)
            {
                GlobalObjects::mpvplayer->seek(currentItem->playTime*1000);
                showMessage(tr("Jumped to the last play position"));
            }
        }
#ifdef QT_DEBUG
        qDebug()<<"Duration Changed";
#endif
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::fileChanged, this, [](){
        if (!EventBus::getEventBus()->hasListener(EventBus::EVENT_PLAYER_FILE_CHANGED)) return;
        QVariantMap param = {
            { "file", GlobalObjects::mpvplayer->getCurrentFile() }
        };
        const PlayListItem *currentItem = GlobalObjects::playlist->getCurrentItem();
        if (currentItem)
        {
            Pool *pool = GlobalObjects::danmuManager->getPool(currentItem->poolID, false);
            if (pool)
            {
                param["anime_name"] = pool->animeTitle();
                param["epinfo"] = pool->toEp().toMap();
            }
        }
        EventBus::getEventBus()->pushEvent(EventParam{EventBus::EVENT_PLAYER_FILE_CHANGED, param});
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::fileChanged, this, [this](){
        const PlayListItem *currentItem = GlobalObjects::playlist->getCurrentItem();
        showPlayerRegion(true);
        launchFocusTimeS = -1;
        progress->setEventMark(QVector<DanmuEvent>());
        progress->setChapterMark(QVector<MPVPlayer::ChapterInfo>());
        QString mediaTitle(GlobalObjects::mpvplayer->getMediaTitle());
        if (!currentItem)
        {
            titleLabel->setText(mediaTitle);
            launchDanmuEdit->hide();
            GlobalObjects::danmuPool->setPoolID("");
            if(resizePercent!=-1 && !miniModeOn) adjustPlayerSize(resizePercent);
#ifdef QT_DEBUG
            qDebug()<<"File Changed(external item), Current File: " << GlobalObjects::mpvplayer->getCurrentFile();
#endif
            return;
        }
        if (currentItem->animeTitle.isEmpty())
        {
            if (currentItem->type == PlayListItem::ItemType::WEB_URL)
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
        if (currentItem->hasPool())
        {
            GlobalObjects::danmuPool->setPoolID(currentItem->poolID);
            launchDanmuEdit->show();
        }
        else
        {
            launchDanmuEdit->hide();
            GlobalObjects::danmuPool->setPoolID("");
            if (currentItem->type == PlayListItem::ItemType::LOCAL_FILE)
            {
                showMessage(tr("File is not associated with Danmu Pool"));
            }
        }
        if (GlobalObjects::danmuPool->isLoadLocalDanmu() && currentItem->type == PlayListItem::ItemType::LOCAL_FILE)
        {
            QString danmuFile(currentItem->path.mid(0, currentItem->path.lastIndexOf('.'))+".xml");
            QFileInfo fi(danmuFile);
            if (fi.exists())
            {
                GlobalObjects::danmuPool->addLocalDanmuFile(danmuFile);
            }
        }
        if (GlobalObjects::danmuRender->isLiveModeOn())
        {
            liveDanmuList->show();
        }
        if (currentItem->trackInfo)
        {
            for (const QString &audioFile : currentItem->trackInfo->audioFiles)
            {
                GlobalObjects::mpvplayer->addAudioTrack(audioFile);
            }
            for (const QString &subFile : currentItem->trackInfo->subFiles)
            {
                GlobalObjects::mpvplayer->addSubtitle(subFile);
            }
            GlobalObjects::mpvplayer->setSubDelay(currentItem->trackInfo->subDelay);
            if (currentItem->trackInfo->audioIndex > -1)
            {
                GlobalObjects::mpvplayer->setTrackId(MPVPlayer::TrackType::AudioTrack, currentItem->trackInfo->audioIndex);
            }
            if (currentItem->trackInfo->subIndex > -1)
            {
                GlobalObjects::mpvplayer->setTrackId(MPVPlayer::TrackType::SubTrack, currentItem->trackInfo->subIndex);
                GlobalObjects::mpvplayer->hideSubtitle(false);
            }
        }
        else
        {
            GlobalObjects::mpvplayer->setSubDelay(0);
        }
        if (resizePercent!=-1 && !miniModeOn) adjustPlayerSize(resizePercent);
#ifdef QT_DEBUG
        qDebug()<<"File Changed,Current Item: "<<currentItem->title;
#endif
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::chapterChanged, this, [this](){
        progress->setChapterMark(GlobalObjects::mpvplayer->getChapters());
    });
    QObject::connect(GlobalObjects::playlist, &PlayList::currentMatchChanged, this, [this](){
        const PlayListItem *currentItem=GlobalObjects::playlist->getCurrentItem();
        if(currentItem->animeTitle.isEmpty())
            titleLabel->setText(currentItem->title);
        else
            titleLabel->setText(QString("%1-%2").arg(currentItem->animeTitle, currentItem->title));
        launchDanmuEdit->setVisible(currentItem->hasPool());
    });
    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::eventAnalyzeFinished, progress, &ClickSlider::setEventMark);
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::positionChanged, this, [this](int newtime){
        const int cs = newtime / 1000;
        const int cmin = cs / 60;
        const int cls = cs - cmin * 60;
        PlayContext::context()->playtime = cs;
        if(!progressPressed)
        {
            progress->blockSignals(true);
            progress->setValue(newtime);
            progress->blockSignals(false);
        }
        if(miniModeOn) miniProgress->setValue(newtime);
        timeLabel->setText(QString("%1:%2%3").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')).arg(this->totalTimeStr));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::bufferingStateChanged, this, [this](int val){
        timeLabel->setText(tr("Buffering: %1%, %2").arg(val).arg(formatSize(true, GlobalObjects::mpvplayer->getCacheSpeed())));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::refreshPreview, this, [this](int timePos, QPixmap *pixmap){
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
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::stateChanged, this, [](MPVPlayer::PlayState state){
        if (!EventBus::getEventBus()->hasListener(EventBus::EVENT_PLAYER_STATE_CHANGED)) return;
        QVariantMap param = {
            { "state", (int)state }
        };
        if (state == MPVPlayer::Play)
        {
            if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
            {
                EventBus::getEventBus()->pushEvent(EventParam{EventBus::EVENT_PLAYER_STATE_CHANGED, param});
            }
        }
        else
        {
            EventBus::getEventBus()->pushEvent(EventParam{EventBus::EVENT_PLAYER_STATE_CHANGED, param});
        }
    });

    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::stateChanged, this, [this](MPVPlayer::PlayState state) {
        bool has_video = !GlobalObjects::mpvplayer->getCurrentFile().isEmpty();
        bool is_playing = (state == MPVPlayer::Play && has_video);
        if (is_playing)
            setAwakeRequired();
        else
            unsetAwakeRequired();

        if(onTopWhilePlaying)
            emit setStayOnTop(is_playing);

        switch(state)
        {
        case MPVPlayer::Play:
            if (has_video)
            {
                this->playPause->setText(QChar(0xe632));
                playerContent->hide();
            }
            break;
        case MPVPlayer::Pause:
            if (has_video)
                this->playPause->setText(QChar(0xe628));
            break;
        case MPVPlayer::EndReached:
		{
            this->playPause->setText(QChar(0xe628));
			GlobalObjects::danmuPool->reset();
			GlobalObjects::danmuRender->cleanup();
            GlobalObjects::playlist->setCurrentPlayTime();
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
            launchDanmuEdit->hide();
            launchFocusTimeS = -1;
            playPause->setText(QChar(0xe628));
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
            //GlobalObjects::playlist->cleanCurrentItem();
            GlobalObjects::danmuPool->setPoolID("");
            GlobalObjects::danmuRender->cleanup();
            //QTimer::singleShot(100, GlobalObjects::mpvplayer, "update");
            playerContent->raise();
            playerContent->show();
            liveDanmuList->hide();
            danmuStatisBar->hide();
            PlayContext::context()->clear();
            showPlayerRegion(false);
            sidePanel->show();
            break;
        }
        }
    });
    QObject::connect(playPause,&QPushButton::clicked,actPlayPause,&QAction::trigger);
    QObject::connect(stop, &QPushButton::clicked, actStop, &QAction::trigger);
    QObject::connect(progress, &ClickSlider::sliderClick, this, [](int pos){
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
    QObject::connect(progress, &ClickSlider::sliderUp, this, [this](int pos){
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
    QObject::connect(progress, &ClickSlider::sliderPressed, this, [this](){
        this->progressPressed = true;
    });
    QObject::connect(progress, &ClickSlider::sliderReleased, this, [this](){
        this->progressPressed = false;
    });
    QObject::connect(progress, &ClickSlider::mouseMove, this, [this](int, int ,int pos, const QString &desc){
        int cs = pos / 1000;
        int cmin = cs / 60;
        int cls = cs - cmin * 60;
        QString timeTip(QString("%1:%2").arg(cmin, 2, 10, QChar('0')).arg(cls, 2, 10, QChar('0')));
        if (!desc.isEmpty()) timeTip += "\n" + desc;
        timeInfoTip->setText(timeTip);
        timeInfoTip->adjustSize();
        if (previewTimer.isActive()) previewTimer.stop();
        if (GlobalObjects::mpvplayer->getShowPreview() && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            previewLabel->clear();
            QPixmap *pixmap = GlobalObjects::mpvplayer->getPreview(cs, false);
            if (pixmap)
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
    QObject::connect(progress, &ClickSlider::mouseEnter, this, [this](){
		if (GlobalObjects::playlist->getCurrentItem() != nullptr && GlobalObjects::danmuPool->totalCount()>0)
			danmuStatisBar->show();
    });
    QObject::connect(progress, &ClickSlider::mouseLeave, this, [this](){
        danmuStatisBar->hide();
        progressInfo->hide();
    });
    QObject::connect(miniProgress, &ClickSlider::sliderClick, this, [](int pos){
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


    QObject::connect(mute, &QPushButton::clicked, this, [=](){
        GlobalObjects::mpvplayer->setMute(!GlobalObjects::mpvplayer->getMute());
    });
    if (GlobalObjects::appSetting->value("Play/Mute", false).toBool())
    {
        mute->click();
    }
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::muteChanged, this, [=](bool mute){
        this->mute->setText(mute? QChar(0xe726) : QChar(0xe725));
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::seekableChanged, this, [this](bool seekable){
        this->progress->setVisible(seekable);
        actSnippet->setEnabled(seekable);
        actGIF->setEnabled(seekable);
        if (!seekable)
        {
            progressInfo->hide();
        }
        if(this->miniModeOn && this->miniProgress->isVisible() && !seekable)
        {
            this->miniProgress->setVisible(false);
        }
        PlayContext::context()->seekable = seekable;
    });

    QObject::connect(launchOptionBtn, &QPushButton::clicked, this, [this](){
        if (!launchOptionPage)
        {
            initLaunchOptionSetting();
        }
        if (!launchOptionPage->isHidden())
        {
            launchOptionPage->hide();
            return;
        }
        if (playSettingPage && !playSettingPage->isHidden())
        {
            playSettingPage->hide();
        }
        if (danmuSettingPage && !danmuSettingPage->isHidden())
        {
            danmuSettingPage->hide();
        }
        launchOptionPage->resize(launchOptionPage->sizeHint());
        launchOptionPage->show();
        launchOptionPage->raise();
        launchOptionPage->move(getPopupMenuPos(launchOptionBtn, launchOptionPage, 10));
    });
    QObject::connect(launchDanmuEdit, &QLineEdit::returnPressed, this, [this](){
        if (!GlobalObjects::danmuPool->hasPool()) return;
        if (GlobalObjects::mpvplayer->getCurrentFile().isEmpty()) return;
        QString text = launchDanmuEdit->text().trimmed();
        if (text.isEmpty()) return;
        int launchSendTimeS = PlayContext::context()->playtime;
        if (launchSendTimeS < 0) return;
        int time = launchSendTimeS;
        if (launchFocusTimeS > 0 && launchSendTimeS - launchFocusTimeS < 5) time = launchFocusTimeS;

        QSharedPointer<DanmuComment> comment{new DanmuComment};
        comment->originTime = comment->time = time * 1000;  //ms
        comment->color = launchColor.rgb();
        comment->text = text;
        comment->type = launchType;
        comment->fontSizeLevel = launchSize;
        comment->date = QDateTime::currentDateTime().toSecsSinceEpoch();
#ifdef KSERVICE
        KService::instance()->launch(comment, GlobalObjects::danmuPool->getPool()->id(), GlobalObjects::mpvplayer->getCurrentFile());
#endif
        GlobalObjects::danmuPool->launch({comment});
        launchDanmuEdit->clear();
    });


    QObject::connect(danmu, &QPushButton::clicked, this, [this]() {
        if (!danmuSettingPage)
        {
            initDanmuSettting();
        }
        if (!danmuSettingPage->isHidden())
		{
			danmuSettingPage->hide();
			return;
		}
        if (playSettingPage && !playSettingPage->isHidden())
        {
            playSettingPage->hide();
        }
        if (launchOptionPage && !launchOptionPage->isHidden())
        {
            launchOptionPage->hide();
        }
        danmuSettingPage->resize(danmuSettingPage->sizeHint());
        danmuSettingPage->show();
		danmuSettingPage->raise();
        danmuSettingPage->move(getPopupMenuPos(danmu, danmuSettingPage, 10));
    });
    QObject::connect(setting, &QPushButton::clicked, this, [this](){
        if (!playSettingPage)
        {
            initPlaySetting();
        }
        if (!playSettingPage->isHidden())
		{
			playSettingPage->hide();
			return;
		}
        if (danmuSettingPage && !danmuSettingPage->isHidden())
        {
            danmuSettingPage->hide();
        }
        if (launchOptionPage && !launchOptionPage->isHidden())
        {
            launchOptionPage->hide();
        }
        playSettingPage->resize(playSettingPage->sizeHint());
        playSettingPage->show();
		playSettingPage->raise();
        playSettingPage->move(getPopupMenuPos(setting, playSettingPage, 10));
    });
    QObject::connect(next, &QPushButton::clicked, actNext, &QAction::trigger);
    QObject::connect(prev, &QPushButton::clicked, actPrev, &QAction::trigger);
    QObject::connect(fullscreen,&QPushButton::clicked,actFullscreen,&QAction::trigger);
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::toggleFullScreen, actFullscreen,&QAction::trigger);
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::toggleMiniMode, actMiniMode, &QAction::trigger);
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::triggerStop, actStop, &QAction::trigger);

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
    double scale = percent / 100.0 / devicePixelRatioF();
    emit resizePlayer(videoSize.width * scale,videoSize.height * scale,aspectRatio);
}

void PlayerWindow::setHideDanmu(bool hide)
{
    if (hide)
    {
        GlobalObjects::mpvplayer->hideDanmu(true);
        liveDanmuList->setVisible(false);
        this->danmu->setText(QChar(0xe618));
    }
    else
    {
        GlobalObjects::mpvplayer->hideDanmu(false);
        if (GlobalObjects::danmuRender->isLiveModeOn() && GlobalObjects::playlist->getCurrentItem())
        {
            liveDanmuList->setVisible(true);
        }
        this->danmu->setText(QChar(0xe619));
    }
}

void PlayerWindow::showMessage(const QString &msg, int flag, const QVariant &extra)
{
    Q_UNUSED(flag)
    if (extra.isNull())
    {
        showMessage(msg, "");
    }
    else
    {
        const QVariantMap extraInfo = extra.toMap();
        const QString type = extraInfo.value("type").toString();
        const int timeout = extraInfo.value("timeout", -1).toInt();
        showMessage(msg, type, timeout);
    }
}

void PlayerWindow::showMessage(const QString &msg, const QString &type, int timeout)
{
    int y = height() - 20*logicalDpiY()/96 - (miniModeOn? miniProgress->height() : playControlPanel->height());
    static_cast<InfoTips *>(playInfo)->setBottom(y);
    if (timeout >= 0)
    {
        static_cast<InfoTips *>(playInfo)->showMessage(msg, type, timeout);
    }
    else
    {
        static_cast<InfoTips *>(playInfo)->showMessage(msg, type);
    }
}

QLayout *PlayerWindow::initPlayControl(QWidget *playControlPanel)
{
    GlobalObjects::iconfont->setPointSize(22);
    QFont normalFont;
    normalFont.setFamily(GlobalObjects::normalFont);

    timeLabel = new QLabel("00:00/00:00",playControlPanel);
    timeLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    timeLabel->setObjectName(QStringLiteral("labelTime"));
    normalFont.setPointSize(10);
    timeLabel->setFont(normalFont);

    playPause = new QPushButton(playControlPanel);
    playPause->setFont(*GlobalObjects::iconfont);
    playPause->setText(QChar(0xe628));
    playPause->setObjectName(QStringLiteral("PlayControlButton"));
    playPause->setToolTip(tr("Play/Pause(Space)"));

    prev = new QPushButton(playControlPanel);
    prev->setFont(*GlobalObjects::iconfont);
    prev->setText(QChar(0xe78c));
    prev->setToolTip(tr("Prev(PageUp)"));
    prev->setObjectName(QStringLiteral("PlayControlButton"));

    next = new QPushButton(playControlPanel);
    next->setFont(*GlobalObjects::iconfont);
    next->setText(QChar(0xea3b));
    next->setToolTip(tr("Next(PageDown)"));
    next->setObjectName(QStringLiteral("PlayControlButton"));

    GlobalObjects::iconfont->setPointSize(20);

    stop = new QPushButton(playControlPanel);
    stop->setFont(*GlobalObjects::iconfont);
    stop->setText(QChar(0xea89));
    stop->setToolTip(tr("Stop"));
    stop->setObjectName(QStringLiteral("PlayControlButton"));

    mute = new QPushButton(playControlPanel);
    mute->setFont(*GlobalObjects::iconfont);
    mute->setText(QChar(0xe725));
    mute->setToolTip(tr("Mute/Unmute"));
    mute->setObjectName(QStringLiteral("PlayControlButton"));
    mute->installEventFilter(this);

    GlobalObjects::iconfont->setPointSize(20);

    setting = new QPushButton(playControlPanel);
    setting->setFont(*GlobalObjects::iconfont);
    setting->setText(QChar(0xe625));
    setting->setToolTip(tr("Play Setting"));
    setting->setObjectName(QStringLiteral("PlayControlButton"));

    danmu = new QPushButton(playControlPanel);
    danmu->setFont(*GlobalObjects::iconfont);
    danmu->setText(QChar(0xe619));
    danmu->setToolTip(tr("Danmu Setting"));
    danmu->setObjectName(QStringLiteral("PlayControlButton"));

    fullscreen = new QPushButton(playControlPanel);
    fullscreen->setFont(*GlobalObjects::iconfont);
    fullscreen->setText(QChar(0xe649));
    fullscreen->setToolTip(tr("FullScreen"));
    fullscreen->setObjectName(QStringLiteral("PlayControlButton"));


    launchDanmuEdit = new KLineEdit(playControlPanel);
    launchDanmuEdit->setPlaceholderText(tr("Launch Danmu"));
    launchDanmuEdit->setMinimumHeight(30);
    QMargins textMargins = launchDanmuEdit->textMargins();
    textMargins.setRight(6);
    launchDanmuEdit->setTextMargins(textMargins);
    normalFont.setPointSize(12);
    launchDanmuEdit->setFont(normalFont);
    launchDanmuEdit->setObjectName(QStringLiteral("LaunchDanmuEdit"));
    QPalette palette = launchDanmuEdit->palette();
    palette.setColor(QPalette::PlaceholderText, QColor(0, 0, 0, 100));
    launchDanmuEdit->setPalette(palette);
    launchDanmuEdit->installEventFilter(this);

    launchOptionBtn = new QPushButton(launchDanmuEdit);
    launchOptionBtn->setObjectName(QStringLiteral("LaunchOptionButton"));
    GlobalObjects::iconfont->setPointSize(12);
    launchOptionBtn->setFont(*GlobalObjects::iconfont);
    launchOptionBtn->setText(QChar(0xea02));
    launchOptionBtn->setCursor(Qt::ArrowCursor);
    launchOptionBtn->setFocusPolicy(Qt::NoFocus);
    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(launchOptionBtn);
    launchDanmuEdit->addAction(optionsAction, QLineEdit::LeadingPosition);

    ctrlPressCount = 0;
    altPressCount = 0;
    QObject::connect(&doublePressTimer, &QTimer::timeout, this, [=](){
        ctrlPressCount=0;
        altPressCount=0;
        doublePressTimer.stop();
    });
    QObject::connect(&hideCursorTimer, &QTimer::timeout, this, [=](){
        if (isFullscreen) setCursor(Qt::BlankCursor);
    });

    QHBoxLayout *buttonHLayout=new QHBoxLayout();
    buttonHLayout->setContentsMargins(8, 6, 8, 6);
    buttonHLayout->setSpacing(10);

    buttonHLayout->addWidget(prev);
    buttonHLayout->addSpacing(8);
    buttonHLayout->addWidget(playPause);
    buttonHLayout->addSpacing(6);
    buttonHLayout->addWidget(next);
    buttonHLayout->addSpacing(4);
    buttonHLayout->addWidget(stop);
    buttonHLayout->addWidget(mute);
    buttonHLayout->addWidget(timeLabel);
    buttonHLayout->addStretch(5);
    buttonHLayout->addWidget(launchDanmuEdit);
    buttonHLayout->addStretch(7);
    buttonHLayout->addWidget(setting);
    buttonHLayout->addWidget(danmu);
    buttonHLayout->addWidget(fullscreen);

    return buttonHLayout;
}

void PlayerWindow::initPlayInfo(QWidget *playInfoPanel)
{
    QFont normalFont;
    normalFont.setFamily(GlobalObjects::normalFont);
    normalFont.setPointSize(12);

    titleLabel = new ElidedLabel("", playInfoPanel);
    titleLabel->setFontColor(QColor(255, 255, 255));
    // titleLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    titleLabel->setObjectName(QStringLiteral("labelTitle"));
    titleLabel->setFont(normalFont);

    constexpr const int infoBtnCount = 3;
    QPair<QChar, QString> infoButtonTexts[infoBtnCount] = {
        {QChar(0xe6ca), tr("Media Info")},
        {QChar(0xe655), tr("Screenshot")},
        {QChar(0xe866), tr("Mini Mode")}
    };
    QPushButton *mediaInfoBtn{nullptr}, *captureBtn{nullptr}, *miniModeBtn{nullptr};
    QPushButton **infoBtnPtrs[infoBtnCount] = {
        &mediaInfoBtn, &captureBtn, &miniModeBtn
    };
    GlobalObjects::iconfont->setPointSize(18);
    QHBoxLayout *infoHLayout = new QHBoxLayout(playInfoPanel);
    infoHLayout->setSpacing(10);
    infoHLayout->addWidget(titleLabel);
    //infoHLayout->addStretch(1);
    for (int i = 0; i < infoBtnCount; ++i)
    {
        *infoBtnPtrs[i] = new QPushButton(playInfoPanel);
        QPushButton *tb = *infoBtnPtrs[i];
        tb->setFont(*GlobalObjects::iconfont);
        tb->setText(infoButtonTexts[i].first);
        tb->setToolTip(infoButtonTexts[i].second);
        tb->setObjectName(QStringLiteral("PlayControlButton"));
        infoHLayout->addWidget(tb);
    }

    QObject::connect(mediaInfoBtn, &QPushButton::clicked, this, [this](){
        if (GlobalObjects::mpvplayer->getCurrentFile().isEmpty()) return;
        MediaInfo mediaInfoDialog(this);
        QRect geo(0,0,400*logicalDpiX()/96,600*logicalDpiY()/96);
        geo.moveCenter(this->geometry().center());
        mediaInfoDialog.move(mapToGlobal(geo.topLeft()));
        mediaInfoDialog.exec();
    });

    auto showPlayInfoPage = [=](const QString &panelName){
        if (!playInfoPage) initPlayInfoPage();
        OptionMenu *menu = static_cast<OptionMenu *>(playInfoPage);
        OptionMenuPanel *showPanel = menu->getPanel(panelName);
        if (!showPanel) return;
        if (showPanel == menu->currentWidget() && !menu->isHidden())
        {
            menu->hide();
            return;
        }
        if (menu->isHidden())
        {
            menu->setCurrentWidget(showPanel);
            menu->resize(menu->sizeHint());
            menu->move(QPoint(playInfoPanel->width() - menu->width() - 10, playInfoPanel->height()));
            menu->show();
            menu->raise();
        }
        else
        {
            menu->raise();
            menu->setPanel(showPanel, OptionMenu::TopRight);
        }
    };
    QObject::connect(captureBtn, &QPushButton::clicked, this, [=](){
        showPlayInfoPage("default");
    });
    QObject::connect(miniModeBtn, &QPushButton::clicked, actMiniMode, &QAction::trigger);
}

void PlayerWindow::initPlayInfoPage()
{
    playInfoPage = new OptionMenu(this->centralWidget());
    playInfoPage->installEventFilter(this);
    OptionMenu *menu = static_cast<OptionMenu *>(playInfoPage);

    OptionMenuPanel *capturePanel = menu->defaultMenuPanel();
    QVector<QAction *> captureActions = {actScreenshotSrc, actScreenshotAct, actSnippet, actGIF};
    for (int i = 0; i < captureActions.size(); ++i)
    {
        QAction *a = captureActions[i];
        OptionMenuItem *m = capturePanel->addMenu(a->text());
        m->setMinWidth(160);
        QObject::connect(m, &OptionMenuItem::click, this, [=](){
            playInfoPage->hide();
            a->trigger();
        });
    }

}

void PlayerWindow::initSidePanel(QWidget *parent)
{
    sidePanel = new SidePanel(parent);
    sidePanel->setMouseTracking(true);
    sidePanel->setObjectName(QStringLiteral("widgetPlayerSidePanel"));

    constexpr const int sideBtnCount = 3;
    QPair<QChar, QString> sideButtonTexts[sideBtnCount] = {
        {QChar(0xe63c), tr("Playlist")},
        {QChar(0xe627), tr("Danmu")},
        {QChar(0xe67a), tr("Subtitle")}
    };

    QPushButton **sideBtnPtrs[sideBtnCount] = {
        &playlistBtn, &danmuBtn, &subBtn
    };
    GlobalObjects::iconfont->setPointSize(18);
    QHBoxLayout *sideHLayout = new QHBoxLayout(sidePanel);
    QVBoxLayout *sideVLayout = new QVBoxLayout();
    sideVLayout->setSpacing(20);
    sideVLayout->addStretch(1);
    for (int i = 0; i < sideBtnCount; ++i)
    {
        *sideBtnPtrs[i] = new QPushButton(sidePanel);
        QPushButton *tb = *sideBtnPtrs[i];
        tb->setFont(*GlobalObjects::iconfont);
        tb->setText(sideButtonTexts[i].first);
        tb->setToolTip(sideButtonTexts[i].second);
        tb->setObjectName(QStringLiteral("PlaySideButton"));
        tb->setCheckable(true);
        sideVLayout->addWidget(tb);
    }
    sideVLayout->addStretch(1);
    sideHLayout->addStretch(1);
    sideHLayout->addLayout(sideVLayout);
    sidePanel->resize(sidePanel->sizeHint());
    // sidePanel->hide();

    bool listVisibility = GlobalObjects::appSetting->value("MainWindow/ListVisibility", true).toBool();
    if (listVisibility) playlistBtn->setChecked(true);

    QObject::connect(playlistBtn, &QPushButton::clicked, this, [=](){
        danmuBtn->setChecked(false);
        subBtn->setChecked(false);
        emit toggleListVisibility(0);
    });
    QObject::connect(danmuBtn, &QPushButton::clicked, this, [=](){
        playlistBtn->setChecked(false);
        subBtn->setChecked(false);
        emit toggleListVisibility(1);
    });
    QObject::connect(subBtn, &QPushButton::clicked, this, [=](){
        danmuBtn->setChecked(false);
        playlistBtn->setChecked(false);
        emit toggleListVisibility(2);
    });


}

void PlayerWindow::switchItem(bool prev, const QString &nullMsg)
{
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
            PlayContext::context()->playItem(item);
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
    //int ty=danmuStatisBar->isHidden()?height()-playControlPanel->height()-progressInfo->height():
    //                                  height()-playControlPanel->height()-progressInfo->height()-statisBarHeight;
    int ty = height()-playControlPanel->height()-progressInfo->height()+(danmuStatisBar->isHidden()?20:danmuStatisBar->height()/2);
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
    if (miniModeOn)
    {
        if (mouseLPressed && pressPos != event->globalPos())
        {
            emit moveWindow(event->globalPos());
            event->accept();
            moving = true;
        }
        else if(GlobalObjects::mpvplayer->getSeekable())
        {
            if (height()-event->pos().y() < miniProgress->height() + 10)
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
    if (!autoHideControlPanel) return;
    if (launchDanmuEdit->hasFocus()) return;
    if (isFullscreen)
    {
        setCursor(Qt::ArrowCursor);
        hideCursorTimer.start(hideCursorTimeout);
    }
    if (GlobalObjects::mpvplayer->getClickBehavior() == 1)return;
    const QPoint pos = event->pos();
    if (height()-pos.y() < playControlPanel->height() + 40)
    {
        playControlPanel->show();
        if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            playInfoPanel->show();
        }
        hideCursorTimer.stop();
    }
    else if(pos.y() < playInfoPanel->height())
    {
        if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            playInfoPanel->show();
        }
        playControlPanel->hide();
        hideCursorTimer.stop();
    } 
    else
    {
        playInfoPanel->hide();
        playControlPanel->hide();
        if (this->width() - pos.x() < sidePanel->width() && pos.y() > sidePanel->height() * 0.2 && pos.y() < sidePanel->height() * 0.8)
        {
            sidePanel->show();
        }
        else
        {
            if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty()) sidePanel->hide();
        }
    } 
}

void PlayerWindow::mouseDoubleClickEvent(QMouseEvent *)
{
    if (miniModeOn)
    {
        exitMiniMode();
        return;
    }
    (GlobalObjects::mpvplayer->getDbClickBehavior() == 0 ? actFullscreen : actPlayPause)->trigger();
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
    if (miniModeOn && event->button() == Qt::LeftButton)
    {
        if (mouseLPressed && moving)
            moving = false;
        else
            actPlayPause->trigger();
        mouseLPressed = false;
        return;
    }
    if (event->button()==Qt::LeftButton)
    {
        if (danmuSettingPage && !danmuSettingPage->isHidden())
        {
            if (!danmuSettingPage->underMouse())
            {
                danmuSettingPage->hide();
                return;
            }
        }
        if (playSettingPage && !playSettingPage->isHidden())
        {
            if (!playSettingPage->underMouse())
            {
                playSettingPage->hide();
                return;
            }
        }
        if (playInfoPage && !playInfoPage->isHidden())
        {
            if (!playInfoPage->underMouse())
            {
                playInfoPage->hide();
                return;
            }
        }
        if (launchOptionPage && !launchOptionPage->isHidden())
        {
            if (!launchOptionPage->underMouse())
            {
                launchOptionPage->hide();
                return;
            }
        }
        if (GlobalObjects::mpvplayer->getClickBehavior() == 1)
        {
            if (playControlPanel->isHidden())
            {
                playControlPanel->show();
                playInfoPanel->show();
                sidePanel->show();
            }
            else
            {
                playInfoPanel->hide();
                playControlPanel->hide();
                if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty()) sidePanel->hide();
            }
        }
        else
        {
            actPlayPause->trigger();
        }
    }
}

void PlayerWindow::resizeEvent(QResizeEvent *)
{
    playInfoPanel->setMaximumWidth(width());
    //sidePanel->move(QPoint(width() - sidePanel->width(), (height() - sidePanel->height()) / 2));
    sidePanel->setGeometry(width() - sidePanel->width(), 0, sidePanel->width(), height());
    //playListCollapseButton->setGeometry(width()-playlistCollapseWidth,(height()-playlistCollapseHeight)/2,playlistCollapseWidth,playlistCollapseHeight);
    if (playSettingPage && !playSettingPage->isHidden())
    {
        playSettingPage->move(getPopupMenuPos(setting, playSettingPage, 10));
    }
    if (danmuSettingPage && !danmuSettingPage->isHidden())
	{
        danmuSettingPage->move(getPopupMenuPos(danmu, danmuSettingPage, 10));
	}
    if (playInfoPage && !playInfoPage->isHidden())
    {
        playInfoPage->move(QPoint(width() - playInfoPage->width() - 10, playInfoPanel->height()));
    }
    playerContent->resize(qMax<int>(width() * 0.7, 580), height());
    playerContent->move((width()-playerContent->width())/2,(height()-playerContent->height())/2);
    int y = height() - 20 - (miniModeOn? miniProgress->height() : playControlPanel->height());
    static_cast<InfoTips *>(playInfo)->setBottom(y);
    static_cast<InfoTips *>(playInfo)->updatePosition();
    liveDanmuList->setFixedSize(width() * 0.45, height() * static_cast<float>(liveVRange) / 100);
    liveDanmuList->scrollToBottom();
    launchDanmuEdit->setMinimumWidth(width() / 5);
}

void PlayerWindow::leaveEvent(QEvent *)
{
    if (autoHideControlPanel)
    {
        if (!launchDanmuEdit->hasFocus())
        {
            QTimer::singleShot(500, this, [this](){
                this->playControlPanel->hide();
                this->playInfoPanel->hide();
                if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty()) this->sidePanel->hide();
            });
        }
    }
    if (miniModeOn && !miniProgress->isHidden())
    {
        QTimer::singleShot(500, this, [this](){
            miniProgress->hide();
        });
    }
}

bool PlayerWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == danmuSettingPage || watched == playSettingPage || watched == launchOptionPage)
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
    if (watched == mute)
    {
        if (!volumeSettingPage) initVolumeSetting();
        if (event->type() == QEvent::Enter)
        {
            volumeSettingPage->show();
            volumeSettingPage->raise();
            QPoint leftTop(mute->geometry().center().x(), height() - playControlPanel->height() - volumeSettingPage->height() + 10);
            volumeSettingPage->move(getPopupMenuPos(mute, volumeSettingPage));
            volumeHideTimer.stop();
            return true;
        }
        else if (event->type() == QEvent::Leave)
        {
            volumeHideTimer.start(300);
            return true;
        }
    }
    if (watched == volumeSettingPage)
    {
        if (event->type() == QEvent::Leave)
        {
            volumeSettingPage->hide();
            return true;
        }
        else if (event->type() == QEvent::Enter)
        {
            volumeHideTimer.stop();
            return true;
        }
    }
    if (watched == launchDanmuEdit)
    {
        if (event->type() == QEvent::FocusIn)
        {
            launchFocusTimeS = PlayContext::context()->playtime;
        }
    }
    return QWidget::eventFilter(watched,event);
}

void PlayerWindow::keyPressEvent(QKeyEvent *event)
{
    QWidget *focusWidget = QApplication::focusWidget();
    if (focusWidget && focusWidget->inherits("QLineEdit")) return QWidget::keyPressEvent(event);

    int key = event->key();
    switch (key)
    {
    case Qt::Key_Control:
    {
        if (altPressCount > 0)
        {
            altPressCount=0;
            doublePressTimer.stop();
        }
        if (!doublePressTimer.isActive())
        {
            doublePressTimer.start(500);
        }
        ctrlPressCount++;
        if (ctrlPressCount == 2)
        {
            setHideDanmu(!GlobalObjects::mpvplayer->getDanmuHide());
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
    default:
        break;
    }

    Qt::KeyboardModifiers modifiers = event->modifiers();
    modifiers = modifiers & ~Qt::KeyboardModifier::ControlModifier;
    modifiers = modifiers & ~Qt::KeyboardModifier::AltModifier;
    modifiers = modifiers & ~Qt::KeyboardModifier::ShiftModifier;

    const QString pressKeyStr = QKeySequence((modifiers == Qt::KeyboardModifier::NoModifier ? event->modifiers() : 0) | event->key()).toString();
    KeyActionModel::instance()->runAction(pressKeyStr, KeyActionItem::KEY_PRESS, this);
}

void PlayerWindow::keyReleaseEvent(QKeyEvent *event)
{
    const QString pressKeyStr = QKeySequence(event->modifiers()|event->key()).toString();
    KeyActionModel::instance()->runAction(pressKeyStr, KeyActionItem::KEY_RELEASE, this);
}

void PlayerWindow::contextMenuEvent(QContextMenuEvent *)
{
    currentDanmu = GlobalObjects::danmuRender->danmuAt(mapFromGlobal(QCursor::pos()) * devicePixelRatioF());
    if (!currentDanmu && liveDanmuList->isVisible())
    {
        QModelIndex index = liveDanmuList->indexAt(liveDanmuList->mapFromGlobal(QCursor::pos()));
        currentDanmu = GlobalObjects::danmuRender->liveDanmuModel()->getDanmu(index);
    }
    if (currentDanmu.isNull()) return;
    ctxText->setText(currentDanmu->text);
    ctxBlockUser->setText(tr("Block User %1").arg(currentDanmu->sender));
    contexMenu->exec(QCursor::pos());
}

void PlayerWindow::closeEvent(QCloseEvent *)
{
    GlobalObjects::playlist->setCurrentPlayTime();
    GlobalObjects::appSetting->beginGroup("Play");
    GlobalObjects::appSetting->setValue("LiveVRange", liveVRange);
    GlobalObjects::appSetting->setValue("Volume",volumeSlider->value());
    GlobalObjects::appSetting->setValue("Mute",GlobalObjects::mpvplayer->getMute());
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
    if (!fileList.isEmpty())
    {
        GlobalObjects::playlist->addItems(fileList,QModelIndex());
        const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(fileList.first());
        if (curItem)
        {
            PlayContext::context()->playItem(curItem);
        }

    }
    for (const QString &dir : dirList)
    {
        GlobalObjects::playlist->addFolder(dir,QModelIndex());
    }
}

void PlayerWindow::wheelEvent(QWheelEvent *event)
{
    int val = volumeSlider->value();
    if ((val > 0 && val < volumeSlider->maximum()) || (val == 0 && event->angleDelta().y()>0) ||
        (val == volumeSlider->maximum() && event->angleDelta().y() < 0))
	{
		static bool inProcess = false;
		if (!inProcess)
		{
            inProcess = true;
            QApplication::sendEvent(volumeSlider, event);
			inProcess = false;
		}
	}
    showMessage(tr("Volume: %0").arg(volumeSlider->value()), "playerInfo");
}

// BEGIN: screen saver manage functions
#ifdef Q_OS_WIN

// MS Windows API

void setAwakeRequired()
{
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED
            | ES_SYSTEM_REQUIRED);
}

void unsetAwakeRequired()
{
    SetThreadExecutionState(ES_CONTINUOUS);
}

#elif defined(Q_OS_LINUX)

// Communicate to org.freedesktop.ScreenSaver with D-Bus
// works on most systems including KDE and Gnome.

#include <QDBusConnection>
#include <QDBusMessage>

namespace
{
void sendInhabitMessage(bool type)
{
    static bool inhabit_status = false;
    static quint32 cookie = 0;

    if (inhabit_status == type)
    {
        return;
    }

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected())
    {
        // Connot connect to D-Bus
        return;
    }

    // Create D-Bus Message
    QDBusMessage message = QDBusMessage::createMethodCall(
        "org.freedesktop.ScreenSaver"           // D-Bus service name
        , "/org/freedesktop/ScreenSaver"        // D-Bus object path
        , "org.freedesktop.ScreenSaver"         // D-Bus interface name
        , type ? "Inhibit" : "UnInhibit");      // D-Bus method name

    if (type)
    {
        // Method Arguments
        message << QCoreApplication::applicationName();
        message << QString("PlayingVideo"); // activity decription
    } else {
        message << cookie;
    }

    QDBusMessage reply = connection.call(message);

    if (reply.type() != QDBusMessage::ReplyMessage)
    {
        qDebug() << "Failed to send D-Bus message to screen saver";
		return;
    }
    
    inhabit_status = type;
	if (type)
        cookie = reply.arguments().at(0).toUInt();
    else
        cookie = 0;
}
}

void setAwakeRequired()
{
    sendInhabitMessage(true);
}

void unsetAwakeRequired()
{
    sendInhabitMessage(false);
}

#else

// Empty stub function for unknown OS
// TODO: implement for Mac

void setAwakeRequired() {}
void unsetAwakeRequired() {}

#endif
// END: screen saver manage functions


PlayerContent::PlayerContent(QWidget *parent):QWidget(parent)
{
    setMouseTracking(true);
    setObjectName(QStringLiteral("PlayerContent"));

    logo = new QLabel(this);
    GlobalObjects::iconfont->setPointSize(96);
    logo->setFont(*GlobalObjects::iconfont);
    logo->setText(QChar(0xe654));
    logo->setAlignment(Qt::AlignCenter);

    const int btnMinWidth = 200;
    QFont btnFont{GlobalObjects::normalFont, 12};
    QPushButton *openFileBtn = new QPushButton(tr("Open File"), this);
    openFileBtn->setObjectName(QStringLiteral("PlayerContentButton"));
    openFileBtn->setMinimumWidth(btnMinWidth);
    openFileBtn->setFont(btnFont);
    QPushButton *openUrlBtn = new QPushButton(tr("Open URL"), this);
    openUrlBtn->setObjectName(QStringLiteral("PlayerContentButton"));
    openUrlBtn->setMinimumWidth(btnMinWidth);
    openUrlBtn->setFont(btnFont);
    QHBoxLayout *btnHLayout = new QHBoxLayout;
    btnHLayout->setContentsMargins(0, 0, 0, 0);
    btnHLayout->addStretch(1);
    btnHLayout->addWidget(openFileBtn);
    btnHLayout->addSpacing(64);
    btnHLayout->addWidget(openUrlBtn);
    btnHLayout->addStretch(1);

    recentTip = new QLabel(tr("Recently Played"), this);
    recentTip->setObjectName(QStringLiteral("RecentlyPlayedTip"));

    recentItemsContainer = new QWidget(this);
    QGridLayout *itemGLayout = new QGridLayout(recentItemsContainer);
    itemGLayout->setContentsMargins(0, 0, 0, 0);
    itemGLayout->setColumnStretch(0, 1);
    itemGLayout->setColumnStretch(1, 1);
    itemGLayout->setRowStretch(0, 1);
    itemGLayout->setRowStretch(1, 1);
    for (int i = 0; i < GlobalObjects::playlist->maxRecentItems; ++i)
    {
        RecentItem *recentItem = new RecentItem(recentItemsContainer);
        itemGLayout->addWidget(recentItem, i / 2, i % 2);
        items.append(recentItem);
    }

    QVBoxLayout *pcVLayout = new QVBoxLayout(this);
    pcVLayout->addStretch(1);
    pcVLayout->addWidget(logo);
    pcVLayout->addSpacing(40);
    pcVLayout->addLayout(btnHLayout);
    pcVLayout->addSpacing(30);
    pcVLayout->addWidget(recentTip);
    pcVLayout->addWidget(recentItemsContainer);
    pcVLayout->addStretch(1);
    refreshItems();

    logo->installEventFilter(this);

    QObject::connect(openFileBtn, &QPushButton::clicked, this, &PlayerContent::openFile);
    QObject::connect(openUrlBtn, &QPushButton::clicked, this, &PlayerContent::openUrl);
    QObject::connect(GlobalObjects::playlist, &PlayList::recentItemsUpdated, this, &PlayerContent::refreshItems);
    QObject::connect(GlobalObjects::playlist, &PlayList::recentItemInfoUpdated, this, [=](int index){
        auto &recent = GlobalObjects::playlist->recent();
        if (index >= recent.size()) return;
        for (RecentItem *rItem : items)
        {
            if (rItem->path == recent[index].path)
            {
                rItem->setData(recent[index]);
                break;
            }
        }
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::showRecentChanged, this, [=](bool on){
        auto &recent = GlobalObjects::playlist->recent();
        logo->setVisible(recent.empty() || !GlobalObjects::mpvplayer->getShowRecent());
        recentTip->setVisible(!recent.empty() && GlobalObjects::mpvplayer->getShowRecent());
        recentItemsContainer->setVisible(!recent.empty() && GlobalObjects::mpvplayer->getShowRecent());
    });

}

void PlayerContent::refreshItems()
{
    int i = 0;
    const int maxRecentCount = GlobalObjects::playlist->maxRecentItems;
    auto &recent = GlobalObjects::playlist->recent();
    for (; i < maxRecentCount && i<recent.count(); ++i)
    {
        items[i]->setData(recent[i]);
    }
    while (i < maxRecentCount)
    {
        items[i++]->hide();
    }
    logo->setVisible(recent.empty() || !GlobalObjects::mpvplayer->getShowRecent());
    recentTip->setVisible(!recent.empty() && GlobalObjects::mpvplayer->getShowRecent());
    recentItemsContainer->setVisible(!recent.empty() && GlobalObjects::mpvplayer->getShowRecent());
}

bool PlayerContent::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == logo && event->type() == QEvent::MouseButtonRelease)
    {
        openFile();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void PlayerContent::openFile()
{
    const QStringList files = QFileDialog::getOpenFileNames(this,tr("Select one or more media files"),"",
        QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
    if (!files.isEmpty())
    {
        GlobalObjects::playlist->addItems(files, QModelIndex());
        const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(files.first());
        if (curItem)
        {
            PlayContext::context()->playItem(curItem);
        }
    }
}

void PlayerContent::openUrl()
{
    LineInputDialog input(tr("Open URL"), tr("URL"), "", "DialogSize/PlayerOpenURL", false, this);
    if (QDialog::Accepted == input.exec())
    {
        GlobalObjects::playlist->addURL({input.text}, QModelIndex());
        PlayContext::context()->playURL(input.text);
    }
}

RecentItem::RecentItem(QWidget *parent):QWidget(parent)
{
    setMinimumHeight(96);
    setObjectName(QStringLiteral("RecentItem"));
    coverLabel = new QLabel(this);
    coverLabel->setFixedSize(128, 72);
    coverLabel->setScaledContents(true);

    titleLabel = new QLabel(this);
    titleLabel->setFont(QFont(GlobalObjects::normalFont, 9));
    timeLabel = new QLabel(this);
    timeLabel->setFont(QFont(GlobalObjects::normalFont, 9));
    titleLabel->setObjectName(QStringLiteral("RecentItemLabel"));
    timeLabel->setObjectName(QStringLiteral("RecentItemTimeLabel"));
    deleteItem = new QPushButton(this);
    deleteItem->setObjectName(QStringLiteral("RecentItemDeleteButton"));
    QObject::connect(deleteItem, &QPushButton::clicked, this, [this](){
        GlobalObjects::playlist->removeRecentItem(path);
    });
    GlobalObjects::iconfont->setPointSize(10);
    deleteItem->setFont(*GlobalObjects::iconfont);
    deleteItem->setText(QChar(0xe60b));
    deleteItem->hide();

    QWidget *textContainer = new QWidget(this);
    textContainer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    QVBoxLayout *textVLayout = new QVBoxLayout(textContainer);
    textVLayout->setContentsMargins(0, 2, 0, 0);
    textVLayout->addWidget(titleLabel);
    textVLayout->addWidget(timeLabel);
    textVLayout->addStretch(1);
    textVLayout->addWidget(deleteItem, 0, Qt::AlignRight | Qt::AlignVCenter);

    QHBoxLayout *itemHLayout = new QHBoxLayout(this);
    itemHLayout->addWidget(coverLabel);
    itemHLayout->addWidget(textContainer);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
}

void RecentItem::setData(const RecentlyPlayedItem &item)
{
    path = item.path;
    titleLabel->setText(item.title);
    titleLabel->setToolTip(item.title);
    titleLabel->adjustSize();
    if (item.playTimeState == PlayListItem::PlayState::FINISH)
    {
        timeLabel->setText(tr("Finished"));
    }
    else if (item.duration > 0 && item.playtime > 0)
    {
        const int d_min = item.duration / 60;
        const int d_sec = item.duration - d_min * 60;
        const int p_min = item.playtime / 60;
        const int p_sec = item.playtime - p_min * 60;
        timeLabel->setText(tr("%1:%2/%3:%4").arg(p_min, 2, 10, QChar('0')).arg(p_sec, 2, 10, QChar('0')).
                           arg(d_min, 2, 10, QChar('0')).arg(d_sec, 2, 10, QChar('0')));
    }
    timeLabel->adjustSize();
    if (item.stopFrame.isNull())
    {
        coverLabel->setPixmap(nullCover());
    }
    else
    {
        coverLabel->setPixmap(QPixmap::fromImage(item.stopFrame));
    }
    coverLabel->adjustSize();
    adjustSize();
    show();
}

QSize RecentItem::sizeHint() const
{
    return layout()->sizeHint();
}

const QPixmap &RecentItem::nullCover()
{
    static QPixmap nullCoverPixmap;
    if (nullCoverPixmap.isNull())
    {
        const int w = 160, h = 90;
        const float pxR = GlobalObjects::context()->devicePixelRatioF;
        nullCoverPixmap = QPixmap(w * pxR, h * pxR);
        nullCoverPixmap.setDevicePixelRatio(pxR);
        nullCoverPixmap.fill(Qt::transparent);
        QPainter painter(&nullCoverPixmap);
        painter.setRenderHints(QPainter::Antialiasing, true);
        painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
        QPainterPath path;
        path.addRoundedRect(0, 0, w, h, 8, 8);
        painter.setClipPath(path);
        painter.drawPixmap(QRect(0, 0, w, h), QPixmap(":/res/images/null-cover-recent.png"));
    }
    return nullCoverPixmap;
}

void RecentItem::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)
    {
        const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(path);
        if (curItem)
        {
            PlayContext::context()->playItem(curItem);
        }
    }
    event->accept();
}

void RecentItem::enterEvent(QEnterEvent *event)
{
    deleteItem->show();
    QWidget::enterEvent(event);
}

void RecentItem::leaveEvent(QEvent *event)
{
    deleteItem->hide();
    QWidget::leaveEvent(event);
}

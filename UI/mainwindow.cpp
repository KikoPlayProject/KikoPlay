#include "mainwindow.h"
#include "globalobjects.h"
#include <QFontDatabase>
#include <QSplitter>
#include <QMenu>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QButtonGroup>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QSystemTrayIcon>
#include <QProgressBar>
#ifdef Q_OS_WIN
#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#endif
#include "about.h"
#include "poolmanager.h"
#include "checkupdate.h"
#include "tip.h"
#include "stylemanager.h"
#include "settings.h"
#include "inputdialog.h"
#include "logwindow.h"
#include "scriptplayground.h"
#include "luatableviewer.h"
#include "widgets/backgroundwidget.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Play/playcontext.h"
#include "appmenu.h"
#include "appbar.h"

MainWindow::MainWindow(QWidget *parent)
    : CFramelessWindow(parent),hasBackground(false),hasCoverBg(false),curPage(0),listWindowWidth(0),
      isMini(false),hideToTrayType(HideToTrayType::NONE)
{
    setObjectName(QStringLiteral("MainWindow"));
    Notifier::getNotifier()->addNotify(Notifier::MAIN_DIALOG_NOTIFY, this);
    setupUI();
    // setWindowIcon(QIcon(":/res/kikoplay.ico"));
    QRect defaultGeo(0,0,800*logicalDpiX()/96,480*logicalDpiX()/96), defaultMiniGeo(0,0,200*logicalDpiX()/96, 200*logicalDpiY()/96);
    defaultGeo.moveCenter(QApplication::desktop()->geometry().center());
	defaultMiniGeo.moveCenter(QApplication::desktop()->geometry().center());
    setGeometry(GlobalObjects::appSetting->value("MainWindow/Geometry",defaultGeo).toRect());
    setMinimumSize(120*logicalDpiX()/96, 100*logicalDpiY()/96);
    miniGeo = GlobalObjects::appSetting->value("MainWindow/miniGeometry", defaultMiniGeo).toRect();
    originalGeo=geometry();
    listWindowWidth = GlobalObjects::appSetting->value("MainWindow/ListWindowWidth", 0).toInt();
    if(listWindowWidth == 0)
    {
        listWindowWidth = 200*logicalDpiX()/96;
    }
    hideToTrayType = static_cast<HideToTrayType>(GlobalObjects::appSetting->value("MainWindow/hideToTrayType", 0).toInt());
    trayIcon = new QSystemTrayIcon(windowIcon(), this);
    trayIcon->setToolTip("KikoPlay");
    QObject::connect(trayIcon, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason){
        if(QSystemTrayIcon::Trigger == reason)
        {
            show();
			if (isMinimized()) showNormal();
            raise();
            trayIcon->hide();
        }
    });
    QMenu *trayMenu = new QMenu(this);
    QAction *actTrayExit=new QAction(tr("Exit"), trayMenu);
    QObject::connect(actTrayExit,&QAction::triggered, this, &MainWindow::close);
    trayMenu->addAction(actTrayExit);
    trayIcon->setContextMenu(trayMenu);
    QVariant splitterState(GlobalObjects::appSetting->value("MainWindow/SplitterState"));
    if(!splitterState.isNull())
        playSplitter->restoreState(splitterState.toByteArray());
    if(!GlobalObjects::appSetting->value("MainWindow/ListVisibility",true).toBool())
        listWindow->hide();
    listShowState=!listWindow->isHidden();
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::stateChanged,[this](MPVPlayer::PlayState state){
#ifdef Q_OS_WIN
        if(state==MPVPlayer::Play && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            winTaskbarProgress->show();
            winTaskbarProgress->resume();
        }
        else if(state==MPVPlayer::Pause && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            winTaskbarProgress->show();
            winTaskbarProgress->pause();
        }
        else if(state==MPVPlayer::Stop)
        {
            winTaskbarProgress->hide();
        }
#endif
        if(state==MPVPlayer::Stop)
        {
            auto geo=originalGeo;
			if (isFullScreen())
			{
				widgetTitlebar->show();
                playerWindow->toggleFullScreenState(false);
				showNormal();
			}
            if(isMaximized())
            {
                showNormal();
            }
			if (listShowState)listWindow->show();
			else listWindow->hide();
            playerWindow->toggleListCollapseState(listShowState);
            setGeometry(geo);
        }
#ifdef Q_OS_WIN
        setScreenSave(!(state==MPVPlayer::Play && GlobalObjects::playlist->getCurrentItem()!=nullptr));
#endif
    });
#ifdef Q_OS_WIN
	QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::positionChanged, this, [this](int val) {
        if(GlobalObjects::playlist->getCurrentItem()!=nullptr)
            winTaskbarProgress->setValue(val);
	});
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::durationChanged,this, [this](int duration){
        winTaskbarProgress->setRange(0, duration);
    });
#endif
    if(GlobalObjects::appSetting->value("MainWindow/ShowTip",true).toBool())
    {
        GlobalObjects::appSetting->setValue("MainWindow/ShowTip",false);
        QTimer::singleShot(0,[this](){
            Tip tip(buttonIcon);
            QRect geo(0,0,400,400);
            geo.moveCenter(this->geometry().center());
            tip.move(geo.topLeft());
            tip.exec();
            QDesktopServices::openUrl(QUrl("https://kikoplayproject.github.io/about"));
        });
    }
}

void MainWindow::setBackground(const QString &path, const QColor &color)
{
    bool forceRefeshQSS = color.isValid() && color != themeColor;
    themeColor = color;
    setBackground(path, forceRefeshQSS);
}

void MainWindow::setBgDarkness(int val)
{
    bgWidget->setBgDarkness(val);
    curDarkness = val;
}

void MainWindow::setThemeColor(const QColor &color)
{
    themeColor = color;
    if(themeColor.isValid() && hasBackground)
        StyleManager::getStyleManager()->setQSS(StyleManager::BG_COLOR, themeColor);
    else if(hasBackground)
        StyleManager::getStyleManager()->setQSS(StyleManager::DEFAULT_BG);
    else
        StyleManager::getStyleManager()->setQSS(StyleManager::NO_BG);
}

void MainWindow::setHideToTrayType(HideToTrayType type)
{
    hideToTrayType = type;
}

void MainWindow::setupUI()
{

    QWidget *centralWidget = new QWidget(this);
    bgWidget = new BackgroundWidget(centralWidget);
    curDarkness = bgWidget->bgDarkness();
    QWidget *centralContainer = new QWidget(centralWidget);
    QStackedLayout *centralSLayout = new QStackedLayout(centralWidget);
    centralSLayout->setStackingMode(QStackedLayout::StackAll);
    centralSLayout->addWidget(centralContainer);
    centralSLayout->addWidget(bgWidget);

    QVBoxLayout *verticalLayout = new QVBoxLayout(centralContainer);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setSpacing(0);
//------title bar
    widgetTitlebar = new DropableWidget(centralContainer);
    widgetTitlebar->setAcceptDrops(true);
    widgetTitlebar->setObjectName(QStringLiteral("widgetTitlebar"));
    QFont normalFont(GlobalObjects::normalFont,12);

    buttonIcon = new QToolButton(widgetTitlebar);
    buttonIcon->setFont(normalFont);
#ifdef Q_OS_WIN
    buttonIcon->setText(" KikoPlay ");
#endif
#ifdef Q_OS_MACOS
    buttonIcon->setProperty("hideMenuIndicator", true);
#endif
    buttonIcon->setObjectName(QStringLiteral("LogoButton"));
    buttonIcon->setIcon(QIcon(":/res/images/kikoplay-3.png"));
    buttonIcon->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    buttonIcon->setPopupMode(QToolButton::InstantPopup);

    QAction *act_poolManager=new QAction(tr("Danmu Pool Manager"), this);
    QObject::connect(act_poolManager,&QAction::triggered,[this](){
        PoolManager poolManage(buttonIcon);
        poolManage.exec();

    });
    buttonIcon->addAction(act_poolManager);

    QAction *act_Settingse=new QAction(tr("Settings"), this);
    QObject::connect(act_Settingse,&QAction::triggered,[this](){
        Settings settings(Settings::PAGE_UI, buttonIcon);
        settings.exec();
    });
    buttonIcon->addAction(act_Settingse);

    scriptPlayground = nullptr;
    QAction *act_ScriptPlayground=new QAction(tr("Script Playground"), this);
    QObject::connect(act_ScriptPlayground,&QAction::triggered,[this](){
        if(!scriptPlayground)
        {
            scriptPlayground = new ScriptPlayground(this);
        }
        scriptPlayground->show();
    });
    buttonIcon->addAction(act_ScriptPlayground);

    logWindow = nullptr;
    QAction *act_ShowLogCenter=new QAction(tr("Log Center"), this);
    QObject::connect(act_ShowLogCenter,&QAction::triggered,[this](){
        if(!logWindow)
        {
            logWindow=new LogWindow(this);
            QObject::connect(logWindow, &LogWindow::destroyed, [this](){
               logWindow = nullptr;
            });
        }
        logWindow->show();
    });
    buttonIcon->addAction(act_ShowLogCenter);

    QAction *act_checkUpdate=new QAction(tr("Check For Updates"), this);
    QObject::connect(act_checkUpdate,&QAction::triggered,[this](){
        CheckUpdate checkUpdate(buttonIcon);
        checkUpdate.exec();
    });
    buttonIcon->addAction(act_checkUpdate);

    QAction *act_useTip=new QAction(tr("Usage Tip"), this);
    QObject::connect(act_useTip,&QAction::triggered,[this](){
        Tip tip(buttonIcon);
        tip.resize(500*logicalDpiX()/96, 400*logicalDpiY()/96);
        tip.exec();
    });
    buttonIcon->addAction(act_useTip);

    QAction *act_about=new QAction(tr("About"), this);
    QObject::connect(act_about,&QAction::triggered,[this](){
        About about(buttonIcon);
        QRect geo(0,0,400,400);
        geo.moveCenter(this->geometry().center());
        about.move(geo.topLeft());
        about.exec();
    });
    buttonIcon->addAction(act_about);
    QAction *act_exit=new QAction(tr("Exit"),this);
    QObject::connect(act_exit,&QAction::triggered, this, &MainWindow::close);
    buttonIcon->addAction(act_exit);

    QStringList pageButtonTexts = {
        tr("Player"),
        tr("Library"),
        tr("Download")
    };
    QToolButton **infoBtnPtrs[] = {
        &buttonPage_Play, &buttonPage_Library, &buttonPage_Downlaod
    };
#ifdef Q_OS_WIN
    #define pageBtnObjName "PageButton"
#else
    #define pageBtnObjName "PageButton_O"
#endif
    QButtonGroup *btnGroup=new QButtonGroup(this);
    for(int i = 0; i < pageButtonTexts.size(); ++i)
    {
        *infoBtnPtrs[i] = new QToolButton(this);
        QToolButton *tb = *infoBtnPtrs[i];
        tb->setFont(normalFont);
        tb->setText(pageButtonTexts[i]);
        tb->setCheckable(true);
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setObjectName(QStringLiteral(pageBtnObjName));
        btnGroup->addButton(tb, i);
    }
    buttonPage_Play->setChecked(true);
    QObject::connect(btnGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[this](int id, bool checked){
        if(checked)
        {
            contentStackLayout->setCurrentIndex(id);
            if(id == 1 && hasCoverBg)
            {
                if(!coverPixmap.isNull()) bgWidget->setBackground(coverPixmap);
                bgWidget->setBgDarkness(curDarkness + 10);
                bgWidget->setBlurAnimation(20., 60.);
            }
            else if(curPage == 1 && hasCoverBg)
            {
                bgWidget->setBackground(bgImg);
                bgWidget->setBgDarkness(curDarkness);
                bgWidget->setBlurAnimation(60., 0.);
            }
            curPage = id;
            if(downloadToolProgress->value() < 100)
            {
                downloadToolProgress->setVisible(curPage != 2);
            }
        }
    });

    downloadToolProgress = new QProgressBar(this);
    downloadToolProgress->setObjectName(QStringLiteral("DownloadToolProgress"));
    downloadToolProgress->setRange(0, 100);
    downloadToolProgress->setTextVisible(false);
    QVBoxLayout *toolProgressVLayout=new QVBoxLayout();
    toolProgressVLayout->addStretch(1);
    toolProgressVLayout->addWidget(downloadToolProgress, 0, Qt::AlignHCenter);
    toolProgressVLayout->setContentsMargins(0, 0, 0, 0);
    buttonPage_Downlaod->setLayout(toolProgressVLayout);


    QVBoxLayout *pageVerticalLayout = new QVBoxLayout();
    pageVerticalLayout->setContentsMargins(0,0,0,0);
#ifdef Q_OS_WIN
    pageVerticalLayout->addStretch(1);
#endif
    QHBoxLayout *pageHLayout = new QHBoxLayout();
    pageHLayout->setContentsMargins(0,0,0,0);
    pageHLayout->addWidget(buttonPage_Play);
    pageHLayout->addWidget(buttonPage_Library);
    pageHLayout->addWidget(buttonPage_Downlaod);
    pageVerticalLayout->addLayout(pageHLayout);


    appBar = new AppBar(widgetTitlebar);

    GlobalObjects::iconfont->setPointSize(10);
    appButton=new QToolButton(widgetTitlebar);
    appButton->setFont(*GlobalObjects::iconfont);
    appButton->setText(QChar(0xe63d));
    appButton->setObjectName(QStringLiteral("ControlButton"));
    appButton->setProperty("hideMenuIndicator", true);
    appButton->setMenu(new AppMenu(appButton, this));
    appButton->setPopupMode(QToolButton::InstantPopup);

    minButton=new QToolButton(widgetTitlebar);
    minButton->setFont(*GlobalObjects::iconfont);
    minButton->setText(QChar(0xe651));
    minButton->setObjectName(QStringLiteral("ControlButton"));
    QObject::connect(minButton,&QToolButton::clicked,this, [=](){
        if(hideToTrayType == HideToTrayType::MINIMIZE)
        {
            this->hide();
            trayIcon->show();
        }
        else
        {
            showMinimized();
        }
    });

    maxButton=new QToolButton(widgetTitlebar);
    maxButton->setFont(*GlobalObjects::iconfont);
    maxButton->setText(QChar(0xe93c));
    maxButton->setObjectName(QStringLiteral("ControlButton"));
    QObject::connect(maxButton,&QToolButton::clicked,[this](){
       if(this->isMaximized())
       {
           maxButton->setText(QChar(0xe93c));
           this->showNormal();
       }
       else
       {
           maxButton->setText(QChar(0xe93d));
           this->showMaximized();
       }
    });

    closeButton=new QToolButton(widgetTitlebar);
    closeButton->setFont(*GlobalObjects::iconfont);
    closeButton->setText(QChar(0xe60b));
    closeButton->setObjectName(QStringLiteral("closelButton"));
    QObject::connect(closeButton,&QToolButton::clicked,[this](){
        if(hideToTrayType == HideToTrayType::CLOSE)
        {
            this->hide();
            trayIcon->show();
        }
        else
        {
            this->close();
        }
    });
    QHBoxLayout *layout = new QHBoxLayout(widgetTitlebar);
    layout->setSpacing(0);
#ifdef Q_OS_WIN
    layout->setContentsMargins(8*logicalDpiX()/96,0,2*logicalDpiY()/96,0);
    layout->addWidget(buttonIcon);
    layout->addSpacing(20*logicalDpiX()/96);
    layout->addLayout(pageVerticalLayout);
    layout->addStretch(1);
    layout->addWidget(appBar);
    layout->addSpacing(10*logicalDpiX()/96);
    layout->addWidget(appButton);
    layout->addWidget(minButton);
    layout->addWidget(maxButton);
    layout->addWidget(closeButton);
#else
    layout->setContentsMargins(0,0,4*logicalDpiX()/96,0);
    layout->addLayout(pageVerticalLayout);
    layout->addStretch(1);
    layout->addWidget(buttonIcon);
    appButton->hide();
    minButton->hide();
    maxButton->hide();
    closeButton->hide();
#endif
    verticalLayout->addWidget(widgetTitlebar);

    contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupPlayPage());
    contentStackLayout->addWidget(setupLibraryPage());
    contentStackLayout->addWidget(setupDownloadPage());
    listWindow->show();

    verticalLayout->addLayout(contentStackLayout);


    setCentralWidget(centralWidget);
    setTitleBar(widgetTitlebar);
	setFocusPolicy(Qt::StrongFocus);

    QObject::connect(widgetTitlebar, &DropableWidget::fileDrop, [this](const QString &url){
       QFileInfo fi(url);
       const QStringList imageFormats{"jpg", "png"};
       if(imageFormats.contains(fi.suffix().toLower())){
           QStringList historyBgs = GlobalObjects::appSetting->value("MainWindow/HistoryBackgrounds").toStringList();
           historyBgs.removeOne(url);
           historyBgs.insert(0, url);
           if(historyBgs.count()>10) historyBgs.removeLast();
           GlobalObjects::appSetting->setValue("MainWindow/HistoryBackgrounds", historyBgs);
           setBackground(url);
       }
       else {
           setBackground("");
       }
    });
    if(GlobalObjects::appSetting->value("MainWindow/CustomColor", false).toBool())
    {
        themeColor = GlobalObjects::appSetting->value("MainWindow/CustomColorHSV", QColor::fromHsv(180, 255, 100)).value<QColor>();
    }
    StyleManager::getStyleManager()->setCondVariable("DarkMode", GlobalObjects::appSetting->value("MainWindow/DarkMode", false).toBool(), false);
    setBackground(GlobalObjects::appSetting->value("MainWindow/Background", "").toString(), true, false);
}

void MainWindow::switchToPlay(const QString &fileToPlay)
{
    GlobalObjects::playlist->addItems(QStringList()<<fileToPlay,QModelIndex());
    const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(fileToPlay);
    if (curItem)
    {
        PlayContext::context()->playItem(curItem);
        buttonPage_Play->click();
    }
}

void MainWindow::setBackground(const QString &imagePath, bool forceRefreshQSS, bool showAnimation)
{
    bool refreshQSS = false;
    if(!imagePath.isEmpty() && QFile::exists(imagePath))
    {
        GlobalObjects::appSetting->setValue("MainWindow/Background", imagePath);
        bgImg = QImage(imagePath);
        if(!hasCoverBg || curPage != 1)
        {
            bgWidget->setBackground(bgImg);
            if(showAnimation) bgWidget->setBlurAnimation(60., 0., 800);
        }
        refreshQSS = !hasBackground;
        hasBackground = true;
    }
    else
    {
        GlobalObjects::appSetting->setValue("MainWindow/Background", "");
        bgImg = QImage();
        if(!hasCoverBg || curPage != 1)
        {
            bgWidget->setBackground(bgImg);
        }
        refreshQSS = hasBackground;
        hasBackground = false;
    }
    if(refreshQSS || forceRefreshQSS)
    {
        if(themeColor.isValid() && hasBackground)
            StyleManager::getStyleManager()->setQSS(StyleManager::BG_COLOR, themeColor);
        else if(hasBackground)
            StyleManager::getStyleManager()->setQSS(StyleManager::DEFAULT_BG);
        else
            StyleManager::getStyleManager()->setQSS(StyleManager::NO_BG);
    }

}

QWidget *MainWindow::setupPlayPage()
{
    QWidget *page_play=new QWidget(centralWidget());
    page_play->setObjectName(QStringLiteral("widgetPagePlay"));
    page_play->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);

    playSplitter = new QSplitter(page_play);
    QVBoxLayout *playVerticalLayout=new QVBoxLayout(page_play);
    playVerticalLayout->setContentsMargins(1,0,1,1);
    playVerticalLayout->addWidget(playSplitter);
    playSplitter->setHandleWidth(1);

    playerWindow=new PlayerWindow();
    playerWindow->setMouseTracking(true);
//#ifdef Q_OS_WIN
    QWindow *native_wnd  = QWindow::fromWinId(playerWindow->winId());
    QWidget *playerWindowWidget=QWidget::createWindowContainer(native_wnd);
    playerWindowWidget->setContentsMargins(1,0,1,1);
    playerWindowWidget->setMouseTracking(true);
    playerWindowWidget->setParent(playSplitter);
    playerWindow->show();
//#else
//    QWidget *playerWindowWidget = playerWindow;
//    playerWindowWidget->setParent(playSplitter);
//    playerWindow->show();
//#endif

    listWindow=new ListWindow(playSplitter);
    QObject::connect(playerWindow,&PlayerWindow::toggleListVisibility,[this](){
        if(listWindow->isHidden())
        {
            if(!isMaximized() && !isFullScreen())
            {
                resize(listWindowWidth+width(),height());
            }
            listWindow->show();
        }
        else
        {
            listWindowWidth=listWindow->width();
            listWindow->hide();
            if(!isMaximized() && !isFullScreen())
                resize(width()-listWindowWidth,height());
        }
        if(GlobalObjects::playlist->getCurrentItem()==nullptr)
            listShowState=!listWindow->isHidden();
    });
    QObject::connect(playerWindow,&PlayerWindow::showFullScreen,[this,playVerticalLayout](bool on){
        static bool isMax, isShowPlaylist;
        if(on)
        {
            isShowPlaylist=!listWindow->isHidden();
            isMax=isMaximized();
            widgetTitlebar->hide();
            showFullScreen();
            listWindow->hide();
            playerWindow->toggleListCollapseState(false);
            playVerticalLayout->setContentsMargins(0,0,0,0);
            playerWindow->activateWindow();
        }
        else
        {
            widgetTitlebar->show();
            isShowPlaylist?listWindow->show():listWindow->hide();
            playerWindow->toggleListCollapseState(isShowPlaylist);
            isMax?showMaximized():showNormal();
            playVerticalLayout->setContentsMargins(1,0,1,1);
            playerWindow->activateWindow();
        }
    });
    QObject::connect(playerWindow, &PlayerWindow::setStayOnTop, this, &CFramelessWindow::setOnTop);
    QObject::connect(playerWindow,&PlayerWindow::resizePlayer,[this](double w,double h,double aspectRatio){
        if (isMaximized() || isFullScreen())return;
        QRect windowGeo(geometry()),desktopGeo(QApplication::desktop()->availableGeometry(windowGeo.center()));
        double extraWidth=windowGeo.width()-playerWindow->width(),extraHeight=windowGeo.height()-playerWindow->height();
        double nw=extraWidth+w,nh=extraHeight+h;
        if(nw > desktopGeo.width())
        {
            nw = desktopGeo.width();
            w = nw - extraWidth;
            h = w / aspectRatio;
            nh = h + extraHeight;
        }
        if(nw < minimumWidth())
        {
            nw = minimumWidth();
            w = nw - extraWidth;
            h = w / aspectRatio;
            nh = h + extraHeight;
        }
        if(nh > desktopGeo.height())
        {
            nh = desktopGeo.height();
            h = nh - extraHeight;
            w = h * aspectRatio;
            nw = w + extraWidth;
        }
        if(nh < minimumHeight())
        {
            nh = minimumHeight();
            h = nh - extraHeight;
            w = h * aspectRatio;
            nw = w + extraWidth;
        }
        windowGeo.setHeight(nh);
        windowGeo.setWidth(nw);
        if(desktopGeo.contains(windowGeo))
            resize(nw,nh);
        else
        {
            windowGeo.moveCenter(desktopGeo.center());
            setGeometry(windowGeo);
        }
    });
    QObject::connect(playerWindow, &PlayerWindow::miniMode, this, [this, playVerticalLayout](bool on){
        static bool isMax, isShowPlaylist;
        static QRect geo;
        if(on)
        {
#ifndef Q_OS_WIN
            setWindowFlags(Qt::CustomizeWindowHint);
            setAttribute(Qt::WA_Hover);
            setMouseTracking(true);
            installEventFilter(this);
            show();
#endif
            geo = geometry();
            isShowPlaylist=!listWindow->isHidden();
            isMax=isMaximized();
            widgetTitlebar->hide();
            listWindow->hide();
            playerWindow->toggleListCollapseState(false);
            setGeometry(miniGeo);
            playVerticalLayout->setContentsMargins(2,2,2,2);
            isMini = true;
        }
        else
        {
#ifndef Q_OS_WIN
            setWindowFlags (windowFlags() & ~Qt::CustomizeWindowHint);
            show();
#endif
			isMini = false;
            widgetTitlebar->show();
            isShowPlaylist?listWindow->show():listWindow->hide();
            playerWindow->toggleListCollapseState(isShowPlaylist);
            isMax?showMaximized():showNormal();
            setGeometry(geo);
            playVerticalLayout->setContentsMargins(1,0,1,1);
        }
    });

    QObject::connect(playerWindow, &PlayerWindow::beforeMove, this, [this](const QPoint &pressPos){
        miniPressPos = pressPos - pos();
    });
    QObject::connect(playerWindow, &PlayerWindow::moveWindow, this, [this](const QPoint &cpos){
        move(cpos-miniPressPos);
    });
    QObject::connect(playerWindow, &PlayerWindow::refreshPool, this, [this](){
        Notifier *notifier = Notifier::getNotifier();
        if(listWindow->isHidden())
        {
            notifier->showMessage(Notifier::PLAYER_NOTIFY, tr("Updating..."));
        }
        int c = listWindow->updateCurrentPool();
        if(listWindow->isHidden())
        {
            notifier->showMessage(Notifier::PLAYER_NOTIFY, tr("Add %1 Danmu").arg(c));
        }
    });
    QObject::connect(playerWindow, &PlayerWindow::showMPVLog, this, [this](){
        if(!logWindow)
        {
            logWindow=new LogWindow(this);
            QObject::connect(logWindow, &LogWindow::destroyed, [this](){
               logWindow = nullptr;
            });
        }
        logWindow->show(Logger::LogType::MPV);
    });

    playSplitter->addWidget(playerWindowWidget);
    playSplitter->addWidget(listWindow);
    playSplitter->setStretchFactor(0,1);
    playSplitter->setStretchFactor(1,0);
    playSplitter->setCollapsible(0,false);
    playSplitter->setCollapsible(1,false);
    listWindow->hide();
    //playerWindowWidget->setMinimumSize(100*logicalDpiX()/96,100*logicalDpiY()/96);
    //listWindow->setMinimumSize(200*logicalDpiX()/96,350*logicalDpiY()/96);

    return page_play;

}

QWidget *MainWindow::setupLibraryPage()
{
    library=new LibraryWindow(this);
    library->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QObject::connect(library,&LibraryWindow::playFile,this,&MainWindow::switchToPlay);
    QObject::connect(library,&LibraryWindow::switchBackground,this,[this](const QPixmap &pixmap, bool setPixmap){
        if(!setPixmap)
        {
            hasCoverBg = false;
            bgWidget->setBackground(bgImg);
            bgWidget->setBgDarkness(curDarkness);
            bgWidget->setBlurAnimation(60., 0.);
        }
        else
        {
            hasCoverBg = true;
            coverPixmap = pixmap;
            if(!pixmap.isNull()) bgWidget->setBackground(pixmap);
            bgWidget->setBgDarkness(curDarkness + 10);
            bgWidget->setBlurAnimation(20., 60.);
        }
    });
    return library;
}

QWidget *MainWindow::setupDownloadPage()
{
    download=new DownloadWindow(this);
    download->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QObject::connect(download, &DownloadWindow::playFile, this, &MainWindow::switchToPlay);
    QObject::connect(download, &DownloadWindow::totalProgressUpdate, this, [=](int progress){
        downloadToolProgress->setValue(progress);
        if(curPage != 2)
        {
            downloadToolProgress->setVisible(progress < 100);
        }
    });
    return download;
}

QVariant MainWindow::showDialog(const QVariant &inputs)
{
    QVariantMap optMap = inputs.toMap();
    if(optMap.contains("_type") && optMap["_type"]=="luaTabelViewer")
    {
        LuaTableModel *model = static_cast<LuaTableModel *>(optMap["_modelPtr"].value<void *>());
        LuaTableViewer viewer(model, this);
        viewer.exec();
        return "";
    }
    QString title = optMap.value("title", "KikoPlay").toString();
    QString tip = optMap.value("tip").toString();
    bool hasText = optMap.contains("text"), hasImage = optMap.contains("image");
    if(!hasText && !hasImage)
    {
        InputDialog dialog(title, tip, this);
        int ret = dialog.exec();
        return QStringList{ret==QDialog::Accepted?"accept":"reject", ""};
    }
    else if(hasText && !hasImage)
    {
        InputDialog dialog(title, tip,  optMap.value("text").toString(), true, this);
        int ret = dialog.exec();
        return ret==QDialog::Accepted? QStringList({"accept", dialog.text}):QStringList({"reject", ""});
    }
    else if(!hasText && hasImage)
    {
        InputDialog dialog(optMap.value("image").toByteArray(), title, tip, this);
        int ret = dialog.exec();
        return QStringList{ret==QDialog::Accepted?"accept":"reject", ""};
    }
    else
    {
        InputDialog dialog(optMap.value("image").toByteArray(), title, tip,  optMap.value("text").toString(), this);
        int ret = dialog.exec();
        return ret==QDialog::Accepted? QStringList({"accept", dialog.text}):QStringList({"reject", ""});
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(hideToTrayType == HideToTrayType::CLOSE)
    {
        this->hide();
        trayIcon->show();
        event->ignore();
        return;
    }
	GlobalObjects::appSetting->setValue("MainWindow/miniGeometry", miniGeo);
    GlobalObjects::appSetting->setValue("MainWindow/hideToTrayType", static_cast<int>(hideToTrayType));
    GlobalObjects::appSetting->setValue("MainWindow/ListWindowWidth", listWindowWidth);
    if(GlobalObjects::playlist->getCurrentItem()==nullptr && !isFullScreen())
    {
        GlobalObjects::appSetting->beginGroup("MainWindow");
        GlobalObjects::appSetting->setValue("Geometry",originalGeo);
        GlobalObjects::appSetting->setValue("SplitterState",playSplitter->saveState());
        GlobalObjects::appSetting->setValue("ListVisibility",!listWindow->isHidden());
        GlobalObjects::appSetting->endGroup();
    }
    GlobalObjects::playlist->setCurrentPlayTime();
    QWidget::closeEvent(event);
    playerWindow->close();
	playerWindow->deleteLater();
    library->beforeClose();
    download->beforeClose();
    GlobalObjects::clear();
    QCoreApplication::instance()->exit();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if(this->isMaximized())
            maxButton->setText(QChar(0xe93d));
        else
            maxButton->setText(QChar(0xe93c));
        if(this->isMinimized() && hideToTrayType == HideToTrayType::MINIMIZE)
        {
            this->hide();
            trayIcon->show();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int key=event->key();
    switch (key)
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Space:
    case Qt::Key_Escape:
        if(buttonPage_Play->isChecked())
        {
            QApplication::sendEvent(playerWindow,event);
            playerWindow->activateWindow();
        }
        break;
    case Qt::Key_F5:
        if(buttonPage_Play->isChecked())
        {
            Notifier *notifier = Notifier::getNotifier();
            if(listWindow->isHidden())
            {

                notifier->showMessage(Notifier::PLAYER_NOTIFY, tr("Updating..."));
            }
            int c = listWindow->updateCurrentPool();
            if(listWindow->isHidden())
            {
                notifier->showMessage(Notifier::PLAYER_NOTIFY, tr("Add %1 Danmu").arg(c));
            }
        }
        break;
    default:
        break;
    }
    CFramelessWindow::keyPressEvent(event);
}

void MainWindow::moveEvent(QMoveEvent *)
{
    if(GlobalObjects::mpvplayer->getCurrentFile().isEmpty() && !isFullScreen() && !isMini)
        originalGeo=geometry();
    if(isMini)
        miniGeo=geometry();
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    if(GlobalObjects::mpvplayer->getCurrentFile().isEmpty() && !isFullScreen() && !isMini)
        originalGeo=geometry();
    if(isMini)
        miniGeo=geometry();
#ifdef Q_OS_WIN
    QFontMetrics ffm(buttonIcon->fontMetrics());
    widgetTitlebar->setFixedHeight(ffm.height() * 1.8);
    //widgetTitlebar->setFixedHeight(36*logicalDpiY()/96);
#endif
    QToolButton **infoBtnPtrs[] = {
        &buttonPage_Play, &buttonPage_Library, &buttonPage_Downlaod
    };
    QFontMetrics fm(buttonPage_Play->fontMetrics());
    int btnHeight = fm.height() + 10*logicalDpiY()/96;
    int btnWidth = 0;
    for(int i = 0; i < sizeof(infoBtnPtrs)/sizeof(QToolButton **); ++i)
    {
        QToolButton *tb = *infoBtnPtrs[i];
        btnWidth = qMax(btnWidth, fm.horizontalAdvance(tb->text()));
    }
    btnWidth += 70*logicalDpiX()/96;
    for(int i = 0; i < sizeof(infoBtnPtrs)/sizeof(QToolButton **); ++i)
    {
        QToolButton *tb = *infoBtnPtrs[i];
        tb->setFixedSize(QSize(btnWidth, btnHeight));
    }
    downloadToolProgress->setFixedSize(btnWidth - 4*logicalDpiX()/96, 1*logicalDpiY()/96);

    const int cBtnH = widgetTitlebar->height() - 4*logicalDpiY()/96;
    const QSize controlButtonSize(cBtnH, cBtnH);
    appButton->setMinimumSize(controlButtonSize);
    maxButton->setMinimumSize(controlButtonSize);
    minButton->setMinimumSize(controlButtonSize);
    closeButton->setMinimumSize(controlButtonSize);
    appBar->setMaximumWidth(100*logicalDpiX()/96);
}

void MainWindow::showEvent(QShowEvent *)
{
#ifdef Q_OS_WIN
    static bool taskProgressInit = false;
    if(!taskProgressInit)
    {
        winTaskbarButton = new QWinTaskbarButton(this);
        QWindow *wh = windowHandle();
        winTaskbarButton->setWindow(wh);
        winTaskbarProgress = winTaskbarButton->progress();
        taskProgressInit = true;
    }
#endif
}

#ifndef Q_OS_WIN
bool MainWindow::eventFilter(QObject *object, QEvent *event) {
    static bool resize = false;
    static bool edge_top = false;
    static bool edge_bottom = false;
    static bool edge_left = false;
    static bool edge_right = false;

    static int left = -1;
    static int right = -1;
    static int top = -1;
    static int bottom = -1;

    if (object && event && isMini) {
        if (event->type() == QEvent::MouseMove ||
            event->type() == QEvent::HoverMove ||
            event->type() == QEvent::Leave ||
            event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseButtonRelease)
        {
            switch (event->type())
            {
            default:
                break;
            case QEvent::MouseMove:
                qInfo() << "Move";
                do {
                    QMouseEvent *e = static_cast<QMouseEvent*>(event);
                    qInfo()<< "Move" << e->pos()<< this->size() << frameGeometry();
                    setCursor(Qt::SizeVerCursor);
                    if (resize) {

                        if (edge_left) {
                            left = e->globalPos().x();
                        }
                        if (edge_right) {
                            right = e->globalPos().x();
                        }
                        if (edge_top) {
                            top = e->globalPos().y();
                        }
                        if (edge_bottom) {
                            bottom = e->globalPos().y();
                        }
                        QRect newRect(QPoint(left, top), QPoint(right, bottom));
                        qInfo()<< "Move to" << newRect;

                        this->setGeometry(QRect(QPoint(left, top), QPoint(right, bottom)));
                    }
                } while (0);
                break;
            case QEvent::HoverMove:
                do {
                    QHoverEvent *e = static_cast<QHoverEvent*>(event);
                    qInfo()<< "Hover" << e->pos()<< this->size() << frameGeometry();

                    bool hover_edge_top = false;
                    bool hover_edge_bottom = false;
                    bool hover_edge_left = false;
                    bool hover_edge_right = false;

                    if (e->pos().x() < this->size().width() / 10) {
                        hover_edge_left = true;
                    }
                    if (e->pos().x() > this->size().width() - this->size().width() / 10) {
                        hover_edge_right = true;
                    }
                    if (e->pos().y() < this->size().height() / 10) {
                        hover_edge_top = true;
                    }
                    if (e->pos().y() > this->size().height() - this->size().height() / 10) {
                        hover_edge_bottom = true;
                    }

                    if ((hover_edge_right && hover_edge_top) || (hover_edge_left && hover_edge_bottom)) {
                        setCursor(Qt::SizeBDiagCursor);
                    } else if ((hover_edge_right && hover_edge_bottom) || (hover_edge_left && hover_edge_top)) {
                        setCursor(Qt::SizeFDiagCursor);
                    } else if (hover_edge_left || hover_edge_right) {
                        setCursor(Qt::SizeHorCursor);
                    } else if (hover_edge_top || hover_edge_bottom) {
                        setCursor(Qt::SizeVerCursor);
                    } else {
                        setCursor(Qt::ArrowCursor);
                    }

                } while (0);
                break;
            case QEvent::Leave:
                qInfo() << "Leave";
                setCursor(Qt::ArrowCursor);
                break;
            case QEvent::MouseButtonPress:
                do {
                    resize = true;

                    left = this->frameGeometry().left();
                    right = this->frameGeometry().right();
                    top = this->frameGeometry().top();
                    bottom = this->frameGeometry().bottom();

                    QMouseEvent *e = static_cast<QMouseEvent*>(event);
                    qInfo()<< "Press"<<e->pos()<< this->size();
                    if (e->pos().x() < this->size().width() / 10) {
                        edge_left = true;
                        edge_right = false;
                        qInfo() << "Left edge";
                    } else if (e->pos().x() > this->size().width() - this->size().width() / 10) {
                        edge_right = true;
                        edge_left = false;
                        qInfo() << "Right edge";
                    }

                    if (e->pos().y() < this->size().height() / 10) {
                        edge_top = true;
                        edge_bottom = false;
                        qInfo() << "Top edge";
                    } else if (e->pos().y() > this->size().height() - this->size().height() / 10) {
                        edge_bottom = true;
                        edge_top = false;
                        qInfo() << "Bottom edge";
                    }

                } while (0);
                break;
            case QEvent::MouseButtonRelease:
                do {
                    resize = false;
                    edge_left = false;
                    edge_right = false;
                    edge_top = false;
                    edge_bottom = false;

                    QMouseEvent *e = static_cast<QMouseEvent*>(event);
                    qInfo()<< "Release"<<e->pos()<< this->size();
                } while (0);
                break;
            }
        }
    }
    return false;
}

#endif /* Q_OS_WIN */

void DropableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void DropableWidget::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls(event->mimeData()->urls());
    if(urls.size()>0)
    {
        QUrl u = urls.first();
        if(u.isLocalFile()) emit fileDrop(u.toLocalFile());
    }
}

void DropableWidget::paintEvent(QPaintEvent *)
{
    QStyleOption o;
    o.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &p, this);
}

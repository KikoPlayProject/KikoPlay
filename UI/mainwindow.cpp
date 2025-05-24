#include "mainwindow.h"
#include "globalobjects.h"
#include <QFontDatabase>
#include <QSplitter>
#include <QMenu>
#include <QLabel>
#include <QApplication>
#include <QButtonGroup>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QSystemTrayIcon>
#include <QProgressBar>
#ifdef Q_OS_WIN
#include "widgets/component/taskbarbtn/qwintaskbarbutton.h"
#include "widgets/component/taskbarbtn/qwintaskbarprogress.h"
#endif
#include "player.h"
#include "list.h"
#include "librarywindow.h"
#include "downloadwindow.h"
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
#include "widgets/windowtip.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Play/playcontext.h"
#include "Play/Danmu/danmupool.h"
#include "Common/kupdater.h"
#include "Common/keyactionmodel.h"
#include "widgets/lazycontainer.h"
#include "appmenu.h"
#include "appbar.h"
#include "ela/ElaToolButton.h"
#include "ela/ElaMenu.h"

#define SETTING_KEY_MAIN_BACKGROUND "MainWindow/Background"


Q_TAKEOVER_NATIVEEVENT_CPP(MainWindow, elaAppBar);
MainWindow::MainWindow(QWidget *parent)
    : BackgroundMainWindow(parent),hasBackground(false),hasCoverBg(false),curPage(0),listWindowWidth(0),
      isMini(false),hideToTrayType(HideToTrayType::NONE)
{
    setObjectName(QStringLiteral("MainWindow"));
    Notifier::getNotifier()->addNotify(Notifier::MAIN_DIALOG_NOTIFY, this);
    Notifier::getNotifier()->addNotify(Notifier::GLOBAL_NOTIFY, this);

    initUI();
    initTray();
    initSignals();
    restoreSize();

    if (GlobalObjects::appSetting->value("MainWindow/ShowTip",true).toBool())
    {
        GlobalObjects::appSetting->setValue("MainWindow/ShowTip", false);
        QTimer::singleShot(0, [this](){
            Tip tip(buttonIcon);
            QRect geo(0,0,400,400);
            geo.moveCenter(this->geometry().center());
            tip.move(geo.topLeft());
            tip.exec();
            QDesktopServices::openUrl(QUrl("https://kikoplayproject.github.io/about"));
        });
    }
}

void MainWindow::setHideToTrayType(HideToTrayType type)
{
    hideToTrayType = type;
}

void MainWindow::initUI()
{
    windowTip = new WindowTip(this);

    elaAppBar = new ElaAppBar(this);
    elaAppBar->setWindowControlFlag(ElaAppBarType::TitleHint, false);
    elaAppBar->setWindowControlFlag(ElaAppBarType::AppButtonHint);
    elaAppBar->setIcon(QIcon(":/res/images/kikoplay.svg"));

    QWidget *pageBtnContainer = new QWidget(this);
    QFont normalFont(GlobalObjects::normalFont, 12);
    QStringList pageButtonTexts = {
        tr("Player"),
        tr("Library"),
        tr("Resource")
    };
    QToolButton **infoBtnPtrs[] = {
        &buttonPage_Play, &buttonPage_Library, &buttonPage_Downlaod
    };
    QButtonGroup *btnGroup = new QButtonGroup(this);
    for (int i = 0; i < pageButtonTexts.size(); ++i)
    {
        *infoBtnPtrs[i] = new QToolButton(pageBtnContainer);
        QToolButton *tb = *infoBtnPtrs[i];
        tb->setFont(normalFont);
        tb->setText(pageButtonTexts[i]);
        tb->setCheckable(true);
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setObjectName(QStringLiteral("PageButton"));
        btnGroup->addButton(tb, i);
    }
    QFontMetrics fm(buttonPage_Play->fontMetrics());
    int btnWidth = 0;
    for (int i = 0; i < pageButtonTexts.size(); ++i)
    {
        btnWidth = qMax(btnWidth, fm.horizontalAdvance((*infoBtnPtrs[i])->text()));
    }
    btnWidth += 50;
    for (int i = 0; i < pageButtonTexts.size(); ++i)
    {
        (*infoBtnPtrs[i])->setFixedWidth(btnWidth);
    }
    buttonPage_Play->setChecked(true);
    QObject::connect(btnGroup, &QButtonGroup::idToggled, this, [this](int id, bool checked){
        if (checked)
        {
            GlobalObjects::context()->curMainPage = id;
            contentStackLayout->setCurrentIndex(id);
            if (id == 1 && hasCoverBg)
            {
                if(!coverPixmap.isNull()) BackgroundMainWindow::setBackground(coverPixmap);
                BackgroundMainWindow::setBlurAnimation(20., 60.);
            }
            else if (curPage == 1 && hasCoverBg)
            {
                BackgroundMainWindow::setBackground(bgImg);
                BackgroundMainWindow::setBlurAnimation(60., 0.);
            }
            curPage = id;
            if (downloadToolProgress->value() < 100)
            {
                downloadToolProgress->setVisible(curPage != 2);
            }
        }
    });
    downloadToolProgress = new QProgressBar(pageBtnContainer);
    downloadToolProgress->setObjectName(QStringLiteral("DownloadToolProgress"));
    downloadToolProgress->setRange(0, 100);
    downloadToolProgress->setTextVisible(false);
    downloadToolProgress->setFixedSize(btnWidth - 4, 1);
    QVBoxLayout *toolProgressVLayout = new QVBoxLayout();
    toolProgressVLayout->addStretch(1);
    toolProgressVLayout->addWidget(downloadToolProgress, 0, Qt::AlignHCenter);
    toolProgressVLayout->setContentsMargins(0, 0, 0, 0);
    buttonPage_Downlaod->setLayout(toolProgressVLayout);

    QHBoxLayout *titleControlHLayout = new QHBoxLayout(pageBtnContainer);
    titleControlHLayout->setContentsMargins(10, 0, 0, 0);
    titleControlHLayout->addWidget(buttonPage_Play, 0, Qt::AlignVCenter);
    titleControlHLayout->addWidget(buttonPage_Library, 0, Qt::AlignVCenter);
    titleControlHLayout->addWidget(buttonPage_Downlaod, 0, Qt::AlignVCenter);

    appBar = new AppBar(pageBtnContainer);
    elaAppBar->appButton()->setProperty("hideMenuIndicator", true);
    elaAppBar->appButton()->setMenu(new AppMenu(elaAppBar->appButton(), this));

    elaAppBar->setCustomWidget(ElaAppBarType::LeftArea, pageBtnContainer);
    elaAppBar->setCustomWidget(ElaAppBarType::RightArea, appBar);
    elaAppBar->setIsDefaultClosed(false);
    QObject::connect(elaAppBar, &ElaAppBar::closeButtonClicked, this, [=](){
        if (hideToTrayType == HideToTrayType::CLOSE)
        {
            this->hide();
            trayIcon->show();
        }
        else
        {
            this->onClose();
        }
    });

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *verticalLayout = new QVBoxLayout(centralWidget);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setSpacing(0);

    contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupPlayPage());
    contentStackLayout->addWidget(new LazyContainer(this, contentStackLayout, [this](){return this->setupLibraryPage();}));
    contentStackLayout->addWidget(new LazyContainer(this, contentStackLayout, [this](){return this->setupDownloadPage();}));
    listWindow->show();

    verticalLayout->addLayout(contentStackLayout);
    setCentralWidget(centralWidget);

    initIconAction();


    setBackground(GlobalObjects::appSetting->value(SETTING_KEY_MAIN_BACKGROUND, "").toString(), false, true);
    StyleManager::getStyleManager()->initStyle(hasBackground);

}

void MainWindow::initIconAction()
{
    ElaMenu* iconMenu = new ElaMenu(this);
    ElaToolButton *iconBtn = elaAppBar->iconButton();
    iconBtn->setMenu(iconMenu);

    QAction *actPoolManager = iconMenu->addAction(tr("Danmu Pool Manager"));
    QObject::connect(actPoolManager, &QAction::triggered, this, [this](){
        PoolManager poolManage(this);
        poolManage.exec();
    });

    QAction *actSettings = iconMenu->addAction(tr("Settings"));
    QObject::connect(actSettings, &QAction::triggered, this, [this](){
        Settings settings(Settings::PAGE_GENERAL, this);
        settings.exec();
    });

    QAction *actScriptPlayground = iconMenu->addAction(tr("Script Playground"));
    QObject::connect(actScriptPlayground, &QAction::triggered, this, [this](){
        if(!scriptPlayground)
        {
            scriptPlayground = new ScriptPlayground(this);
        }
        scriptPlayground->show();
    });

    QAction *actShowLogCenter = iconMenu->addAction(tr("Log Center"));
    QObject::connect(actShowLogCenter, &QAction::triggered, this, [this](){
        if (!logWindow)
        {
            logWindow = new LogWindow(this);
            QObject::connect(logWindow, &LogWindow::destroyed, this, [this](){
                logWindow = nullptr;
            });
        }
        logWindow->show();
    });

    QAction *actCheckUpdate = iconMenu->addAction(tr("Check For Updates"));
    QObject::connect(actCheckUpdate, &QAction::triggered, this, [this](){
        CheckUpdate checkUpdate(this);
        checkUpdate.exec();
    });
    if (KUpdater::instance()->needCheck())
    {
        actCheckUpdate->setEnabled(false);
        actCheckUpdate->setText(tr("Checking..."));
        QObject::connect(KUpdater::instance(), &KUpdater::checkDone, this, [=](){
            auto updater = KUpdater::instance();
            if (updater->hasNewVersion())
            {
                actCheckUpdate->setText(tr("Check For Updates[New Version: %1]").arg(updater->version()));
            }
            else
            {
                actCheckUpdate->setText(tr("Check For Updates"));
            }
            actCheckUpdate->setEnabled(true);
        });
        KUpdater::instance()->check();
    }

    QAction *actUseTip = iconMenu->addAction(tr("Usage Tip"));
    QObject::connect(actUseTip, &QAction::triggered, this, [this](){
        Tip tip(this);
        tip.resize(520, 400);
        tip.exec();
    });


    QAction *actFeedback = iconMenu->addAction(tr("Feedback"));
    QObject::connect(actFeedback, &QAction::triggered, [](){
        QDesktopServices::openUrl(QUrl("https://support.qq.com/product/655998"));
    });


    QAction *actAbout = iconMenu->addAction(tr("About"));
    QObject::connect(actAbout,&QAction::triggered, this, [this](){
        About about(this);
        QRect geo(0,0,400,400);
        geo.moveCenter(this->geometry().center());
        about.move(geo.topLeft());
        about.exec();
    });

    QAction *actExit = iconMenu->addAction(tr("Exit"));
    QObject::connect(actExit, &QAction::triggered, this, [=](){ this->onClose(true); });

}

void MainWindow::initTray()
{
    hideToTrayType = static_cast<HideToTrayType>(GlobalObjects::appSetting->value("MainWindow/hideToTrayType", 0).toInt());
    trayIcon = new QSystemTrayIcon(windowIcon(), this);
    trayIcon->setToolTip("KikoPlay");
    QObject::connect(trayIcon, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason){
        if (QSystemTrayIcon::Trigger == reason)
        {
            show();
            if (isMinimized()) showNormal();
            raise();
            trayIcon->hide();
        }
    });
    QMenu *trayMenu = new ElaMenu(this);
    QAction *actTrayExit = new QAction(tr("Exit"), trayMenu);
    QObject::connect(actTrayExit, &QAction::triggered, this, &MainWindow::close);
    trayMenu->addAction(actTrayExit);
    trayIcon->setContextMenu(trayMenu);
}

void MainWindow::initSignals()
{
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::stateChanged, this, [=](MPVPlayer::PlayState state){
#ifdef Q_OS_WIN
        if (winTaskbarProgress && state == MPVPlayer::Play && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            winTaskbarProgress->show();
            winTaskbarProgress->resume();
        }
        else if (winTaskbarProgress && state == MPVPlayer::Pause && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            winTaskbarProgress->show();
            winTaskbarProgress->pause();
        }
        else if (winTaskbarProgress && state == MPVPlayer::Stop)
        {
            winTaskbarProgress->hide();
        }
#endif
        if (state == MPVPlayer::Stop)
        {
            auto geo = originalGeo;
            if (isMaximized())
            {
                showNormal();
            }
            listWindow->setVisible(listShowState);
            playerWindow->toggleListCollapseState(listShowState? listWindow->currentList() : -1);
            setGeometry(geo);
        }
#ifdef Q_OS_WIN
        elaAppBar->setScreenSave(!(state == MPVPlayer::Play && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty()));
#endif
    });
#ifdef Q_OS_WIN
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::positionChanged, this, [this](int val) {
        if (winTaskbarProgress && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            winTaskbarProgress->setValue(val);
        }
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::durationChanged, this, [this](int duration){
        if (winTaskbarProgress)
        {
            winTaskbarProgress->setRange(0, duration);
        }
    });
#endif
}

void MainWindow::restoreSize()
{
    QRect defaultGeo(0, 0, 800, 480), defaultMiniGeo(0, 0, 200, 200);
    //defaultGeo.moveCenter(QApplication::desktop()->geometry().center());
    //defaultMiniGeo.moveCenter(QApplication::desktop()->geometry().center());
    defaultGeo.moveCenter(QGuiApplication::primaryScreen()->geometry().center());
    defaultMiniGeo.moveCenter(QGuiApplication::primaryScreen()->geometry().center());
    setGeometry(GlobalObjects::appSetting->value("MainWindow/Geometry",defaultGeo).toRect());
    setMinimumSize(120, 100);
    miniGeo = GlobalObjects::appSetting->value("MainWindow/miniGeometry", defaultMiniGeo).toRect();
    originalGeo = geometry();
    listWindowWidth = GlobalObjects::appSetting->value("MainWindow/ListWindowWidth", 0).toInt();
    if (listWindowWidth == 0)
    {
        listWindowWidth = 200;
    }
    QVariant splitterState(GlobalObjects::appSetting->value("MainWindow/SplitterState"));
    if (!splitterState.isNull())
    {
        playSplitter->restoreState(splitterState.toByteArray());
    }
    if (!GlobalObjects::appSetting->value("MainWindow/ListVisibility", true).toBool())
    {
        listWindow->hide();
    }
    listShowState = !listWindow->isHidden();
}

void MainWindow::onClose(bool forceExit)
{
    if (hideToTrayType == HideToTrayType::CLOSE && !forceExit)
    {
        this->hide();
        trayIcon->show();
        return;
    }
    GlobalObjects::appSetting->setValue("MainWindow/miniGeometry", miniGeo);
    GlobalObjects::appSetting->setValue("MainWindow/hideToTrayType", static_cast<int>(hideToTrayType));
    GlobalObjects::appSetting->setValue("MainWindow/ListWindowWidth", listWindowWidth);
    if (GlobalObjects::playlist->getCurrentItem()==nullptr && !isFullScreen())
    {
        GlobalObjects::appSetting->beginGroup("MainWindow");
        GlobalObjects::appSetting->setValue("Geometry",originalGeo);
        GlobalObjects::appSetting->setValue("SplitterState",playSplitter->saveState());
        GlobalObjects::appSetting->setValue("ListVisibility",!listWindow->isHidden());
        GlobalObjects::appSetting->endGroup();
    }
    GlobalObjects::playlist->setCurrentPlayTime();
    playerWindow->close();
    playerWindow->deleteLater();
    if (library) library->beforeClose();
    if (download) download->beforeClose();
    GlobalObjects::clear();
    QCoreApplication::instance()->exit();
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

void MainWindow::setBackground(const QString &imagePath, bool showAnimation, bool isStartup)
{
    bool refreshQSS = false;
    if (!imagePath.isEmpty() && QFile::exists(imagePath))
    {
        GlobalObjects::appSetting->setValue(SETTING_KEY_MAIN_BACKGROUND, imagePath);
        bgImg = QImage(imagePath);
        if (!hasCoverBg || curPage != 1)
        {
            BackgroundMainWindow::setBackground(bgImg);
            if (showAnimation) BackgroundMainWindow::setBlurAnimation(60., 0., 800);
        }
        refreshQSS = !hasBackground;
        hasBackground = true;
    }
    else
    {
        GlobalObjects::appSetting->setValue(SETTING_KEY_MAIN_BACKGROUND, "");
        bgImg = QImage();
        BackgroundMainWindow::setBackground(bgImg);
        refreshQSS = hasBackground;
        hasBackground = false;
        hasCoverBg = false;
    }
    if (isStartup) return;
    StyleManager::getStyleManager()->setCondVariable("hasBackground", hasBackground, refreshQSS);
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
    QWindow *native_wnd = QWindow::fromWinId(playerWindow->winId());
    QWidget *playerWindowWidget = QWidget::createWindowContainer(native_wnd);
    playerWindowWidget->setContentsMargins(1,0,1,1);
    playerWindowWidget->setMouseTracking(true);
    playerWindow->show();
//#else
//    QWidget *playerWindowWidget = playerWindow;
//    playerWindowWidget->setParent(playSplitter);
//    playerWindow->show();
//#endif

    listWindow = new ListWindow(playSplitter);
    QObject::connect(playerWindow, &PlayerWindow::toggleListVisibility, this, [=](int listType){
        if (listWindow->isHidden())
        {
            if (!isMaximized() && !isFullScreen())
            {
                resize(listWindowWidth+width(),height());
            }
            listWindow->setCurrentList(listType);
            listWindow->show();
        }
        else
        {
            if (listWindow->currentList() == listType)
            {
                listWindowWidth = listWindow->width();
                listWindow->hide();
                if (!isMaximized() && !isFullScreen())
                {
                    resize(width()-listWindowWidth,height());
                }
            }
            else
            {
                listWindow->setCurrentList(listType);
            }
        }
        if (GlobalObjects::playlist->getCurrentItem() == nullptr)
        {
            listShowState = !listWindow->isHidden();
        }
    });
    QObject::connect(playerWindow, &PlayerWindow::enterFullScreen, this, [=](bool on){
        static bool isMax, isShowPlaylist;
        if (on)
        {
            isShowPlaylist=!listWindow->isHidden();
            isMax=isMaximized();
            elaAppBar->hideAppBar(true, true);
            showFullScreen();
            listWindow->hide();
            playerWindow->toggleListCollapseState(-1);
            playVerticalLayout->setContentsMargins(0,0,0,0);
            playerWindow->activateWindow();
        }
        else
        {
            elaAppBar->hideAppBar(false);
            isShowPlaylist?listWindow->show():listWindow->hide();
            playerWindow->toggleListCollapseState(isShowPlaylist? listWindow->currentList() : -1);
            isMax?showMaximized():showNormal();
            playVerticalLayout->setContentsMargins(1,0,1,1);
            playerWindow->activateWindow();
        }
    });
    QObject::connect(playerWindow, &PlayerWindow::setStayOnTop, elaAppBar, &ElaAppBar::setOnTop);
    QObject::connect(playerWindow, &PlayerWindow::resizePlayer, this, [this](double w,double h,double aspectRatio){
        if (isMaximized() || isFullScreen())return;
        QRect windowGeo(geometry());
        const QRect desktopGeo = QGuiApplication::screenAt(windowGeo.center())->availableGeometry();
        double extraWidth = windowGeo.width() - playerWindow->width(), extraHeight = windowGeo.height()-playerWindow->height();
        double nw=extraWidth+w,nh=extraHeight+h;
        if (nw > desktopGeo.width())
        {
            nw = desktopGeo.width();
            w = nw - extraWidth;
            h = w / aspectRatio;
            nh = h + extraHeight;
        }
        if (nw < minimumWidth())
        {
            nw = minimumWidth();
            w = nw - extraWidth;
            h = w / aspectRatio;
            nh = h + extraHeight;
        }
        if (nh > desktopGeo.height())
        {
            nh = desktopGeo.height();
            h = nh - extraHeight;
            w = h * aspectRatio;
            nw = w + extraWidth;
        }
        if (nh < minimumHeight())
        {
            nh = minimumHeight();
            h = nh - extraHeight;
            w = h * aspectRatio;
            nw = w + extraWidth;
        }
        windowGeo.setHeight(nh);
        windowGeo.setWidth(nw);
        if (desktopGeo.contains(windowGeo))
        {
            resize(nw,nh);
        }
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
            elaAppBar->hideAppBar(true);
            isShowPlaylist=!listWindow->isHidden();
            isMax=isMaximized();
            listWindow->hide();
            playerWindow->toggleListCollapseState(-1);
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
            elaAppBar->hideAppBar(false);
            isShowPlaylist?listWindow->show():listWindow->hide();
            playerWindow->toggleListCollapseState(isShowPlaylist? listWindow->currentList() : -1);
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
    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::triggerUpdate, this, [this](){
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

    playSplitter->addWidget(playerWindowWidget);
    playSplitter->addWidget(listWindow);
    playSplitter->setStretchFactor(0,1);
    playSplitter->setStretchFactor(1,0);
    playSplitter->setCollapsible(0, false);
    playSplitter->setCollapsible(1, false);
    listWindow->hide();


    return page_play;

}

QWidget *MainWindow::setupLibraryPage()
{
    library = new LibraryWindow(this);
    library->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QObject::connect(library,&LibraryWindow::playFile,this,&MainWindow::switchToPlay);
    QObject::connect(library,&LibraryWindow::switchBackground,this,[this](const QPixmap &pixmap, bool setPixmap){
        if (!hasBackground) return;
        if (!setPixmap)
        {
            hasCoverBg = false;
            BackgroundMainWindow::setBackground(bgImg);
            BackgroundMainWindow::setBlurAnimation(60., 0.);
        }
        else
        {
            hasCoverBg = true;
            coverPixmap = pixmap.scaled(pixmap.size() * 1.2).copy(pixmap.width() * 0.1, pixmap.height() * 0.1, pixmap.width(), pixmap.height());
            if(!pixmap.isNull()) BackgroundMainWindow::setBackground(coverPixmap);
            BackgroundMainWindow::setBlurAnimation(10., 60.);
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

void MainWindow::showMessage(const QString &content, int, const QVariant &extra)
{
    windowTip->setTop(elaAppBar->isVisible()? elaAppBar->height() + 4 : 4);
    if (extra.isValid() && extra.userType() == QMetaType::type("TipParams"))
    {
        windowTip->addTip(extra.value<TipParams>());
    }
    else
    {
        TipParams param;
        param.message = content;
        windowTip->addTip(param);
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
    if (library) library->beforeClose();
    if (download) download->beforeClose();
    GlobalObjects::clear();
    QCoreApplication::instance()->exit();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (this->isMinimized() && hideToTrayType == HideToTrayType::MINIMIZE)
        {
            this->hide();
            trayIcon->show();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const QString pressKeyStr = QKeySequence(event->modifiers()|event->key()).toString();
    KeyActionModel::instance()->runAction(pressKeyStr, KeyActionItem::KEY_PRESS, this);
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    const QString pressKeyStr = QKeySequence(event->modifiers()|event->key()).toString();
    KeyActionModel::instance()->runAction(pressKeyStr, KeyActionItem::KEY_RELEASE, this);
}

void MainWindow::moveEvent(QMoveEvent *)
{
    if (GlobalObjects::mpvplayer->getCurrentFile().isEmpty() && !isFullScreen() && !isMini)
    {
        originalGeo = geometry();
    }
    if (isMini)
    {
        miniGeo = geometry();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (GlobalObjects::mpvplayer->getCurrentFile().isEmpty() && !isFullScreen() && !isMini)
    {
        originalGeo = geometry();
    }
    if (isMini)
    {
        miniGeo = geometry();
    }
    BackgroundMainWindow::resizeEvent(event);
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

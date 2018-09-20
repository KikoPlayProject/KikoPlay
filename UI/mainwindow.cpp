#include "mainwindow.h"
#include "globalobjects.h"
#include <QFontDatabase>
#include <QSplitter>
#include <QDebug>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QButtonGroup>
#include "about.h"
#include "poolmanager.h"
#include "checkupdate.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/danmurender.h"
MainWindow::MainWindow(QWidget *parent)
    : CFramelessWindow(parent),listWindowWidth(0)
{
    setupUI();
    setWindowIcon(QIcon(":/res/kikoplay.ico"));
    QRect defaultGeo(0,0,800,600);
    defaultGeo.moveCenter(QApplication::desktop()->geometry().center());
    setGeometry(GlobalObjects::appSetting->value("MainWindow/Geometry",defaultGeo).toRect());
    originalGeo=geometry();
    QVariant splitterState(GlobalObjects::appSetting->value("MainWindow/SplitterState"));
    if(!splitterState.isNull())
        playSplitter->restoreState(splitterState.toByteArray());
    if(GlobalObjects::appSetting->value("MainWindow/ListVisibility",true).toBool())
        listWindow->show();
    else
        listWindow->hide();
    listShowState=!listWindow->isHidden();
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::stateChanged,[this](MPVPlayer::PlayState state){
        if(state==MPVPlayer::Stop)
        {
			if (isFullScreen())
			{
				widgetTitlebar->show();
				showNormal();
			}
			if (listShowState)listWindow->show();
			else listWindow->hide();
            setGeometry(originalGeo);
        }
    });
}

MainWindow::~MainWindow()
{
	delete playerWindow;
}

void MainWindow::setupUI()
{

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *verticalLayout = new QVBoxLayout(centralWidget);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setSpacing(0);
//------title bar
    widgetTitlebar = new QWidget(centralWidget);
    widgetTitlebar->setObjectName(QStringLiteral("widgetTitlebar"));
    widgetTitlebar->setFixedHeight(40*logicalDpiY()/96);

    QFont normalFont("Microsoft YaHei",12);
    buttonIcon = new QToolButton(widgetTitlebar);
    buttonIcon->setFont(normalFont);
    buttonIcon->setText(" KikoPlay");
    buttonIcon->setObjectName(QStringLiteral("LogoButton"));
    buttonIcon->setIcon(QIcon(":/res/images/kikoplay-3.png"));
    buttonIcon->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    buttonIcon->setPopupMode(QToolButton::InstantPopup);

    QAction *act_poolManager=new QAction(tr("Danmu Pool Manager"), this);
    QObject::connect(act_poolManager,&QAction::triggered,[this](){
        static bool refreshPool=true;
        PoolManager poolManage(refreshPool,buttonIcon);
        poolManage.exec();
        if(refreshPool)refreshPool=false;
    });
    buttonIcon->addAction(act_poolManager);

    QAction *act_checkUpdate=new QAction(tr("Check For Updates"), this);
    QObject::connect(act_checkUpdate,&QAction::triggered,[this](){
        CheckUpdate checkUpdate(buttonIcon);
        checkUpdate.exec();
    });
    buttonIcon->addAction(act_checkUpdate);

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
    QObject::connect(act_exit,&QAction::triggered,[this](){
        close();
    });
    buttonIcon->addAction(act_exit);
    QSize pageButtonSize(100*logicalDpiX()/96,32*logicalDpiY()/96);

    buttonPage_Play = new QToolButton(widgetTitlebar);
    buttonPage_Play->setFont(normalFont);
    buttonPage_Play->setText(tr("Player"));
    buttonPage_Play->setObjectName(QStringLiteral("PageButton"));
    buttonPage_Play->setCheckable(true);
    buttonPage_Play->setToolButtonStyle(Qt::ToolButtonTextOnly);
    buttonPage_Play->setFixedSize(pageButtonSize);
    buttonPage_Play->setChecked(true);

    buttonPage_Library = new QToolButton(widgetTitlebar);
    buttonPage_Library->setFont(normalFont);
    buttonPage_Library->setText(tr("Library"));
    buttonPage_Library->setObjectName(QStringLiteral("PageButton"));
    buttonPage_Library->setCheckable(true);
    buttonPage_Library->setToolButtonStyle(Qt::ToolButtonTextOnly);
    buttonPage_Library->setFixedSize(pageButtonSize);

    buttonPage_Downlaod = new QToolButton(widgetTitlebar);
    buttonPage_Downlaod->setFont(normalFont);
    buttonPage_Downlaod->setText(tr("Download"));
    buttonPage_Downlaod->setObjectName(QStringLiteral("PageButton"));
    buttonPage_Downlaod->setCheckable(true);
    buttonPage_Downlaod->setToolButtonStyle(Qt::ToolButtonTextOnly);
    buttonPage_Downlaod->setFixedSize(pageButtonSize);

    QButtonGroup *btnGroup=new QButtonGroup(this);
    btnGroup->addButton(buttonPage_Play,0);
    btnGroup->addButton(buttonPage_Library,1);
    btnGroup->addButton(buttonPage_Downlaod,2);
    QObject::connect(btnGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[this](int id, bool checked){
        if(checked)
        {
            contentStackLayout->setCurrentIndex(id);
        }
    });

    QHBoxLayout *layout = new QHBoxLayout(widgetTitlebar);
    layout->setSpacing(0);
    layout->setContentsMargins(8*logicalDpiY()/96,0,8*logicalDpiY()/96,0);
    layout->addWidget(buttonIcon);
    layout->addSpacing(20*logicalDpiY()/96);
    QVBoxLayout *pageVerticalLayout = new QVBoxLayout();
    pageVerticalLayout->setContentsMargins(0,0,0,0);
    //pageVerticalLayout->addSpacing(10*logicalDpiY()/96);
    pageVerticalLayout->addSpacerItem(new QSpacerItem(1,10,QSizePolicy::Minimum,QSizePolicy::MinimumExpanding));
    QHBoxLayout *pageHLayout = new QHBoxLayout();
    pageHLayout->setContentsMargins(0,0,0,0);
    pageHLayout->addWidget(buttonPage_Play);
    pageHLayout->addWidget(buttonPage_Library);
    pageHLayout->addWidget(buttonPage_Downlaod);
    pageVerticalLayout->addLayout(pageHLayout);
    layout->addLayout(pageVerticalLayout);
    QSpacerItem *horizontalSpacer1 = new QSpacerItem(100, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer1);

    GlobalObjects::iconfont.setPointSize(10);
    int cbHeight=24*logicalDpiY()/96;
    minButton=new QToolButton(widgetTitlebar);
    minButton->setFont(GlobalObjects::iconfont);
    minButton->setText(QChar(0xe651));
    minButton->setObjectName(QStringLiteral("ControlButton"));
    minButton->setMinimumHeight(cbHeight);
    QObject::connect(minButton,&QToolButton::clicked,this,&MainWindow::showMinimized);

    maxButton=new QToolButton(widgetTitlebar);
    maxButton->setFont(GlobalObjects::iconfont);
    maxButton->setText(QChar(0xe93c));
    maxButton->setObjectName(QStringLiteral("ControlButton"));
    maxButton->setMinimumHeight(cbHeight);
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
    closeButton->setFont(GlobalObjects::iconfont);
    closeButton->setText(QChar(0xe60b));
    closeButton->setObjectName(QStringLiteral("closelButton"));
    closeButton->setMinimumHeight(cbHeight);
    QObject::connect(closeButton,&QToolButton::clicked,[this](){
       this->close();
    });

    layout->addWidget(minButton);
    layout->addWidget(maxButton);
    layout->addWidget(closeButton);

    verticalLayout->addWidget(widgetTitlebar);

    contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupPlayPage());
    contentStackLayout->addWidget(setupLibraryPage());
    contentStackLayout->addWidget(setupDownloadPage());

    verticalLayout->addLayout(contentStackLayout);

    setCentralWidget(centralWidget);
    setTitleBar(widgetTitlebar);
	setFocusPolicy(Qt::StrongFocus);

}

void MainWindow::switchToPlay(const QString &fileToPlay)
{
    int playTime=GlobalObjects::mpvplayer->getTime(),duration=GlobalObjects::mpvplayer->getDuration();
    if(playTime>10 && playTime<duration-10)
        GlobalObjects::playlist->setCurrentPlayTime(playTime);
    GlobalObjects::playlist->addItems(QStringList()<<fileToPlay,QModelIndex());
    const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(fileToPlay);
    if (curItem)
    {
        GlobalObjects::danmuPool->reset();
        GlobalObjects::danmuRender->cleanup();
        GlobalObjects::mpvplayer->setMedia(curItem->path);
        buttonPage_Play->click();
    }
}

QWidget *MainWindow::setupPlayPage()
{
    QWidget *page_play=new QWidget(centralWidget());
    page_play->setObjectName(QStringLiteral("widgetPagePlay"));
    page_play->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

    playSplitter = new QSplitter(page_play);
    QVBoxLayout *playVerticalLayout=new QVBoxLayout(page_play);
    playVerticalLayout->setContentsMargins(1,0,1,1);
    playVerticalLayout->addWidget(playSplitter);
    playSplitter->setHandleWidth(1);
    //QObject::connect(playSplitter,&QSplitter::splitterMoved,)

    playerWindow=new PlayerWindow();
    playerWindow->setMouseTracking(true);
    QWindow* native_wnd  = QWindow::fromWinId(playerWindow->winId());
    QWidget * playerWindowWidget=QWidget::createWindowContainer(native_wnd);
    playerWindowWidget->setContentsMargins(1,0,1,1);
    playerWindowWidget->setMouseTracking(true);
    playerWindowWidget->setParent(playSplitter);
    playerWindow->show();

    listWindow=new ListWindow(playSplitter);
    QObject::connect(playerWindow,&PlayerWindow::toggleListVisibility,[this](){
        if(listWindow->isHidden())
        {
            if(!isMaximized() && !isFullScreen())
            {
                if(listWindowWidth==0)
                    listWindowWidth=listWindow->width();
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
    QObject::connect(playerWindow,&PlayerWindow::showFullScreen,[this](bool on){
        if(on)
        {
            widgetTitlebar->hide();
            showFullScreen();
        }
        else
        {
            widgetTitlebar->show();
            showNormal();
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

    playSplitter->addWidget(playerWindowWidget);
    playSplitter->addWidget(listWindow);
    playSplitter->setStretchFactor(0,1);
    playSplitter->setStretchFactor(1,0);
    playSplitter->setCollapsible(0,false);
    playSplitter->setCollapsible(1,false);
    playerWindowWidget->setMinimumSize(600*logicalDpiX()/96,350*logicalDpiY()/96);
    listWindow->setMinimumSize(200*logicalDpiX()/96,350*logicalDpiY()/96);

    return page_play;

}

QWidget *MainWindow::setupLibraryPage()
{
    library=new LibraryWindow(this);
    QObject::connect(library,&LibraryWindow::playFile,this,&MainWindow::switchToPlay);
    return library;
}

QWidget *MainWindow::setupDownloadPage()
{
    download=new DownloadWindow(this);
    QObject::connect(download,&DownloadWindow::playFile,this,&MainWindow::switchToPlay);
    return download;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    GlobalObjects::appSetting->beginGroup("MainWindow");
    GlobalObjects::appSetting->setValue("Geometry",geometry());
    GlobalObjects::appSetting->setValue("SplitterState",playSplitter->saveState());
    GlobalObjects::appSetting->setValue("ListVisibility",!listWindow->isHidden());
    GlobalObjects::appSetting->endGroup();
    QWidget::closeEvent(event);
    playerWindow->close();
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
        //if(!QApplication::focusWidget())
            QApplication::sendEvent(playerWindow,event);
        break;
    default:
        break;
    }
    CFramelessWindow::keyPressEvent(event);
}

void MainWindow::moveEvent(QMoveEvent *)
{
    if(GlobalObjects::playlist->getCurrentItem()==nullptr)
        originalGeo=geometry();
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    if(GlobalObjects::playlist->getCurrentItem()==nullptr)
        originalGeo=geometry();
}

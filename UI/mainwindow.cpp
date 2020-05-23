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
#include "serversettting.h"
#include "checkupdate.h"
#include "tip.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/Render/danmurender.h"
namespace
{
    class BackgroundWidget : public QWidget
    {
    public:
        using QWidget::QWidget;
        void setImage(const QString &path)
        {
            if(path.isEmpty())
            {
                QImage t;
                img.swap(t);
                return;
            }
            if(img.load(path))
            {
                QPainter p(&img);
                p.fillRect(img.rect(), QColor(0,0,0,90));
                p.end();
                bgCache = QPixmap::fromImage(img.scaled(width(),height(),Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                update();
            }
        }

        // QWidget interface
    protected:
        virtual void paintEvent(QPaintEvent *e)
        {
            if(!img.isNull())
            {
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.drawPixmap(0, 0, bgCache);
                QWidget::paintEvent(e);
            }
        }
        virtual void resizeEvent(QResizeEvent *)
        {
            bgCache = QPixmap::fromImage(img.scaled(width(),height(),Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    private:
        QImage img;
        QPixmap bgCache;
    };
}
MainWindow::MainWindow(QWidget *parent)
    : CFramelessWindow(parent),listWindowWidth(0),hasBackground(false)
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
    if(GlobalObjects::appSetting->value("MainWindow/ShowTip",true).toBool())
    {
        GlobalObjects::appSetting->setValue("MainWindow/ShowTip",false);
        QTimer::singleShot(0,[this](){
            Tip tip(buttonIcon);
            tip.exec();
        });
    }
}

MainWindow::~MainWindow()
{
    delete playerWindow;
}

void MainWindow::setupUI()
{

    QWidget *centralWidget = new BackgroundWidget(this);

    QVBoxLayout *verticalLayout = new QVBoxLayout(centralWidget);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->setSpacing(0);
//------title bar
    widgetTitlebar = new DropableWidget(centralWidget);
    widgetTitlebar->setAcceptDrops(true);
    widgetTitlebar->setObjectName(QStringLiteral("widgetTitlebar"));
#ifdef Q_OS_WIN
    widgetTitlebar->setFixedHeight(40*logicalDpiY()/96);
#endif

    QFont normalFont("Microsoft YaHei",12);
    buttonIcon = new QToolButton(widgetTitlebar);
    buttonIcon->setFont(normalFont);
#ifdef Q_OS_WIN
    buttonIcon->setText(" KikoPlay ");
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

    QAction *act_lanServer=new QAction(tr("LAN Server"), this);
    QObject::connect(act_lanServer,&QAction::triggered,[this](){
        ServerSettting serverSetting(buttonIcon);
        serverSetting.exec();
    });
    buttonIcon->addAction(act_lanServer);

    QAction *act_checkUpdate=new QAction(tr("Check For Updates"), this);
    QObject::connect(act_checkUpdate,&QAction::triggered,[this](){
        CheckUpdate checkUpdate(buttonIcon);
        checkUpdate.exec();
    });
    buttonIcon->addAction(act_checkUpdate);

    QAction *act_useTip=new QAction(tr("Useage Tip"), this);
    QObject::connect(act_useTip,&QAction::triggered,[this](){
        Tip tip(buttonIcon);
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
    QObject::connect(act_exit,&QAction::triggered,[this](){
        close();
    });
    buttonIcon->addAction(act_exit);
    QSize pageButtonSize(100*logicalDpiX()/96,32*logicalDpiY()/96);
#ifdef Q_OS_WIN
    #define pageBtnObjName "PageButton"
#else
    #define pageBtnObjName "PageButton_O"
#endif
    buttonPage_Play = new QToolButton(widgetTitlebar);
    buttonPage_Play->setFont(normalFont);
    buttonPage_Play->setText(tr("Player"));
    buttonPage_Play->setObjectName(QStringLiteral(pageBtnObjName));
    buttonPage_Play->setCheckable(true);
    buttonPage_Play->setToolButtonStyle(Qt::ToolButtonTextOnly);
    buttonPage_Play->setFixedSize(pageButtonSize);
    buttonPage_Play->setChecked(true);

    buttonPage_Library = new QToolButton(widgetTitlebar);
    buttonPage_Library->setFont(normalFont);
    buttonPage_Library->setText(tr("Library"));
    buttonPage_Library->setObjectName(QStringLiteral(pageBtnObjName));
    buttonPage_Library->setCheckable(true);
    buttonPage_Library->setToolButtonStyle(Qt::ToolButtonTextOnly);
    buttonPage_Library->setFixedSize(pageButtonSize);

    buttonPage_Downlaod = new QToolButton(widgetTitlebar);
    buttonPage_Downlaod->setFont(normalFont);
    buttonPage_Downlaod->setText(tr("Download"));
    buttonPage_Downlaod->setObjectName(QStringLiteral(pageBtnObjName));
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


    QVBoxLayout *pageVerticalLayout = new QVBoxLayout();
    pageVerticalLayout->setContentsMargins(0,0,0,0);
#ifdef Q_OS_WIN
    pageVerticalLayout->addSpacerItem(new QSpacerItem(1,10,QSizePolicy::Minimum,QSizePolicy::MinimumExpanding));
#endif
    QHBoxLayout *pageHLayout = new QHBoxLayout();
    pageHLayout->setContentsMargins(0,0,0,0);
    pageHLayout->addWidget(buttonPage_Play);
    pageHLayout->addWidget(buttonPage_Library);
    pageHLayout->addWidget(buttonPage_Downlaod);
    pageVerticalLayout->addLayout(pageHLayout);


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
    QHBoxLayout *layout = new QHBoxLayout(widgetTitlebar);
    layout->setSpacing(0);
#ifdef Q_OS_WIN
    layout->setContentsMargins(8*logicalDpiY()/96,0,8*logicalDpiY()/96,0);
    layout->addWidget(buttonIcon);
    layout->addSpacing(20*logicalDpiY()/96);
    layout->addLayout(pageVerticalLayout);
    layout->addStretch(1);
    layout->addWidget(minButton);
    layout->addWidget(maxButton);
    layout->addWidget(closeButton);
#else
    layout->setContentsMargins(0,0,4*logicalDpiX()/96,0);
    layout->addLayout(pageVerticalLayout);
    layout->addStretch(1);
    layout->addWidget(buttonIcon);
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

    verticalLayout->addLayout(contentStackLayout);

    setCentralWidget(centralWidget);
    setTitleBar(widgetTitlebar);
	setFocusPolicy(Qt::StrongFocus);

    QObject::connect(widgetTitlebar, &DropableWidget::fileDrop, [this](const QString &url){
       QFileInfo fi(url);
       const QStringList imageFormats{"jpg", "png"};
       if(imageFormats.contains(fi.suffix().toLower())){
           setBackground(url);
       }
       else {
           setBackground("");
       }
    });
    setBackground(GlobalObjects::appSetting->value("MainWindow/Background", "").toString(), true);

}

void MainWindow::switchToPlay(const QString &fileToPlay)
{
    int playTime=GlobalObjects::mpvplayer->getTime();
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

void MainWindow::setBackground(const QString &imagePath, bool forceRefreshQSS)
{
    BackgroundWidget *centralWidget = static_cast<BackgroundWidget*>(this->centralWidget());
    bool refreshQSS = false;
    QString qssFile;
    if(!imagePath.isEmpty() && QFile::exists(imagePath))
    {
        centralWidget->setImage(imagePath);
        refreshQSS = !hasBackground;
        qssFile = ":/res/style_bg.qss";
        hasBackground = true;
        GlobalObjects::appSetting->setValue("MainWindow/Background", imagePath);
    }
    else
    {
        centralWidget->setImage("");
        refreshQSS = hasBackground;
        hasBackground = false;
        qssFile = ":/res/style.qss";
        GlobalObjects::appSetting->setValue("MainWindow/Background", "");
    }
    if(refreshQSS || forceRefreshQSS)
    {
        QFile qss(qssFile);
        if(qss.open(QFile::ReadOnly))
        {
            QString qssContent = QLatin1String(qss.readAll());
            qApp->setStyleSheet(qssContent);
        }
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
    QWindow *native_wnd  = QWindow::fromWinId(playerWindow->winId());
    QWidget *playerWindowWidget=QWidget::createWindowContainer(native_wnd);
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
        }
        else
        {
            widgetTitlebar->show();
            isShowPlaylist?listWindow->show():listWindow->hide();
            playerWindow->toggleListCollapseState(isShowPlaylist);
            isMax?showMaximized():showNormal();
            playVerticalLayout->setContentsMargins(1,0,1,1);
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
    if(GlobalObjects::playlist->getCurrentItem()==nullptr && !isFullScreen())
    {
        GlobalObjects::appSetting->beginGroup("MainWindow");
        GlobalObjects::appSetting->setValue("Geometry",geometry());
        GlobalObjects::appSetting->setValue("SplitterState",playSplitter->saveState());
        GlobalObjects::appSetting->setValue("ListVisibility",!listWindow->isHidden());
        GlobalObjects::appSetting->endGroup();
    }
    int playTime=GlobalObjects::mpvplayer->getTime();
    GlobalObjects::playlist->setCurrentPlayTime(playTime);
    QWidget::closeEvent(event);
    playerWindow->close();
    library->beforeClose();
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
    case Qt::Key_Escape:
        if(buttonPage_Play->isChecked())
            QApplication::sendEvent(playerWindow,event);
        break;
    default:
        break;
    }
    CFramelessWindow::keyPressEvent(event);
}

void MainWindow::moveEvent(QMoveEvent *)
{
    if(GlobalObjects::playlist->getCurrentItem()==nullptr && !isFullScreen())
        originalGeo=geometry();
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    if(GlobalObjects::playlist->getCurrentItem()==nullptr && !isFullScreen())
        originalGeo=geometry();
}

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

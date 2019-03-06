#include "downloadwindow.h"
#include <QVBoxLayout>
#include <QStackedLayout>
#include <QGridLayout>
#include <QToolButton>
#include <QPushButton>
#include <QTreeView>
#include <QLineEdit>
#include <QLabel>
#include <QButtonGroup>
#include <QProcess>
#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>
#include <QPlainTextEdit>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QSplitter>
#include <QDebug>
#include "Download/aria2jsonrpc.h"
#include "Download/downloadmodel.h"
#include "Download/downloaditemdelegate.h"
#include "Download/torrent.h"
#include "Play/Playlist/playlist.h"
#include "Play/Video/mpvplayer.h"
#include "adduritask.h"
#include "downloadsetting.h"
#include "selecttorrentfile.h"
#include "globalobjects.h"
#include "bgmlistwindow.h"
#include "ressearchwindow.h"
namespace
{
    struct FontIconToolButtonOptions
    {
        QChar iconChar;
        int fontSize;
        int iconSize;
        int leftMargin;
        int normalColor;
        int hoverColor;
        int iconTextSpace;
    };
    class FontIconToolButton : public QToolButton
    {
    public:
        explicit FontIconToolButton(const FontIconToolButtonOptions &options, QWidget *parent=nullptr):QToolButton(parent)
        {
            iconLabel=new QLabel(this);
            textLabel=new QLabel(this);
            GlobalObjects::iconfont.setPointSize(options.iconSize);
            iconLabel->setFont(GlobalObjects::iconfont);
            iconLabel->setText(options.iconChar);
            iconLabel->setMaximumWidth(iconLabel->height()+options.iconTextSpace);
            textLabel->setFont(QFont("Microsoft Yahei",options.fontSize));
            textLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            QHBoxLayout *btnHLayout=new QHBoxLayout(this);
            btnHLayout->setSpacing(0);
            btnHLayout->addSpacing(options.leftMargin*logicalDpiX()/96);
            btnHLayout->addWidget(iconLabel);
            btnHLayout->addSpacing(10*logicalDpiX()/96);
            btnHLayout->addWidget(textLabel);
            btnHLayout->addStretch(1);
            normalStyleSheet=QString("*{color:#%1;}").arg(options.normalColor,0,16);
            hoverStyleSheet=QString("*{color:#%1;}").arg(options.hoverColor,0,16);
            setNormalState();
            QObject::connect(this,&FontIconToolButton::toggled,[this](bool toggled){
               if(toggled)setHoverState();
               else setNormalState();
            });
        }
        void setCheckable(bool checkable)
        {
            if(checkable)
            {
                QObject::disconnect(this,&FontIconToolButton::pressed,this,&FontIconToolButton::setHoverState);
                QObject::disconnect(this,&FontIconToolButton::released,this,&FontIconToolButton::setNormalState);
            }
            QToolButton::setCheckable(checkable);
        }
        void setText(const QString &text)
        {
            textLabel->setText(text);
        }
    protected:
        QLabel *iconLabel,*textLabel;
        QString normalStyleSheet,hoverStyleSheet;
        virtual void setHoverState()
        {
            iconLabel->setStyleSheet(hoverStyleSheet);
            textLabel->setStyleSheet(hoverStyleSheet);
        }
        virtual void setNormalState()
        {
            iconLabel->setStyleSheet(normalStyleSheet);
            textLabel->setStyleSheet(normalStyleSheet);
        }

        virtual void enterEvent(QEvent *)
        {
            setHoverState();
        }
        virtual void leaveEvent(QEvent *)
        {
            if(!isChecked())
                setNormalState();
        }
        virtual void mousePressEvent(QMouseEvent *e)
        {
            setHoverState();
			QToolButton::mousePressEvent(e);
        }

        // QWidget interface
    public:
        virtual QSize sizeHint() const
        {
            return layout()->sizeHint();
        }
    };
}
DownloadWindow::DownloadWindow(QWidget *parent) : QWidget(parent),currentTask(nullptr)
{
    initActions();

    FontIconToolButtonOptions btnOptions;
    btnOptions.iconSize=12;
    btnOptions.fontSize=10;
    btnOptions.leftMargin=3*logicalDpiX()/96;
    btnOptions.iconTextSpace=2*logicalDpiX()/96;
    btnOptions.normalColor=0x606060;
    btnOptions.hoverColor=0x4599f7;

    btnOptions.iconChar=QChar(0xe604);
    FontIconToolButton *addUriTask=new FontIconToolButton(btnOptions, this);
    addUriTask->setObjectName(QStringLiteral("DownloadToolButton"));
    addUriTask->setText(tr("Add URI"));
    QObject::connect(addUriTask,&FontIconToolButton::clicked,[this](){
        AddUriTask addUriTaskDialog(this);
        if(QDialog::Accepted==addUriTaskDialog.exec())
        {
            for(QString &uri:addUriTaskDialog.uriList)
            {
                QString errInfo(GlobalObjects::downloadModel->addUriTask(uri,addUriTaskDialog.dir));
                if(!errInfo.isEmpty())
                    QMessageBox::information(this,tr("Error"),tr("An error occurred while adding : URI:\n %1 \n %2").arg(uri).arg(errInfo));
            }
        }
    });

    btnOptions.iconChar=QChar(0xe605);
    FontIconToolButton *addTorrentTask=new FontIconToolButton(btnOptions,this);
    addTorrentTask->setObjectName(QStringLiteral("DownloadToolButton"));
    addTorrentTask->setText(tr("Add Torrent"));
    QObject::connect(addTorrentTask,&FontIconToolButton::clicked,[this](){
        QString file = QFileDialog::getOpenFileName(this,tr("Select Torrent File"),"","Torrent(*.torrent) ");
        if(!file.isEmpty())
        {
            try
            {
                TorrentDecoder decoder(file);
                SelectTorrentFile selectTorrentFile(decoder.root,this);
                QCoreApplication::processEvents();
                if(QDialog::Accepted==selectTorrentFile.exec())
                {
                    QString errInfo(GlobalObjects::downloadModel->addTorrentTask(decoder.rawContent,decoder.infoHash,
                                                                 selectTorrentFile.dir,selectTorrentFile.selectIndexes,QString()));
                    if(!errInfo.isEmpty())
                        QMessageBox::information(this,tr("Error"),tr("An error occurred while adding Torrent : \n %1 ").arg(errInfo));
                }
                delete decoder.root;
            }
            catch(TorrentError &err)
            {
                QMessageBox::information(this,tr("Error"),err.errorInfo);
            }
        }
    });

    btnOptions.iconChar=QChar(0xe615);
    FontIconToolButton *settings=new FontIconToolButton(btnOptions,this);
    settings->setObjectName(QStringLiteral("DownloadToolButton"));
    settings->setText(tr("Settings"));
    QObject::connect(settings,&FontIconToolButton::clicked,[this](){
        DownloadSetting settingDialog(this);
        QRect geo(0,0,400,400);
        geo.moveCenter(this->geometry().center());
        settingDialog.move(geo.topLeft());
        if(QDialog::Accepted==settingDialog.exec())
        {
            QJsonObject globalOptions,taskOptions;
            if(settingDialog.downSpeedChange)
            {
                globalOptions.insert("max-overall-download-limit",QString::number(GlobalObjects::appSetting->value("Download/MaxDownloadLimit",0).toInt())+"K");
            }
            if(settingDialog.upSpeedChange)
            {
                globalOptions.insert("max-overall-upload-limit",QString::number(GlobalObjects::appSetting->value("Download/MaxUploadLimit",0).toInt())+"K");
            }
            if(settingDialog.concurrentChange)
            {
                globalOptions.insert("max-concurrent-downloads",QString::number(GlobalObjects::appSetting->value("Download/ConcurrentDownloads",5).toInt()));
            }
            if(settingDialog.seedTimeChange)
            {
                taskOptions.insert("seed-time",QString::number(GlobalObjects::appSetting->value("Download/SeedTime",5).toInt()));
            }
            if(settingDialog.btTrackerChange)
            {
                taskOptions.insert("bt-tracker",GlobalObjects::appSetting->value("Download/Trackers",QStringList()).toStringList().join(','));
            }
            if(globalOptions.count()>0)
            {
                rpc->changeGlobalOption(globalOptions);
            }
            if(taskOptions.count()>0)
            {
                auto &items=GlobalObjects::downloadModel->getItems();
                for(auto iter=items.cbegin();iter!=items.cend();++iter)
                {
                    if(iter.value()->status!=DownloadTask::Complete)
                        rpc->changeOption(iter.key(),taskOptions);
                }
            }
        }
    });

    QLineEdit *searchEdit=new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search Task"));
    searchEdit->setMinimumWidth(180*logicalDpiX()/96);
    searchEdit->setClearButtonEnabled(true);

    QHBoxLayout *toolBarHLayout=new QHBoxLayout();
    toolBarHLayout->setContentsMargins(0,10*logicalDpiY()/96,0,0);
    toolBarHLayout->addWidget(addUriTask);
    toolBarHLayout->addWidget(addTorrentTask);
    toolBarHLayout->addWidget(settings);
    toolBarHLayout->addStretch(1);
    toolBarHLayout->addWidget(searchEdit);

    downloadView=new QTreeView(this);
    downloadView->setObjectName(QStringLiteral("DownloadView"));
    downloadView->setRootIsDecorated(false);
    downloadView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    downloadView->setAlternatingRowColors(true);

    rpc=new Aria2JsonRPC(this);
    GlobalObjects::downloadModel->setRPC(rpc);
    QTimer::singleShot(1000,[this](){
        QJsonObject globalOptions;
        globalOptions.insert("max-overall-download-limit",QString::number(GlobalObjects::appSetting->value("Download/MaxDownloadLimit",0).toInt())+"K");
        globalOptions.insert("max-overall-upload-limit",QString::number(GlobalObjects::appSetting->value("Download/MaxUploadLimit",0).toInt())+"K");
        globalOptions.insert("max-concurrent-downloads",QString::number(GlobalObjects::appSetting->value("Download/ConcurrentDownloads",5).toInt()));
        rpc->changeGlobalOption(globalOptions);
    });
	//downloadView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    downloadView->setItemDelegate(new DownloadItemDelegate(this));
    downloadView->setContextMenuPolicy(Qt::ActionsContextMenu);
    TaskFilterProxyModel *proxyModel=new TaskFilterProxyModel(this);
    proxyModel->setSourceModel(GlobalObjects::downloadModel);
    downloadView->setModel(proxyModel);
    downloadView->setSortingEnabled(true);
    proxyModel->setTaskStatus(1);
    proxyModel->setFilterKeyColumn(1);

    downloadView->addAction(act_addToPlayList);
    QAction *act_separator0=new QAction(this);
    act_separator0->setSeparator(true);
    downloadView->addAction(act_separator0);
    downloadView->addAction(act_Pause);
    downloadView->addAction(act_Start);
    downloadView->addAction(act_Remove);
    QAction *act_separator1=new QAction(this);
    act_separator1->setSeparator(true);
    downloadView->addAction(act_separator1);
    downloadView->addAction(act_CopyURI);
    downloadView->addAction(act_SaveTorrent);
    downloadView->addAction(act_BrowseFile);
    QAction *act_separator2=new QAction(this);
    act_separator2->setSeparator(true);
    downloadView->addAction(act_separator2);
    downloadView->addAction(act_PauseAll);
    downloadView->addAction(act_StartAll);
	downloadView->header()->resizeSection(1, 350 * logicalDpiX() / 96);
	downloadView->header()->setStretchLastSection(false);

    QObject::connect(downloadView, &QTreeView::doubleClicked,[this,proxyModel](const QModelIndex &index){
        DownloadTask *task=GlobalObjects::downloadModel->getDownloadTask(proxyModel->mapToSource(index));
        if(task->status==DownloadTask::Complete || task->status==DownloadTask::Paused || task->status==DownloadTask::Error)
            act_Start->trigger();
        else
            act_Pause->trigger();
    });
    QObject::connect(downloadView->selectionModel(), &QItemSelectionModel::selectionChanged,this,&DownloadWindow::downloadSelectionChanged);
    QObject::connect(searchEdit,&QLineEdit::textChanged,[proxyModel](const QString &keyword){
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterRegExp(keyword);
    });

    int pageBtnHeight=28*logicalDpiY()/96;
    QToolButton *generalInfoPage=new QToolButton(this);
    generalInfoPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    generalInfoPage->setText(tr("General"));
    generalInfoPage->setFixedHeight(pageBtnHeight);
    generalInfoPage->setCheckable(true);

    QToolButton *fileInfoPage=new QToolButton(this);
    fileInfoPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    fileInfoPage->setText(tr("File"));
    fileInfoPage->setFixedHeight(pageBtnHeight);
    fileInfoPage->setCheckable(true);

    QToolButton *logPage=new QToolButton(this);
    logPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    logPage->setFixedHeight(pageBtnHeight);
    logPage->setText(tr("Global Log"));
    logPage->setCheckable(true);

    QHBoxLayout *pageBarHLayout=new QHBoxLayout();
    pageBarHLayout->setContentsMargins(0,0,0,2*logicalDpiY()/96);
    pageBarHLayout->addWidget(generalInfoPage);
    pageBarHLayout->addWidget(fileInfoPage);
    pageBarHLayout->addWidget(logPage);
    pageBarHLayout->addStretch(1);

    QButtonGroup *pageButtonGroup=new QButtonGroup(this);
    pageButtonGroup->addButton(generalInfoPage,0);
    pageButtonGroup->addButton(fileInfoPage,1);
    pageButtonGroup->addButton(logPage,2);

    generalInfoPage->setChecked(true);


    QWidget *detailInfoContent=new QWidget(this);
    detailInfoContent->setContentsMargins(0,0,0,10*logicalDpiX()/96);
    QStackedLayout *detailInfoSLayout=new QStackedLayout(detailInfoContent);
    detailInfoSLayout->addWidget(setupGeneralInfoPage());
    detailInfoSLayout->addWidget(setupFileInfoPage());
    detailInfoSLayout->addWidget(setupGlobalLogPage());

    QObject::connect(pageButtonGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[detailInfoSLayout](int id, bool checked){
        if(checked)detailInfoSLayout->setCurrentIndex(id);
    });

    QWidget *bottomContent=new QWidget(this);
    QVBoxLayout *bvLayout=new QVBoxLayout(bottomContent);
    bvLayout->setContentsMargins(0,0,0,0);
    bvLayout->addLayout(pageBarHLayout);
    bvLayout->addWidget(detailInfoContent);
    QSplitter *viewBottomSplitter=new QSplitter(Qt::Vertical,this);
    viewBottomSplitter->setObjectName(QStringLiteral("NormalSplitter"));
    viewBottomSplitter->addWidget(downloadView);
    viewBottomSplitter->addWidget(bottomContent);
    viewBottomSplitter->setStretchFactor(0,4);
    viewBottomSplitter->setStretchFactor(1,1);
    viewBottomSplitter->setCollapsible(0,false);
    viewBottomSplitter->setCollapsible(1,true);

    QWidget *downloadContainer=new QWidget(this);
    QGridLayout *downContainerGLayout=new QGridLayout(downloadContainer);
    downContainerGLayout->addLayout(toolBarHLayout,0,1);
    downContainerGLayout->addWidget(viewBottomSplitter,1,1);
    downContainerGLayout->setRowStretch(1,1);
    downContainerGLayout->setContentsMargins(0,0,0,0);

    BgmListWindow *bgmListWindow=new BgmListWindow(this);
    ResSearchWindow *resSearchWindow=new ResSearchWindow(this);
    QObject::connect(bgmListWindow,&BgmListWindow::searchBgm,this,[this,resSearchWindow](const QString &item){
        taskTypeButtonGroup->button(4)->setChecked(true);
        resSearchWindow->search(item);
    });
    QObject::connect(resSearchWindow,&ResSearchWindow::addTask,this,[this](const QStringList &urls){
        AddUriTask addUriTaskDialog(this,urls);
        if(QDialog::Accepted==addUriTaskDialog.exec())
        {
            for(QString &uri:addUriTaskDialog.uriList)
            {
                QString errInfo(GlobalObjects::downloadModel->addUriTask(uri,addUriTaskDialog.dir));
                if(!errInfo.isEmpty())
                    QMessageBox::information(this,tr("Error"),tr("An error occurred while adding : URI:\n %1 \n %2").arg(uri).arg(errInfo));
            }
        }
    });
    rightPanelSLayout=new QStackedLayout();
    rightPanelSLayout->addWidget(downloadContainer);
    rightPanelSLayout->addWidget(bgmListWindow);
    rightPanelSLayout->addWidget(resSearchWindow);

    QGridLayout *contentGLayout=new QGridLayout(this);
    contentGLayout->addWidget(setupLeftPanel(),0,0);
    contentGLayout->addLayout(rightPanelSLayout,0,1);
    contentGLayout->setColumnStretch(1,1);
    contentGLayout->setRowStretch(0,1);
    contentGLayout->setContentsMargins(0,0,10*logicalDpiX()/96,0);

    refreshTimer=new QTimer();
    QObject::connect(refreshTimer,&QTimer::timeout,[this](){
        auto &items=GlobalObjects::downloadModel->getItems();
        for(auto iter=items.cbegin();iter!=items.cend();++iter)
        {
            rpc->tellStatus(iter.key());
        }
        rpc->tellGlobalStatus();
    });
    QObject::connect(rpc,&Aria2JsonRPC::refreshStatus,[this](const QJsonObject &statusObj){
        QString gid(statusObj.value("gid").toString());
        if(currentTask && currentTask->gid==gid)
        {
            selectedTFModel->updateFileProgress(statusObj.value("files").toArray());
        }
        GlobalObjects::downloadModel->updateItemStatus(statusObj);
    });
    QObject::connect(rpc,&Aria2JsonRPC::refreshGlobalStatus,[this](int downSpeed,int upSpeed,int numActive){
        downSpeedLabel->setText(formatSize(true,downSpeed));
        upSpeedLabel->setText(formatSize(true,upSpeed));
    });

    QObject::connect(rpc,&Aria2JsonRPC::showLog,logView,&QPlainTextEdit::appendPlainText);

    QObject::connect(GlobalObjects::downloadModel,&DownloadModel::magnetDone,[this](const QString &path, const QString &magnet){
        try
        {
            TorrentDecoder decoder(path);
            SelectTorrentFile selectTorrentFile(decoder.root,this);
            QCoreApplication::processEvents();
            if(QDialog::Accepted==selectTorrentFile.exec())
            {
                QString errInfo(GlobalObjects::downloadModel->addTorrentTask(decoder.rawContent,decoder.infoHash,
                                                             selectTorrentFile.dir,selectTorrentFile.selectIndexes,magnet));
                if(!errInfo.isEmpty())
                    QMessageBox::information(this,tr("Error"),tr("An error occurred while adding Torrent : \n %1 ").arg(errInfo));
            }
            delete decoder.root;
        }
        catch(TorrentError &err)
        {
            QMessageBox::information(this,tr("Error"),err.errorInfo);
        }
    });
    QObject::connect(GlobalObjects::downloadModel,&DownloadModel::removeTask,[this](const QString &gid){
        if(currentTask && currentTask->gid==gid)
        {
            setDetailInfo(nullptr);
        }
    });
    downloadSelectionChanged();

}

DownloadWindow::~DownloadWindow()
{
    act_Pause->trigger();
}

QWidget *DownloadWindow::setupLeftPanel()
{
    QWidget *leftPanel=new QWidget(this);
    leftPanel->setObjectName(QStringLiteral("DownloadLeftPanel"));
    const int panelWidth=220*logicalDpiX()/96;
    leftPanel->setFixedWidth(panelWidth);

    //const int tbHeight=45*logicalDpiY()/96;
    FontIconToolButtonOptions btnOptions;
    btnOptions.fontSize=12;
    btnOptions.iconSize=12;
    btnOptions.normalColor=0xc5c5c5;
    btnOptions.hoverColor=0x679bcc;
    btnOptions.leftMargin=20*logicalDpiX()/96;
    btnOptions.iconTextSpace=4*logicalDpiX()/96;

    btnOptions.iconChar=QChar(0xe653);
    FontIconToolButton *downloadingTask=new FontIconToolButton(btnOptions,leftPanel);
    downloadingTask->setObjectName(QStringLiteral("TaskTypeToolButton"));
    downloadingTask->setFixedWidth(panelWidth);
    downloadingTask->setCheckable(true);
    downloadingTask->setChecked(true);
    downloadingTask->setText(tr("Downloading"));

    btnOptions.iconChar=QChar(0xe69a);
    FontIconToolButton *completedTask=new FontIconToolButton(btnOptions,leftPanel);
    completedTask->setObjectName(QStringLiteral("TaskTypeToolButton"));
    completedTask->setFixedWidth(panelWidth);
    completedTask->setCheckable(true);
    completedTask->setText(tr("Completed"));

    btnOptions.iconChar=QChar(0xe603);
    FontIconToolButton *allTask=new FontIconToolButton(btnOptions,leftPanel);
    allTask->setObjectName(QStringLiteral("TaskTypeToolButton"));
    allTask->setFixedWidth(panelWidth);
    allTask->setCheckable(true);
    allTask->setText(tr("All"));

    btnOptions.iconChar=QChar(0xe63a);
    FontIconToolButton *bgmList=new FontIconToolButton(btnOptions,leftPanel);
    bgmList->setObjectName(QStringLiteral("TaskTypeToolButton"));
    bgmList->setFixedWidth(panelWidth);
    bgmList->setCheckable(true);
    bgmList->setText(tr("BgmList"));

    btnOptions.iconChar=QChar(0xe609);
    FontIconToolButton *resSearch=new FontIconToolButton(btnOptions,leftPanel);
    resSearch->setObjectName(QStringLiteral("TaskTypeToolButton"));
    resSearch->setFixedWidth(panelWidth);
    resSearch->setCheckable(true);
    resSearch->setText(tr("ResSearch"));

    taskTypeButtonGroup=new QButtonGroup(this);
    taskTypeButtonGroup->addButton(downloadingTask,1);
    taskTypeButtonGroup->addButton(completedTask,2);
    taskTypeButtonGroup->addButton(allTask,0);
    taskTypeButtonGroup->addButton(bgmList,3);
    taskTypeButtonGroup->addButton(resSearch,4);
    QObject::connect(taskTypeButtonGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[this](int id, bool checked){
        if(checked)
        {
            if(id<3)
            {
                rightPanelSLayout->setCurrentIndex(0);
                TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
                model->setTaskStatus(id);
                model->setFilterKeyColumn(1);
            }
            else if(id==3)
            {
                rightPanelSLayout->setCurrentIndex(1);
            }
            else if(id==4)
            {
                rightPanelSLayout->setCurrentIndex(2);
            }
        }
    });

    GlobalObjects::iconfont.setPointSize(12);
    QLabel *downSpeedIconLabel=new QLabel(this);
    downSpeedIconLabel->setObjectName(QStringLiteral("DownSpeedIcon"));
    downSpeedIconLabel->setFont(GlobalObjects::iconfont);
    downSpeedIconLabel->setText(QChar(0xe910));
    downSpeedIconLabel->setMaximumWidth(downSpeedIconLabel->height()+4*logicalDpiX()/96);
    QLabel *upSpeedIconLabel=new QLabel(this);
    upSpeedIconLabel->setObjectName(QStringLiteral("UpSpeedIcon"));
    upSpeedIconLabel->setFont(GlobalObjects::iconfont);
    upSpeedIconLabel->setText(QChar(0xe941));
    upSpeedIconLabel->setMaximumWidth(upSpeedIconLabel->height()+4*logicalDpiX()/96);
    downSpeedLabel=new QLabel(this);
    downSpeedLabel->setObjectName(QStringLiteral("DownSpeedLabel"));
    upSpeedLabel=new QLabel(this);
    upSpeedLabel->setObjectName(QStringLiteral("UpSpeedLabel"));

    QVBoxLayout *leftVLayout=new QVBoxLayout(leftPanel);
    leftVLayout->setContentsMargins(0,0,0,0);
    leftVLayout->setSpacing(0);
    leftVLayout->addWidget(downloadingTask);
    leftVLayout->addWidget(completedTask);
    leftVLayout->addWidget(allTask);
    leftVLayout->addWidget(bgmList);
    leftVLayout->addWidget(resSearch);
    leftVLayout->addStretch(1);
    QGridLayout *speedInfoGLayout=new QGridLayout();
    speedInfoGLayout->addWidget(downSpeedIconLabel,0,0);
    speedInfoGLayout->addWidget(downSpeedLabel,0,1);
    speedInfoGLayout->addWidget(upSpeedIconLabel,1,0);
    speedInfoGLayout->addWidget(upSpeedLabel,1,1);
    speedInfoGLayout->setContentsMargins(30*logicalDpiX()/96,0,20*logicalDpiX()/96,20*logicalDpiY()/96);
    leftVLayout->addLayout(speedInfoGLayout);
	_ASSERTE(_CrtCheckMemory());
    return leftPanel;
}

QWidget *DownloadWindow::setupGeneralInfoPage()
{
    QWidget *content=new QWidget(this);
    QGridLayout *gInfoGLayout=new QGridLayout(content);
    taskTitleLabel=new QLabel(content);
    taskTitleLabel->setFont(QFont("Microsoft Yahei",12));
    taskTitleLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    taskTimeLabel=new QLabel(content);
    taskDirLabel=new QLabel(content);
    taskDirLabel->setOpenExternalLinks(true);
    gInfoGLayout->addWidget(taskTitleLabel,0,0);
    gInfoGLayout->addWidget(taskTimeLabel,1,0);
    gInfoGLayout->addWidget(taskDirLabel,2,0);
    gInfoGLayout->setRowStretch(3,1);
    gInfoGLayout->setColumnStretch(0,1);
    return content;
}

QWidget *DownloadWindow::setupFileInfoPage()
{
    selectedTFModel=new CTorrentFileModel(this);
    fileInfoView=new QTreeView(this);
    fileInfoView->setAlternatingRowColors(true);
    fileInfoView->setModel(selectedTFModel);
    //fileInfoView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    fileInfoView->header()->resizeSection(0,300*logicalDpiX()/96);

    QObject::connect(fileInfoView,&QTreeView::doubleClicked,[this](const QModelIndex &index){
        TorrentFile *item = static_cast<TorrentFile*>(index.internalPointer());
        if(currentTask && item->index>0 && item->completedSize==item->size)
        {
            QFileInfo info(currentTask->dir,item->name);
            if(!info.exists())
            {
                QMessageBox::information(this,"KikoPlay",tr("File Not Exist"));
                return;
            }
            if(info.isDir())return;
            if(GlobalObjects::mpvplayer->videoFileFormats.contains("*."+info.suffix().toLower()))
            {
                emit playFile(info.absoluteFilePath());
            }
        }
    });
    QObject::connect(selectedTFModel,&CTorrentFileModel::checkedIndexChanged,[this](){
        if(!currentTask) return;
        QString selIndexes(selectedTFModel->getCheckedIndex());
        if(selIndexes!=currentTask->selectedIndexes)
        {
            QJsonObject options;
            options.insert("select-file", selIndexes);
            rpc->changeOption(currentTask->gid,options);
            currentTask->selectedIndexes=selIndexes;
            GlobalObjects::downloadModel->saveItemStatus(currentTask);
        }
    });
    return fileInfoView;
}

QWidget *DownloadWindow::setupGlobalLogPage()
{
    logView=new QPlainTextEdit(this);
    logView->setReadOnly(true);
    logView->setCenterOnScroll(true);
    logView->setMaximumBlockCount(50);
    //logView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    return logView;
}

void DownloadWindow::initActions()
{
    act_Pause=new QAction(tr("Pause"),this);
    QObject::connect(act_Pause,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QModelIndexList selectedRows= downloadView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        QList<DownloadTask*> taskList;
        for(const QModelIndex &proxyIndex:selectedRows)
        {
            taskList.append(GlobalObjects::downloadModel->getDownloadTask(model->mapToSource(proxyIndex)));
        }
        for(DownloadTask *task:taskList)
        {
            if(task->gid.isEmpty())
                continue;
            if(task->status==DownloadTask::Paused || task->status==DownloadTask::Complete)
                continue;
            rpc->switchPauseStatus(task->gid,true);
        }
    });
    act_Start=new QAction(tr("Start"),this);
    QObject::connect(act_Start,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QModelIndexList selectedRows= downloadView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        QList<DownloadTask*> taskList;
        for(const QModelIndex &proxyIndex:selectedRows)
        {
            taskList.append(GlobalObjects::downloadModel->getDownloadTask(model->mapToSource(proxyIndex)));
        }
        for(DownloadTask *task:taskList)
        {
            if(task->gid.isEmpty())
            {
                QFileInfo fi(task->dir,task->title+".aria2");
                if(!fi.exists())
                {
                    QMessageBox::StandardButton btn = QMessageBox::information(this,tr("Resume"),tr("Control file(*.aria2) does not exist, download the file all over again ?\n%1").arg(task->title),
                                             QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::No);
                    if(btn==QMessageBox::Cancel)return;
                    GlobalObjects::downloadModel->restartDownloadTask(task,btn==QMessageBox::Yes);
                }
				else
				{
					GlobalObjects::downloadModel->restartDownloadTask(task);
				}
            }
            else if(task->status==DownloadTask::Downloading || task->status==DownloadTask::Waiting)
            {
                continue;
            }
            rpc->switchPauseStatus(task->gid,false);
        }
    });
    act_Remove=new QAction(tr("Remove"),this);
    QObject::connect(act_Remove,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QModelIndexList selectedRows= downloadView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        QMessageBox::StandardButton btn = QMessageBox::information(this,tr("Remove"),tr("Delete the Downloaded Files?"),
                                 QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::No);
        if(btn==QMessageBox::Cancel)return;
        QModelIndexList sourceIndexes;
        for(const QModelIndex &proxyIndex:selectedRows)
        {
            QModelIndex index(model->mapToSource(proxyIndex));
            sourceIndexes.append(index);
            if(currentTask==GlobalObjects::downloadModel->getDownloadTask(index))
            {
                setDetailInfo(nullptr);
            }
        }
        GlobalObjects::downloadModel->removeItem(sourceIndexes,btn==QMessageBox::Yes);
    });
    act_PauseAll=new QAction(tr("Pause All"),this);
    QObject::connect(act_PauseAll,&QAction::triggered,[this](){
        rpc->switchAllPauseStatus(true);
    });
    act_StartAll=new QAction(tr("Start All"),this);
    QObject::connect(act_StartAll,&QAction::triggered,[this](){
        auto &taskList=GlobalObjects::downloadModel->getAllTask();
        for(DownloadTask *task:taskList)
        {
            if(task->status!=DownloadTask::Complete && task->gid.isEmpty())
            {
                QFileInfo fi(task->dir,task->title+".aria2");
                if(fi.exists())
                {
                    GlobalObjects::downloadModel->restartDownloadTask(task);
                }
            }
        }
        rpc->switchAllPauseStatus(false);
    });
    act_BrowseFile=new QAction(tr("Browse File"),this);
    QObject::connect(act_BrowseFile,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QModelIndexList selectedRows= downloadView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        DownloadTask *task=GlobalObjects::downloadModel->getDownloadTask(model->mapToSource(selectedRows.last()));
        QFileInfo info(task->dir,task->title);
        if(!info.exists())return;
		QString command("Explorer /select," + QDir::toNativeSeparators(info.absoluteFilePath()));
        QProcess::startDetached(command);
    });
    act_addToPlayList=new QAction(tr("Add To PlayList"),this);
    QObject::connect(act_addToPlayList,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QModelIndexList selectedRows= downloadView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        for(const QModelIndex &proxyIndex:selectedRows)
        {
            DownloadTask *task=GlobalObjects::downloadModel->getDownloadTask(model->mapToSource(proxyIndex));
            QFileInfo info(task->dir,task->title);
            if(!info.exists())continue;
            if(info.isDir())
                GlobalObjects::playlist->addFolder(info.absoluteFilePath(),QModelIndex());
            else
                GlobalObjects::playlist->addItems(QStringList()<<info.absoluteFilePath(),QModelIndex());
        }
    });
    QObject::connect(GlobalObjects::downloadModel,&DownloadModel::taskFinish,this,[](DownloadTask *task){
        if(GlobalObjects::appSetting->value("Download/AutoAddToList",false).toBool())
        {
            QFileInfo info(task->dir,task->title);
            if(!info.exists()) return;
            if(info.isDir())
                GlobalObjects::playlist->addFolder(info.absoluteFilePath(),QModelIndex());
            else
                GlobalObjects::playlist->addItems(QStringList()<<info.absoluteFilePath(),QModelIndex());
        }
    });

    act_CopyURI=new QAction(tr("Copy URI"),this);
    QObject::connect(act_CopyURI,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QModelIndexList selectedRows= downloadView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        DownloadTask *task=GlobalObjects::downloadModel->getDownloadTask(model->mapToSource(selectedRows.last()));
        QClipboard *cb = QApplication::clipboard();
        cb->setText(task->uri);
    });
    act_SaveTorrent=new QAction(tr("Save Torrent"),this);
    QObject::connect(act_SaveTorrent,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QModelIndexList selectedRows= downloadView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        DownloadTask *task=GlobalObjects::downloadModel->getDownloadTask(model->mapToSource(selectedRows.last()));
        if(task->torrentContentState==-1) GlobalObjects::downloadModel->tryLoadTorrentContent(task);
        if(task->torrentContent.isEmpty()) return;
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Torrent"),task->title,"Torrent File (*.torrent)");
        if(!fileName.isEmpty())
        {
            QFile torrnentFile(fileName);
            bool ret=torrnentFile.open(QIODevice::WriteOnly);
            if(!ret) return;
            torrnentFile.write(task->torrentContent);
        }
    });

}

void DownloadWindow::downloadSelectionChanged()
{
    TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
    QItemSelection selection = model->mapSelectionToSource(downloadView->selectionModel()->selection());
    bool hasSelection = !selection.isEmpty();
    act_addToPlayList->setEnabled(hasSelection);
    act_Pause->setEnabled(hasSelection);
    act_Start->setEnabled(hasSelection);
    act_Remove->setEnabled(hasSelection);
    act_BrowseFile->setEnabled(hasSelection);
    setDetailInfo(hasSelection?GlobalObjects::downloadModel->getDownloadTask(selection.indexes().last()):nullptr);
}

void DownloadWindow::setDetailInfo(DownloadTask *task)
{
    if(task)
    {
        act_CopyURI->setEnabled(!task->uri.isEmpty());
        act_SaveTorrent->setEnabled(task->torrentContentState==0);
        currentTask=task;
        taskTitleLabel->setText(task->title);
        taskTimeLabel->setText(tr("Create Time: %1 \t Finish Time: %2")
                               .arg(QDateTime::fromSecsSinceEpoch(task->createTime).toString("yyyy-MM-dd hh:mm:ss"))
                               .arg(task->finishTime<task->createTime?"----":QDateTime::fromSecsSinceEpoch(task->finishTime).toString("yyyy-MM-dd hh:mm:ss")));
        taskDirLabel->setText(QString("<a href = \"file:///%1\">%2</a>").arg(task->dir).arg(task->dir));
        taskDirLabel->setOpenExternalLinks(true);
        if(task->torrentContentState==-1) GlobalObjects::downloadModel->tryLoadTorrentContent(task);
        if(!task->torrentContent.isEmpty())
        {
            if(task->fileInfo)
            {
                selectedTFModel->setContent(task->fileInfo,task->selectedIndexes);
            }
            else
            {
                try
                {
                    TorrentDecoder decoder(task->torrentContent);
                    task->fileInfo=new TorrentFileInfo();
                    task->fileInfo->root=decoder.root;
                    task->fileInfo->setIndexMap();
                    if(task->gid.isEmpty() && task->status==DownloadTask::Complete)
                    {
                        for(auto iter=task->fileInfo->indexMap.begin();iter!=task->fileInfo->indexMap.end();++iter)
                        {
                            iter.value()->completedSize=iter.value()->size;
                        }
                    }
                    selectedTFModel->setContent(task->fileInfo, task->selectedIndexes);
                }
                catch(TorrentError)
                {
                    selectedTFModel->setContent(nullptr);
                }
            }
        }
        else
        {
            selectedTFModel->setContent(nullptr);
        }
    }
    else
    {
        currentTask=nullptr;
        taskTitleLabel->setText(tr("<No Item has been Selected>"));
        taskTimeLabel->setText(tr("Create Time: ---- \t Finish Time: ----"));
        taskDirLabel->setText(QString());
        selectedTFModel->setContent(nullptr);
        act_CopyURI->setEnabled(false);
        act_SaveTorrent->setEnabled(false);
    }
}

void DownloadWindow::showEvent(QShowEvent *)
{
    if(!refreshTimer->isActive())
        refreshTimer->start(refreshInterval);
    else
        refreshTimer->setInterval(refreshInterval);
}

void DownloadWindow::hideEvent(QHideEvent *)
{
    refreshTimer->setInterval(backgoundRefreshInterval);
}


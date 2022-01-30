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
#include <QScrollArea>
#include "Download/aria2jsonrpc.h"
#include "Download/downloadmodel.h"
#include "Download/downloaditemdelegate.h"
#include "Download/torrent.h"
#include "Download/peermodel.h"
#include "Play/Playlist/playlist.h"
#include "Play/Video/mpvplayer.h"
#include "adduritask.h"
#include "settings.h"
#include "stylemanager.h"
#include "selecttorrentfile.h"
#include "globalobjects.h"
#include "bgmlistwindow.h"
#include "ressearchwindow.h"
#include "autodownloadwindow.h"
#include "widgets/fonticonbutton.h"
#include "widgets/dialogtip.h"
#include "Common/logger.h"

DownloadWindow::DownloadWindow(QWidget *parent) : QWidget(parent),currentTask(nullptr)
{
    setObjectName(QStringLiteral("DownLoadWindow"));
    dialogTip = new DialogTip(this);
    dialogTip->raise();
    dialogTip->hide();
    Notifier::getNotifier()->addNotify(Notifier::DOWNLOAD_NOTIFY, this);

    initActions();

    contentSplitter = new QSplitter(this);
    contentSplitter->setObjectName(QStringLiteral("DownloadSpliter"));
    QWidget *containerWidget = new QWidget(contentSplitter);
    containerWidget->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
    containerWidget->setMinimumWidth(100*logicalDpiX()/96);

    QWidget *downloadContainer=new QWidget(this);

    FontIconButton *addUriTask=new FontIconButton(QChar(0xe604), tr("Add URI"), 12, 10, 2*logicalDpiX()/96, downloadContainer);
    addUriTask->setObjectName(QStringLiteral("DownloadToolButton"));
    QObject::connect(addUriTask,&FontIconButton::clicked,[this](){
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

    FontIconButton *addTorrentTask=new FontIconButton(QChar(0xe605),tr("Add Torrent"),12, 10, 2*logicalDpiX()/96,downloadContainer);
    addTorrentTask->setObjectName(QStringLiteral("DownloadToolButton"));
    QObject::connect(addTorrentTask,&FontIconButton::clicked,[this](){
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

    FontIconButton *settings=new FontIconButton(QChar(0xe615),tr("Settings"), 12, 10, 2*logicalDpiX()/96, downloadContainer);
    settings->setObjectName(QStringLiteral("DownloadToolButton"));
    QObject::connect(settings,&FontIconButton::clicked,[this](){
        Settings settings(Settings::PAGE_DOWN, this);
        const SettingPage *downPage = settings.getPage(Settings::PAGE_DOWN);
        if(QDialog::Accepted==settings.exec())
        {
            QJsonObject globalOptions,taskOptions;
            const auto &changedValues = downPage->getChangedValues();
            if(changedValues.contains("downSpeed"))
            {
                globalOptions.insert("max-overall-download-limit",QString::number(changedValues["downSpeed"].toInt())+"K");
            }
            if(changedValues.contains("upSpeed"))
            {
                globalOptions.insert("max-overall-upload-limit",QString::number(changedValues["upSpeed"].toInt())+"K");
            }
            if(changedValues.contains("concurrent"))
            {
                globalOptions.insert("max-concurrent-downloads",QString::number(changedValues["concurrent"].toInt()));
            }
            if(changedValues.contains("seedTime"))
            {
                taskOptions.insert("seed-time",QString::number(changedValues["seedTime"].toInt()));
            }
            if(changedValues.contains("btTracker"))
            {
                taskOptions.insert("bt-tracker",changedValues["btTracker"].toStringList().join(','));
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

    QLineEdit *searchEdit=new QLineEdit(downloadContainer);
    searchEdit->setPlaceholderText(tr("Search Task"));
    searchEdit->setMinimumWidth(180*logicalDpiX()/96);
    searchEdit->setClearButtonEnabled(true);

    QHBoxLayout *toolBarHLayout=new QHBoxLayout();
    toolBarHLayout->setContentsMargins(0,0,0,0);
    toolBarHLayout->addWidget(addUriTask);
    toolBarHLayout->addWidget(addTorrentTask);
    toolBarHLayout->addWidget(settings);
    toolBarHLayout->addStretch(1);
    toolBarHLayout->addWidget(searchEdit);

    downloadView=new QTreeView(downloadContainer);
    downloadView->setObjectName(QStringLiteral("DownloadView"));
    downloadView->setFont(QFont(GlobalObjects::normalFont,10));
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
    downloadView->setItemDelegate(new DownloadItemDelegate(this));
    downloadView->setContextMenuPolicy(Qt::ActionsContextMenu);
    TaskFilterProxyModel *proxyModel=new TaskFilterProxyModel(this);
    proxyModel->setSourceModel(GlobalObjects::downloadModel);
    downloadView->setModel(proxyModel);
    downloadView->setSortingEnabled(true);
    proxyModel->setTaskStatus(1);
    proxyModel->setFilterKeyColumn(static_cast<int>(DownloadModel::Columns::TITLE));

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
    downloadView->header()->resizeSection(static_cast<int>(DownloadModel::Columns::TITLE), 350 * logicalDpiX() / 96);
    //downloadView->header()->setStretchLastSection(false);

    downloadView->hideColumn(static_cast<int>(DownloadModel::Columns::UPSPEED));
    QAction *actShowUpSpeed=new QAction(tr("UpSpeed"),this);
    actShowUpSpeed->setCheckable(true);
    QObject::connect(actShowUpSpeed,&QAction::toggled,[this](bool checked){
        if(checked) downloadView->showColumn(static_cast<int>(DownloadModel::Columns::UPSPEED));
        else downloadView->hideColumn(static_cast<int>(DownloadModel::Columns::UPSPEED));
        GlobalObjects::appSetting->setValue("Download/ShowUpSpeed", checked);
    });
    actShowUpSpeed->setChecked(GlobalObjects::appSetting->value("Download/ShowUpSpeed", false).toBool());

    downloadView->hideColumn(static_cast<int>(DownloadModel::Columns::CONNECTION));
    QAction *actShowConnections=new QAction(tr("Connections"),this);
    actShowConnections->setCheckable(true);
    QObject::connect(actShowConnections,&QAction::toggled,[this](bool checked){
        if(checked) downloadView->showColumn(static_cast<int>(DownloadModel::Columns::CONNECTION));
        else downloadView->hideColumn(static_cast<int>(DownloadModel::Columns::CONNECTION));
        GlobalObjects::appSetting->setValue("Download/ShowConnections", checked);
    });
    actShowConnections->setChecked(GlobalObjects::appSetting->value("Download/ShowConnections", false).toBool());

    downloadView->hideColumn(static_cast<int>(DownloadModel::Columns::SEEDER));
    QAction *actShowSeeders=new QAction(tr("Seeders"),this);
    actShowSeeders->setCheckable(true);
    QObject::connect(actShowSeeders,&QAction::toggled,[this](bool checked){
        if(checked) downloadView->showColumn(static_cast<int>(DownloadModel::Columns::SEEDER));
        else downloadView->hideColumn(static_cast<int>(DownloadModel::Columns::SEEDER));
        GlobalObjects::appSetting->setValue("Download/ShowSeeders", checked);
    });
    actShowSeeders->setChecked(GlobalObjects::appSetting->value("Download/ShowSeeders", false).toBool());

    downloadView->hideColumn(static_cast<int>(DownloadModel::Columns::DIR));
    QAction *actShowDir=new QAction(tr("Dir"),this);
    actShowDir->setCheckable(true);
    QObject::connect(actShowDir,&QAction::toggled,[this](bool checked){
        if(checked) downloadView->showColumn(static_cast<int>(DownloadModel::Columns::DIR));
        else downloadView->hideColumn(static_cast<int>(DownloadModel::Columns::DIR));
        GlobalObjects::appSetting->setValue("Download/ShowDir", checked);
    });
    actShowDir->setChecked(GlobalObjects::appSetting->value("Download/ShowDir", false).toBool());


    downloadView->header()->setContextMenuPolicy(Qt::ActionsContextMenu);
    downloadView->header()->addAction(actShowUpSpeed);
    downloadView->header()->addAction(actShowConnections);
    downloadView->header()->addAction(actShowSeeders);
    downloadView->header()->addAction(actShowDir);

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

    int pageBtnHeight=30*logicalDpiY()/96;
    QToolButton *generalInfoPage=new QToolButton(downloadContainer);
    generalInfoPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    generalInfoPage->setText(tr("General"));
    generalInfoPage->setFixedHeight(pageBtnHeight);
    generalInfoPage->setCheckable(true);

    QToolButton *fileInfoPage=new QToolButton(downloadContainer);
    fileInfoPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    fileInfoPage->setText(tr("File"));
    fileInfoPage->setFixedHeight(pageBtnHeight);
    fileInfoPage->setCheckable(true);

    QToolButton *blockPage=new QToolButton(downloadContainer);
    blockPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    blockPage->setText(tr("Block"));
    blockPage->setFixedHeight(pageBtnHeight);
    blockPage->setCheckable(true);

    QToolButton *connectionPage=new QToolButton(downloadContainer);
    connectionPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    connectionPage->setText(tr("Connection"));
    connectionPage->setFixedHeight(pageBtnHeight);
    connectionPage->setCheckable(true);

    QToolButton *logPage=new QToolButton(downloadContainer);
    logPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    logPage->setFixedHeight(pageBtnHeight);
    logPage->setText(tr("Global Log"));
    logPage->setCheckable(true);

    QHBoxLayout *pageBarHLayout=new QHBoxLayout();
    pageBarHLayout->setContentsMargins(0,0,0,2*logicalDpiY()/96);
    pageBarHLayout->addWidget(generalInfoPage);
    pageBarHLayout->addWidget(fileInfoPage);
    pageBarHLayout->addWidget(blockPage);
    pageBarHLayout->addWidget(connectionPage);
    pageBarHLayout->addWidget(logPage);
    pageBarHLayout->addStretch(1);

    QButtonGroup *pageButtonGroup=new QButtonGroup(downloadContainer);
    pageButtonGroup->addButton(generalInfoPage,0);
    pageButtonGroup->addButton(fileInfoPage,1);
    pageButtonGroup->addButton(blockPage,2);
    pageButtonGroup->addButton(connectionPage,3);
    pageButtonGroup->addButton(logPage,4);

    generalInfoPage->setChecked(true);


    QWidget *detailInfoContent=new QWidget(downloadContainer);
    detailInfoContent->setContentsMargins(0,0,0,0);
    QStackedLayout *detailInfoSLayout=new QStackedLayout(detailInfoContent);
    detailInfoSLayout->addWidget(setupGeneralInfoPage(detailInfoContent));
    detailInfoSLayout->addWidget(setupFileInfoPage(detailInfoContent));
    detailInfoSLayout->addWidget(setupBlockPage(detailInfoContent));
    detailInfoSLayout->addWidget(setupConnectionPage(detailInfoContent));
    detailInfoSLayout->addWidget(setupGlobalLogPage(detailInfoContent));

    QObject::connect(pageButtonGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[detailInfoSLayout](int id, bool checked){
        if(checked)detailInfoSLayout->setCurrentIndex(id);
    });

    QWidget *bottomContent=new QWidget(downloadContainer);
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


    QGridLayout *downContainerGLayout=new QGridLayout(downloadContainer);
    downContainerGLayout->addLayout(toolBarHLayout,0,1);
    downContainerGLayout->addWidget(viewBottomSplitter,1,1);
    downContainerGLayout->setRowStretch(1,1);

    BgmListWindow *bgmListWindow=new BgmListWindow(containerWidget);

    ResSearchWindow *resSearchWindow=new ResSearchWindow(containerWidget);
    QObject::connect(bgmListWindow,&BgmListWindow::searchBgm,this,[this,resSearchWindow](const QString &item){
        taskTypeButtonGroup->button(4)->setChecked(true);
        resSearchWindow->search(item, true);
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
    AutoDownloadWindow *autoDownloadWindow = new AutoDownloadWindow(containerWidget);
    autoDownloadWindow->setObjectName(QStringLiteral("AutoDownloadWindow"));
    QObject::connect(autoDownloadWindow,&AutoDownloadWindow::addTask,this,[this](const QStringList &uris, bool directly, const QString &path){
        if(directly)
        {
            for(const QString &uri:uris)
            {
                QString errInfo(GlobalObjects::downloadModel->addUriTask(uri,path,true));
                if(!errInfo.isEmpty())
                    Logger::logger()->log(Logger::LogType::Aria2, QString("[AddTask]%1: %2").arg(uri, errInfo));
            }
        }
        else
        {
            AddUriTask addUriTaskDialog(this,uris,path);
            if(QDialog::Accepted==addUriTaskDialog.exec())
            {
                for(QString &uri:addUriTaskDialog.uriList)
                {
                    QString errInfo(GlobalObjects::downloadModel->addUriTask(uri,addUriTaskDialog.dir));
                    if(!errInfo.isEmpty())
                        QMessageBox::information(this,tr("Error"),tr("An error occurred while adding : URI:\n %1 \n %2").arg(uri).arg(errInfo));
                }
            }
        }
    });

    QObject::connect(StyleManager::getStyleManager(), &StyleManager::styleModelChanged, this, [=](StyleManager::StyleMode mode){
        bool setScrollStyle = (mode==StyleManager::BG_COLOR || mode==StyleManager::DEFAULT_BG);
        downloadView->setProperty("cScrollStyle", setScrollStyle);
        detailInfoContent->setProperty("cScrollStyle", setScrollStyle);
        bgmListWindow->setProperty("cScrollStyle", setScrollStyle);
    });

    rightPanelSLayout=new QStackedLayout(containerWidget);
    rightPanelSLayout->addWidget(downloadContainer);
    rightPanelSLayout->addWidget(bgmListWindow);
    rightPanelSLayout->addWidget(resSearchWindow);
    rightPanelSLayout->addWidget(autoDownloadWindow);

    contentSplitter->setHandleWidth(1);
    contentSplitter->addWidget(setupLeftPanel(contentSplitter));
    contentSplitter->addWidget(containerWidget);
    contentSplitter->setCollapsible(1, false);

    QHBoxLayout *contentHLayout=new QHBoxLayout(this);
    contentHLayout->addWidget(contentSplitter);
    contentHLayout->setContentsMargins(0,0,0,0);

    QByteArray splitterState = GlobalObjects::appSetting->value("Download/SplitterState").toByteArray();
    if(!splitterState.isNull())
    {
        contentSplitter->restoreState(splitterState);
    }

    QObject::connect(contentSplitter, &QSplitter::splitterMoved, this, [contentHLayout, this](){
        int leftSize = contentSplitter->sizes()[0];
        if(leftSize == 0) contentHLayout->setContentsMargins(5*logicalDpiX()/96,0,0,0);
        else contentHLayout->setContentsMargins(0,0,0,0);
    });

    refreshTimer=new QTimer();
    QObject::connect(refreshTimer,&QTimer::timeout,[this](){
        auto &items=GlobalObjects::downloadModel->getItems();
        for(auto iter=items.cbegin();iter!=items.cend();++iter)
        {
            rpc->tellStatus(iter.key());
        }
        rpc->tellGlobalStatus();
        if(currentTask && !this->isHidden() && !currentTask->gid.isEmpty())
        {
            rpc->getPeers(currentTask->gid);
        }
    });
    QObject::connect(rpc,&Aria2JsonRPC::refreshStatus,[this](const QJsonObject &statusObj){
        QString gid(statusObj.value("gid").toString());
        GlobalObjects::downloadModel->updateItemStatus(statusObj);
        if(currentTask && currentTask->gid==gid)
        {
            selectedTFModel->updateFileProgress(statusObj.value("files").toArray());
            blockView->setBlock(currentTask->numPieces, currentTask->bitfield);
            blockView->setToolTip(tr("Blocks: %1 Size: %2").arg(currentTask->numPieces).arg(formatSize(false, currentTask->pieceLength)));
        }
    });
    QObject::connect(rpc,&Aria2JsonRPC::refreshGlobalStatus,[this](int downSpeed,int upSpeed,int numActive){
        downSpeedLabel->setText(formatSize(true,downSpeed));
        upSpeedLabel->setText(formatSize(true,upSpeed));
    });
    QObject::connect(rpc,&Aria2JsonRPC::refreshPeerStatus,[this](const QJsonArray &peerArray){
       if(currentTask && !this->isHidden())
       {           
           peerModel->setPeers(peerArray, currentTask->numPieces);
       }
    });
    QObject::connect(GlobalObjects::downloadModel,&DownloadModel::magnetDone,[this](const QString &path, const QString &magnet, bool directlyDownload){
        try
        {
            TorrentDecoder decoder(path);
            if(directlyDownload)
            {
                TorrentFileModel model(decoder.root);
                model.checkAll(true);
                QFileInfo torrentFile(path);
                QString errInfo=GlobalObjects::downloadModel->addTorrentTask(decoder.rawContent,decoder.infoHash,
                                                                            torrentFile.absoluteDir().absolutePath(),model.getCheckedIndex(),magnet);
                if(!errInfo.isEmpty())
                    Logger::logger()->log(Logger::LogType::Aria2, QString("[AddTorrentTask]%1: %2").arg(magnet, errInfo));
            }
            else
            {
                SelectTorrentFile selectTorrentFile(decoder.root,this);
                QCoreApplication::processEvents();
                if(QDialog::Accepted==selectTorrentFile.exec())
                {
                    QString errInfo(GlobalObjects::downloadModel->addTorrentTask(decoder.rawContent,decoder.infoHash,
                                                                 selectTorrentFile.dir,selectTorrentFile.selectIndexes,magnet));
                    if(!errInfo.isEmpty())
                        QMessageBox::information(this,tr("Error"),tr("An error occurred while adding Torrent : \n %1 ").arg(errInfo));
                }
            }
            QFile::remove(path);
            delete decoder.root;
        }
        catch(TorrentError &err)
        {
            showMessage(err.errorInfo, NM_ERROR | NM_HIDE);
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

void DownloadWindow::beforeClose()
{
    act_Pause->trigger();
    GlobalObjects::appSetting->setValue("Download/SplitterState", contentSplitter->saveState());
    rpc->exit();
}

QWidget *DownloadWindow::setupLeftPanel(QWidget *parent)
{
    QWidget *leftPanel=new QWidget(parent);
    leftPanel->setObjectName(QStringLiteral("DownloadLeftPanel"));
    leftPanel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    const int panelWidth=200*logicalDpiX()/96;
    leftPanel->resize(panelWidth, leftPanel->height());

    const int iconSpace = 8*logicalDpiX()/96;
    const QSizePolicy btnSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    FontIconButton *downloadingTask=new FontIconButton(QChar(0xe653),tr("Downloading"),12,12,iconSpace,leftPanel);
    downloadingTask->setObjectName(QStringLiteral("TaskTypeToolButton"));
    downloadingTask->setSizePolicy(btnSizePolicy);
    downloadingTask->setCheckable(true);
    downloadingTask->setAutoHideText(true);
    downloadingTask->setChecked(true);

    FontIconButton *completedTask=new FontIconButton(QChar(0xe69a),tr("Completed"), 12,12,iconSpace,leftPanel);
    completedTask->setObjectName(QStringLiteral("TaskTypeToolButton"));
    completedTask->setSizePolicy(btnSizePolicy);
    completedTask->setCheckable(true);

    FontIconButton *allTask=new FontIconButton(QChar(0xe603),tr("All"), 12,12,iconSpace,leftPanel);
    allTask->setObjectName(QStringLiteral("TaskTypeToolButton"));
    allTask->setSizePolicy(btnSizePolicy);
    allTask->setCheckable(true);

    FontIconButton *bgmList=new FontIconButton(QChar(0xe63a),tr("BgmList"), 12,12,iconSpace,leftPanel);
    bgmList->setObjectName(QStringLiteral("TaskTypeToolButton"));
    bgmList->setSizePolicy(btnSizePolicy);
    bgmList->setCheckable(true);

    FontIconButton *resSearch=new FontIconButton(QChar(0xe609),tr("ResSearch"), 12,12,iconSpace,leftPanel);
    resSearch->setObjectName(QStringLiteral("TaskTypeToolButton"));
    resSearch->setSizePolicy(btnSizePolicy);
    resSearch->setCheckable(true);

    FontIconButton *autoDownload=new FontIconButton(QChar(0xe610),tr("AutoDownload"),12,12,iconSpace,leftPanel);
    autoDownload->setObjectName(QStringLiteral("TaskTypeToolButton"));
    autoDownload->setSizePolicy(btnSizePolicy);
    autoDownload->setCheckable(true);

    taskTypeButtonGroup=new QButtonGroup(parent);
    taskTypeButtonGroup->addButton(downloadingTask,1);
    taskTypeButtonGroup->addButton(completedTask,2);
    taskTypeButtonGroup->addButton(allTask,0);
    taskTypeButtonGroup->addButton(bgmList,3);
    taskTypeButtonGroup->addButton(resSearch,4);
    taskTypeButtonGroup->addButton(autoDownload,5);
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
            else
            {
                rightPanelSLayout->setCurrentIndex(id-2);
            }
        }
    });



    QObject::connect(downloadingTask, &FontIconButton::textHidden, this, [this, completedTask,
                     allTask, bgmList, resSearch,autoDownload,leftPanel](bool hide){
        if(hide)
        {
            completedTask->hideText(true);
            allTask->hideText(true);
            bgmList->hideText(true);
            resSearch->hideText(true);
            autoDownload->hideText(true);
            leftPanel->setMinimumWidth(completedTask->sizeHint().width());
        }
        else
        {
            completedTask->hideText(false);
            allTask->hideText(false);
            bgmList->hideText(false);
            resSearch->hideText(false);
            autoDownload->hideText(false);
        }
    });

    QVBoxLayout *leftVLayout=new QVBoxLayout(leftPanel);
    leftVLayout->setContentsMargins(0,0,0,0);
    leftVLayout->setSpacing(0);
    leftVLayout->addWidget(downloadingTask);
    leftVLayout->addWidget(completedTask);
    leftVLayout->addWidget(allTask);
    leftVLayout->addWidget(bgmList);
    leftVLayout->addWidget(resSearch);
    leftVLayout->addWidget(autoDownload);
    leftVLayout->addStretch(1);

    return leftPanel;
}

QWidget *DownloadWindow::setupGeneralInfoPage(QWidget *parent)
{
    QWidget *content=new QWidget(parent);
    QGridLayout *gInfoGLayout=new QGridLayout(content);
    taskTitleLabel=new QLabel(content);
    taskTitleLabel->setFont(QFont(GlobalObjects::normalFont,12));
    taskTitleLabel->setObjectName(QStringLiteral("TaskTitleLabel"));
    taskTitleLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    taskTimeLabel=new QLabel(content);
    taskTimeLabel->setObjectName(QStringLiteral("TaskTimeLabel"));

    GlobalObjects::iconfont.setPointSize(12);
    downSpeedIconLabel=new QLabel(parent);
    downSpeedIconLabel->setObjectName(QStringLiteral("DownSpeedIcon"));
    downSpeedIconLabel->setFont(GlobalObjects::iconfont);
    downSpeedIconLabel->setText(QChar(0xe910));
    downSpeedIconLabel->setMaximumWidth(downSpeedIconLabel->height()+4*logicalDpiX()/96);
    upSpeedIconLabel=new QLabel(parent);
    upSpeedIconLabel->setObjectName(QStringLiteral("UpSpeedIcon"));
    upSpeedIconLabel->setFont(GlobalObjects::iconfont);
    upSpeedIconLabel->setText(QChar(0xe941));
    upSpeedIconLabel->setMaximumWidth(upSpeedIconLabel->height()+4*logicalDpiX()/96);
    downSpeedLabel=new QLabel(parent);
    downSpeedLabel->setObjectName(QStringLiteral("DownSpeedLabel"));
    upSpeedLabel=new QLabel(parent);
    upSpeedLabel->setObjectName(QStringLiteral("UpSpeedLabel"));

    QGridLayout *speedGLayout = new QGridLayout;
    speedGLayout->addWidget(downSpeedIconLabel, 0, 0);
    speedGLayout->addWidget(downSpeedLabel, 0, 1);
    speedGLayout->addWidget(upSpeedIconLabel, 0, 2);
    speedGLayout->addWidget(upSpeedLabel, 0, 3);
    speedGLayout->setContentsMargins(0, 0, 0, 0);
    speedGLayout->setSpacing(4*logicalDpiX()/96);
    speedGLayout->setColumnStretch(1, 1);
    speedGLayout->setColumnStretch(3, 1);
    speedGLayout->setRowStretch(1, 1);

    QHBoxLayout *speedHLayout = new QHBoxLayout;
    speedHLayout->setContentsMargins(0, 0, 0, 0);
    speedHLayout->setSpacing(0);
    speedHLayout->addLayout(speedGLayout);
    speedHLayout->addStretch(1);

    gInfoGLayout->addWidget(taskTitleLabel,0,0);
    gInfoGLayout->addWidget(taskTimeLabel,1,0);
    gInfoGLayout->addLayout(speedHLayout,2,0);
    gInfoGLayout->setRowStretch(3,1);
    gInfoGLayout->setColumnStretch(0,1);
    return content;
}

QWidget *DownloadWindow::setupFileInfoPage(QWidget *parent)
{
    selectedTFModel=new CTorrentFileModel(this);
    fileInfoView=new TorrentTreeView(parent);
    fileInfoView->setAlternatingRowColors(true);
    fileInfoView->setModel(selectedTFModel);
    fileInfoView->setObjectName(QStringLiteral("TaskFileInfoView"));
    //fileInfoView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    fileInfoView->header()->resizeSection(0,300*logicalDpiX()/96);
    fileInfoView->setFont(QFont(GlobalObjects::normalFont,10));
    QObject::connect(fileInfoView, &TorrentTreeView::ignoreColorChanged, selectedTFModel, &CTorrentFileModel::setIgnoreColor);
    QObject::connect(fileInfoView, &TorrentTreeView::normColorChanged, selectedTFModel, &CTorrentFileModel::setNormColor);

    QObject::connect(fileInfoView,&QTreeView::doubleClicked,[this](const QModelIndex &index){
        TorrentFile *item = static_cast<TorrentFile*>(index.internalPointer());
        if(currentTask && item->index>0 && item->completedSize==item->size)
        {
            QString dir(currentTask->dir);
            TorrentFile *root(item),*curItem(item->parent);
            while(root->parent) root=root->parent;
            QStringList folders;
            while(curItem && curItem!=root)
            {
                folders.push_front(curItem->name);
                curItem=curItem->parent;
            }
            if(root->children.count()>1) folders.push_front(root->name);
            if(folders.count()>0) dir+="/" + folders.join('/');
            QFileInfo info(dir,item->name);
            if(!info.exists())
            {
                showMessage(tr("File Not Exist"), NM_ERROR|NM_HIDE);
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

QWidget *DownloadWindow::setupBlockPage(QWidget *parent)
{
    QScrollArea *contentScrollArea=new QScrollArea(parent);
    blockView = new BlockWidget(parent);
    blockView->setObjectName(QStringLiteral("TaskBlockView"));
    contentScrollArea->setWidget(blockView);
    contentScrollArea->setWidgetResizable(true);
    contentScrollArea->setAlignment(Qt::AlignCenter);
    return contentScrollArea;
}

QWidget *DownloadWindow::setupConnectionPage(QWidget *parent)
{
    peerModel = new PeerModel(this);
    PeerTreeView *peerView = new PeerTreeView(parent);
    peerView->setModel(peerModel);
    peerView->setRootIsDecorated(false);
    peerView->setObjectName(QStringLiteral("TaskPeerView"));
    peerView->header()->resizeSection(static_cast<int>(PeerModel::Columns::PROGRESS), 280*logicalDpiX()/96);
    peerView->header()->resizeSection(static_cast<int>(PeerModel::Columns::CLIENT), 180*logicalDpiX()/96);
     peerView->header()->resizeSection(static_cast<int>(PeerModel::Columns::IP), 160*logicalDpiX()/96);
    peerView->setFont(QFont(GlobalObjects::normalFont,10));
    PeerDelegate *peerDelegate = new PeerDelegate(this);
    QObject::connect(peerView, &PeerTreeView::barColorChanged, [=](const QColor &c){peerDelegate->barColor=c;});
    QObject::connect(peerView, &PeerTreeView::borderColorChanged, [=](const QColor &c){peerDelegate->borderColor=c;});
    QObject::connect(peerView, &PeerTreeView::backgroundColorChanged, [=](const QColor &c){peerDelegate->backgroundcolor=c;});
    peerView->setItemDelegate(peerDelegate);
    return peerView;
}

QWidget *DownloadWindow::setupGlobalLogPage(QWidget *parent)
{
    QPlainTextEdit *logView=new QPlainTextEdit(parent);
    logView->setReadOnly(true);
    logView->setCenterOnScroll(true);
    logView->setMaximumBlockCount(50);
    logView->setObjectName(QStringLiteral("TaskLogView"));
    QObject::connect(Logger::logger(),&Logger::logAppend, this, [=](Logger::LogType type, const QString &log){
        if(type == Logger::Aria2)
        {
            logView->appendPlainText(log);
            QTextCursor cursor =logView->textCursor();
            cursor.movePosition(QTextCursor::End);
            logView->setTextCursor(cursor);
        }
    });
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
        currentTask=task;
        taskTitleLabel->setText(task->title);
        taskTimeLabel->setText(tr("Create Time: %1 \t Finish Time: %2")
                               .arg(QDateTime::fromSecsSinceEpoch(task->createTime).toString("yyyy-MM-dd hh:mm:ss"))
                               .arg(task->finishTime<task->createTime?"----":QDateTime::fromSecsSinceEpoch(task->finishTime).toString("yyyy-MM-dd hh:mm:ss")));
        blockView->setBlock(task->numPieces, task->bitfield);
        blockView->setToolTip(tr("Blocks: %1 Size: %2").arg(task->numPieces).arg(formatSize(false, task->pieceLength)));
        if(task->torrentContentState==-1) GlobalObjects::downloadModel->tryLoadTorrentContent(task);
		act_SaveTorrent->setEnabled(task->torrentContentState == 1);
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
        peerModel->clear();
        taskTitleLabel->setText(tr("<No Item has been Selected>"));
        taskTimeLabel->setText(tr("Create Time: ---- \t Finish Time: ----"));
        blockView->setBlock(0, "0");
        blockView->setToolTip("");
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

void DownloadWindow::showMessage(const QString &content, int flag)
{
    dialogTip->showMessage(content, flag);
}


BlockWidget::BlockWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(blockWidth+2*marginX, blockHeight+2*marginY);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
}

void BlockWidget::setBlock(int count, const QString &state)
{
    blockCount = count;
    blockState.resize(state.size()*4);
    int i = 0;
    for(QChar c: state){
        int n = c.toLatin1();
        if(c.isDigit()) n = n-'0';
        else if('a'<=n && 'f'>=n) n = n-'a'+10;
        if(n >=0 && n < 16){
            blockState[i] = (n & 0x8)?1:0;
            blockState[i+1] = (n & 0x4)?1:0;
            blockState[i+2] = (n & 0x2)?1:0;
            blockState[i+3] = (n & 0x1)?1:0;
        }
        i += 4;
    }
    int sw = blockWidth + marginX;
    int sh = blockHeight+marginY;
    int colCount = width() / sw;
    int rowCount = blockCount / colCount + 1;
    setMinimumHeight(rowCount*sh+marginY);
    update();
}

void BlockWidget::paintEvent(QPaintEvent *event)
{
    int w = width();
    int sw = blockWidth + marginX;
    int sh = blockHeight+marginY;
    int colCount = w / sw;

    QPainter painter(this);
    QPen pen(borderColor);
    //pen.setWidth(1);
    painter.setPen(pen);
    QVector<QRect> rects, rects2;
    for(int i=0;i<blockCount;++i)
    {
        int cr = i/colCount, cc = i%colCount;
        if(blockState[i]) rects.append(QRect(sw*cc+marginX,cr*sh+marginY,blockWidth,blockHeight));
        else rects2.append(QRect(sw*cc+marginX,cr*sh+marginY,blockWidth,blockHeight));
    }
    painter.setBrush(QBrush(fillColorF));
    painter.drawRects(rects);
    painter.setBrush(QBrush(fillColorU));
    painter.drawRects(rects2);
    QWidget::paintEvent(event);
}

void BlockWidget::resizeEvent(QResizeEvent *)
{
    blockWidth = 12*logicalDpiX()/96;
    blockHeight = 12*logicalDpiY()/96;
    marginX = 2*logicalDpiX()/96;
    marginY = 2*logicalDpiY()/96;

    int sw = blockWidth + marginX;
    int sh = blockHeight+marginY;
    int colCount = width() / sw;
    int rowCount = blockCount / colCount + 1;
    int dh = rowCount*sh;
    setMinimumHeight(dh+marginY);
}


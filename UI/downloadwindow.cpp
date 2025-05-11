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
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/klineedit.h"
#include "UI/widgets/kplaintextedit.h"
#include "UI/widgets/lazycontainer.h"
#include "adduritask.h"
#include "settings.h"
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
    setAttribute(Qt::WA_StyledBackground, true);
    dialogTip = new DialogTip(this);
    dialogTip->raise();
    dialogTip->hide();
    Notifier::getNotifier()->addNotify(Notifier::DOWNLOAD_NOTIFY, this);

    initActions();

    rpc = new Aria2JsonRPC(this);
    GlobalObjects::downloadModel->setRPC(rpc);
    QTimer::singleShot(1000, [this](){
        QJsonObject globalOptions;
        globalOptions.insert("max-overall-download-limit",QString::number(GlobalObjects::appSetting->value("Download/MaxDownloadLimit",0).toInt())+"K");
        globalOptions.insert("max-overall-upload-limit",QString::number(GlobalObjects::appSetting->value("Download/MaxUploadLimit",0).toInt())+"K");
        globalOptions.insert("max-concurrent-downloads",QString::number(GlobalObjects::appSetting->value("Download/ConcurrentDownloads",5).toInt()));
        rpc->changeGlobalOption(globalOptions);
    });

    QWidget *containerWidget = new QWidget(this);
    containerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    containerWidget->setMinimumWidth(200);

    QWidget *downloadContainer = initDownloadPage();
    rightPanelSLayout=new QStackedLayout(containerWidget);
    rightPanelSLayout->addWidget(downloadContainer);
    //rightPanelSLayout->addWidget(new AutoDownloadWindow(containerWidget));

    rightPanelSLayout->addWidget(new LazyContainer(this, rightPanelSLayout, [=](){
        AutoDownloadWindow *autoDownloadWindow = new AutoDownloadWindow(containerWidget);
        //autoDownloadWindow->setObjectName(QStringLiteral("AutoDownloadWindow"));
        QObject::connect(autoDownloadWindow, &AutoDownloadWindow::addTask, this, [=](const QStringList &uris, bool directly, const QString &path){
            if (directly)
            {
                for (const QString &uri : uris)
                {
                    QString errInfo(GlobalObjects::downloadModel->addUriTask(uri,path,true));
                    if (!errInfo.isEmpty())
                    {
                        Logger::logger()->log(Logger::LogType::Aria2, QString("[AddTask]%1: %2").arg(uri, errInfo));
                    }
                }
            }
            else
            {
                addUrlTask(uris, path);
            }
        });
        return autoDownloadWindow;
    }));
    rightPanelSLayout->addWidget(new LazyContainer(this, rightPanelSLayout, [=](){
        BgmListWindow *bgmListWindow=new BgmListWindow(containerWidget);
        QObject::connect(bgmListWindow,&BgmListWindow::searchBgm,this,[this](const QString &item){
            taskTypeButtonGroup->button(5)->setChecked(true);
            rightPanelSLayout->setCurrentIndex(3);
            resSearchWindow->search(item, true);
        });
        return bgmListWindow;
    }));
    rightPanelSLayout->addWidget(new LazyContainer(this, rightPanelSLayout, [=](){
        resSearchWindow = new ResSearchWindow(containerWidget);
        QObject::connect(resSearchWindow, &ResSearchWindow::addTask, this, [=](const QStringList &urls){
            addUrlTask(urls);
        });
        return resSearchWindow;
    }));


    QHBoxLayout *contentHLayout = new QHBoxLayout(this);
    contentHLayout->addWidget(initLeftPanel(this));
    contentHLayout->addWidget(containerWidget);
    contentHLayout->setContentsMargins(0,0,8,8);

    refreshTimer = new QTimer();
    QObject::connect(refreshTimer, &QTimer::timeout, this, [=](){
        auto &items=GlobalObjects::downloadModel->getItems();
        qint64 totalLength = 0, completedLength = 0;
#ifdef QT_DEBUG
        QStringList logs;
#endif
        for(auto iter=items.cbegin();iter!=items.cend();++iter)
        {
            if(iter.value()->status == DownloadTask::Downloading)
            {
                totalLength += iter.value()->totalLength;
                completedLength += iter.value()->completedLength;
#ifdef QT_DEBUG
                logs << QString("[%1/%2]%3").arg(iter.value()->totalLength).arg(iter.value()->completedLength).arg(iter.value()->title);
#endif
            }
            rpc->tellStatus(iter.key());
        }
#ifdef QT_DEBUG
        if(logs.size()>0)
        Logger::logger()->log(Logger::APP, QString("Progress Stat[%1/%2]\n%3").arg(totalLength).arg(completedLength).arg(logs.join('\n')));
#endif
        emit totalProgressUpdate(totalLength==0 ? 100 : qBound<double>(0,(double)completedLength/totalLength*100,100));
        rpc->tellGlobalStatus();
        if(currentTask && !this->isHidden() && !currentTask->gid.isEmpty())
        {
            rpc->getPeers(currentTask->gid);
        }
    });
    QObject::connect(rpc, &Aria2JsonRPC::refreshStatus, this, [=](const QJsonObject &statusObj){
        QString gid(statusObj.value("gid").toString());
        GlobalObjects::downloadModel->updateItemStatus(statusObj);
        if (currentTask && currentTask->gid == gid)
        {
            selectedTFModel->updateFileProgress(statusObj.value("files").toArray());
            blockView->setBlock(currentTask->numPieces, currentTask->bitfield);
            blockView->setToolTip(tr("Blocks: %1 Size: %2").arg(currentTask->numPieces).arg(formatSize(false, currentTask->pieceLength)));
        }
    });
    QObject::connect(rpc, &Aria2JsonRPC::refreshGlobalStatus, this, [=](int downSpeed,int upSpeed,int numActive){
        downSpeedLabel->setText(formatSize(true,downSpeed));
        upSpeedLabel->setText(formatSize(true,upSpeed));
    });
    QObject::connect(rpc, &Aria2JsonRPC::refreshPeerStatus, this, [=](const QJsonArray &peerArray){
       if(currentTask && !this->isHidden())
       {           
           peerModel->setPeers(peerArray, currentTask->numPieces);
       }
    });
    QObject::connect(GlobalObjects::downloadModel, &DownloadModel::magnetDone, this, [=](const QString &path, const QString &magnet, bool directlyDownload){
        try
        {
            TorrentDecoder decoder(path);
            if (directlyDownload)
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
    QObject::connect(GlobalObjects::downloadModel, &DownloadModel::removeTask, this, [=](const QString &gid){
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
    rpc->exit();
}

QWidget *DownloadWindow::initLeftPanel(QWidget *parent)
{
    QWidget *leftPanel = new QWidget(parent);

    FontIconButton *unfinishedTasks{nullptr}, *finishedTasks{nullptr}, *autoRules{nullptr}, *bgmList{nullptr}, *resSearch{nullptr};
    FontIconButton **btnPtrs[] = {
        &unfinishedTasks, &finishedTasks, &autoRules, &bgmList, &resSearch
    };
    QVector<QPair<QChar, QString>> btnTexts {
        { QChar(0xe653), tr("Downloading")},
        { QChar(0xe69a), tr("Completed")},
        { QChar(0xe6e2), tr("Automatic Rules")},
        { QChar(0xe63a), tr("Daily Broadcast")},
        { QChar(0xea8a), tr("Resource Search")},
    };
    for (int i = 0; i < btnTexts.size(); ++i)
    {
        *btnPtrs[i] = new FontIconButton(btnTexts[i].first, "", 14, 14, 10, leftPanel);
        FontIconButton *btn = *btnPtrs[i];
        btn->setToolTip(btnTexts[i].second);
        btn->setObjectName(QStringLiteral("TaskTypeToolButton"));
        btn->setCheckable(true);
        btn->setContentsMargins(8, 4, 4, 4);
        btn->setMinimumWidth(46);
        if (i == 0)
        {
            btn->setAutoHideText(true);
            btn->setChecked(true);
        }
    }

    taskTypeButtonGroup = new QButtonGroup(parent);
    taskTypeButtonGroup->addButton(unfinishedTasks, 1);
    taskTypeButtonGroup->addButton(finishedTasks, 2);
    taskTypeButtonGroup->addButton(autoRules, 3);
    taskTypeButtonGroup->addButton(bgmList, 4);
    taskTypeButtonGroup->addButton(resSearch, 5);
    QObject::connect(taskTypeButtonGroup, &QButtonGroup::idToggled, this, [=](int id, bool checked){
        if (checked)
        {
            if (id < 3)
            {
                rightPanelSLayout->setCurrentIndex(0);
                TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
                model->setTaskStatus(id);
                model->setFilterKeyColumn(1);
            }
            else
            {
                rightPanelSLayout->setCurrentIndex(id - 2);
            }
        }
    });

    QVBoxLayout *leftVLayout=new QVBoxLayout(leftPanel);
    leftVLayout->setContentsMargins(16, 16, 12, 4);
    leftVLayout->setSpacing(4);

    leftVLayout->addSpacing(8);
    leftVLayout->addWidget(unfinishedTasks);
    leftVLayout->addWidget(finishedTasks);
    leftVLayout->addWidget(autoRules);
    leftVLayout->addSpacing(32);

    leftVLayout->addWidget(bgmList);
    leftVLayout->addWidget(resSearch);
    leftVLayout->addStretch(1);

    return leftPanel;
}

QWidget *DownloadWindow::initDownloadPage()
{
    QWidget *downloadContainer = new QWidget(this);
    downloadContainer->setMinimumWidth(220);

    FontIconButton *addUriTask = new FontIconButton(QChar(0xe604), tr("Add URI"), 12, 10, 2, downloadContainer);
    addUriTask->setObjectName(QStringLiteral("FontIconToolButton"));
    addUriTask->setContentsMargins(4, 2, 4, 2);
    QObject::connect(addUriTask, &FontIconButton::clicked, this, [=](){ this->addUrlTask(); });

    FontIconButton *addTorrentTask = new FontIconButton(QChar(0xe605), tr("Add Torrent") ,12, 10, 2,downloadContainer);
    addTorrentTask->setObjectName(QStringLiteral("FontIconToolButton"));
    addTorrentTask->setContentsMargins(4, 2, 4, 2);
    QObject::connect(addTorrentTask,&FontIconButton::clicked, this, &DownloadWindow::addTorrentTask);

    FontIconButton *settings = new FontIconButton(QChar(0xe615),tr("Settings"), 12, 10, 2, downloadContainer);
    settings->setObjectName(QStringLiteral("FontIconToolButton"));
    settings->setContentsMargins(4, 2, 4, 2);
    QObject::connect(settings, &FontIconButton::clicked, this, &DownloadWindow::showSettings);

    FontIconButton *sortBtn = new FontIconButton(QChar(0xe633), "", 14, 10, 2, downloadContainer);
    sortBtn->setObjectName(QStringLiteral("FontIconToolButton"));
    sortBtn->setContentsMargins(2, 2, 2, 2);

    KLineEdit *searchEdit = new KLineEdit(downloadContainer);
    searchEdit->setFont(QFont(GlobalObjects::normalFont, 12));
    searchEdit->setObjectName(QStringLiteral("DownloadSearchEdit"));
    searchEdit->setPlaceholderText(tr("Search Task"));
    searchEdit->setMinimumWidth(180);
    searchEdit->setClearButtonEnabled(true);
    QMargins textMargins = searchEdit->textMargins();
    textMargins.setLeft(6);
    searchEdit->setTextMargins(textMargins);

    QHBoxLayout *toolBarHLayout = new QHBoxLayout();
    toolBarHLayout->setContentsMargins(0,0,0,0);
    toolBarHLayout->addWidget(addUriTask);
    toolBarHLayout->addWidget(addTorrentTask);
    toolBarHLayout->addWidget(settings);
    toolBarHLayout->addStretch(1);
    toolBarHLayout->addWidget(sortBtn);
    toolBarHLayout->addWidget(searchEdit);

    downloadView = new QListView(downloadContainer);
    downloadView->setObjectName(QStringLiteral("DownloadView"));
    downloadView->setFont(QFont(GlobalObjects::normalFont, 10));
    downloadView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    downloadView->setMinimumWidth(220);
    downloadView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    new FloatScrollBar(downloadView->verticalScrollBar(), downloadView);

    downloadView->setItemDelegate(new DownloadItemDelegate(this));
    downloadView->setContextMenuPolicy(Qt::CustomContextMenu);
    TaskFilterProxyModel *proxyModel=new TaskFilterProxyModel(this);
    proxyModel->setSourceModel(GlobalObjects::downloadModel);
    downloadView->setModel(proxyModel);
    proxyModel->setTaskStatus(1);
    proxyModel->setFilterKeyColumn(static_cast<int>(DownloadModel::Columns::TITLE));

    QMenu *sortMenu = new ElaMenu(sortBtn);

    QActionGroup *ascDesc = new QActionGroup(sortMenu);
    QAction *actAsc = ascDesc->addAction(tr("Ascending"));
    QAction *actDesc = ascDesc->addAction(tr("Descending"));
    actAsc->setCheckable(true);
    actDesc->setCheckable(true);
    actDesc->setChecked(true);

    QActionGroup *orderTypes = new QActionGroup(sortMenu);
    QAction *actOrderCreateTime = orderTypes->addAction(tr("Create Time"));
    actOrderCreateTime->setData(static_cast<int>(DownloadModel::Columns::CREATETIME));
    actOrderCreateTime->setCheckable(true);
    QAction *actOrderTitle = orderTypes->addAction(tr("Title"));
    actOrderTitle->setData(static_cast<int>(DownloadModel::Columns::TITLE));
    actOrderTitle->setCheckable(true);
    QAction *actOrderSize = orderTypes->addAction(tr("Size"));
    actOrderSize->setData(static_cast<int>(DownloadModel::Columns::SIZE));
    actOrderSize->setCheckable(true);
    actOrderCreateTime->setChecked(true);

    QObject::connect(ascDesc, &QActionGroup::triggered, downloadView, [=](QAction *act){
        QAction *typeAction = orderTypes->checkedAction();
        if (typeAction && typeAction->data().isValid())
        {
            proxyModel->sort(typeAction->data().toInt(), act == actAsc ? Qt::SortOrder::AscendingOrder : Qt::SortOrder::DescendingOrder);
        }
    });
    bool orderAsc = GlobalObjects::appSetting->value("Download/SortOrderAscending", false).toBool();
    (orderAsc ? actAsc : actDesc)->trigger();

    QObject::connect(orderTypes, &QActionGroup::triggered, downloadView, [=](QAction *act){
        proxyModel->sort(act->data().toInt(), ascDesc->checkedAction() == actAsc ? Qt::SortOrder::AscendingOrder : Qt::SortOrder::DescendingOrder);
    });
    int orderType = GlobalObjects::appSetting->value("Download/SortOrderType", (int)DownloadModel::Columns::CREATETIME).toInt();
    if (orderType == (int)DownloadModel::Columns::TITLE)
    {
        actOrderTitle->trigger();
    }
    else if (orderType == (int)DownloadModel::Columns::SIZE)
    {
        actOrderSize->trigger();
    }

    sortMenu->addAction(actAsc);
    sortMenu->addAction(actDesc);
    sortMenu->addSeparator();
    sortMenu->addAction(actOrderCreateTime);
    sortMenu->addAction(actOrderTitle);
    sortMenu->addAction(actOrderSize);
    sortBtn->setMenu(sortMenu);


    QMenu *downloadMenu = new ElaMenu(downloadView);
    downloadMenu->addAction(act_addToPlayList);

    downloadMenu->addSeparator();
    downloadMenu->addAction(act_Pause);
    downloadMenu->addAction(act_Start);
    downloadMenu->addAction(act_Remove);

    downloadMenu->addSeparator();
    downloadMenu->addAction(act_CopyURI);
    downloadMenu->addAction(act_SaveTorrent);
    downloadMenu->addAction(act_BrowseFile);

    downloadMenu->addSeparator();
    downloadMenu->addAction(act_PauseAll);
    downloadMenu->addAction(act_StartAll);

    QObject::connect(downloadView, &QListView::customContextMenuRequested, downloadView, [=](){
        downloadMenu->exec(QCursor::pos());
    });

    QObject::connect(downloadView, &QListView::doubleClicked, this, [=](const QModelIndex &index){
        DownloadTask *task=GlobalObjects::downloadModel->getDownloadTask(proxyModel->mapToSource(index));
        if (task->status == DownloadTask::Complete || task->status == DownloadTask::Paused || task->status == DownloadTask::Error)
        {
            act_Start->trigger();
        }
        else
        {
            act_Pause->trigger();
        }
    });
    QObject::connect(downloadView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DownloadWindow::downloadSelectionChanged);
    QObject::connect(searchEdit, &QLineEdit::textChanged, downloadView, [=](const QString &keyword){
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterRegularExpression(keyword);
    });

    QWidget *detailInfoContent = new QWidget(downloadContainer);
    detailInfoContent->setContentsMargins(0, 0, 0, 0);
    QStackedLayout *detailInfoSLayout = new QStackedLayout(detailInfoContent);
    detailInfoSLayout->addWidget(setupGeneralInfoPage(detailInfoContent));
    detailInfoSLayout->addWidget(setupFileInfoPage(detailInfoContent));
    detailInfoSLayout->addWidget(setupBlockPage(detailInfoContent));
    detailInfoSLayout->addWidget(setupConnectionPage(detailInfoContent));
    detailInfoSLayout->addWidget(setupGlobalLogPage(detailInfoContent));
    detailInfoContent->hide();

    QSplitter *viewBottomSplitter = new QSplitter(Qt::Vertical, downloadContainer);
    viewBottomSplitter->setObjectName(QStringLiteral("NormalSplitter"));
    viewBottomSplitter->addWidget(downloadView);
    viewBottomSplitter->addWidget(detailInfoContent);
    viewBottomSplitter->setStretchFactor(0, 8);
    viewBottomSplitter->setStretchFactor(1, 3);
    viewBottomSplitter->setCollapsible(0, false);
    viewBottomSplitter->setCollapsible(1, false);

    QStringList pageButtonTexts = {
        tr("General"),
        tr("File"),
        tr("Block"),
        tr("Connection"),
        tr("Log"),
    };
    QFontMetrics fm = QToolButton().fontMetrics();
    int btnWidth = 0;
    for(const QString &t : pageButtonTexts)
    {
        btnWidth = qMax(btnWidth, fm.horizontalAdvance(t));
    }
    btnWidth += 30;


    QHBoxLayout *pageBarHLayout = new QHBoxLayout();
    pageBarHLayout->setSpacing(2);
    pageBarHLayout->setContentsMargins(0, 4, 0, 0);
    QVector<QToolButton *> pageBtns;
    for (int i = 0; i < pageButtonTexts.size(); ++i)
    {
        QToolButton *btn = new QToolButton(downloadContainer);
        btn->setText(pageButtonTexts[i]);
        btn->setCheckable(true);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setObjectName(QStringLiteral("DownloadInfoPageButton"));
        btn->setFixedWidth(btnWidth);
        pageBarHLayout->addWidget(btn);
        pageBtns << btn;
    }

    auto btnCheckFunc = [=](int index, bool check){
        for (int i = 0; i < pageBtns.size(); ++i)
        {
            pageBtns[i]->setChecked(i == index ? check : false);
        }
        detailInfoSLayout->setCurrentIndex(index);
        detailInfoContent->setVisible(check);
    };

    for (int i = 0; i < pageBtns.size(); ++i)
    {
        QObject::connect(pageBtns[i], &QPushButton::clicked, this, [=](bool check){
            btnCheckFunc(i, check);
        });
    }

    GlobalObjects::iconfont->setPointSize(12);
    downSpeedIconLabel = new QLabel("--", downloadContainer);
    downSpeedIconLabel->setObjectName(QStringLiteral("DownSpeedIcon"));
    downSpeedIconLabel->setFont(*GlobalObjects::iconfont);
    downSpeedIconLabel->setText(QChar(0xe910));
    downSpeedIconLabel->setMaximumWidth(downSpeedIconLabel->height() + 4);
    upSpeedIconLabel = new QLabel("--", downloadContainer);
    upSpeedIconLabel->setObjectName(QStringLiteral("UpSpeedIcon"));
    upSpeedIconLabel->setFont(*GlobalObjects::iconfont);
    upSpeedIconLabel->setText(QChar(0xe941));
    upSpeedIconLabel->setMaximumWidth(upSpeedIconLabel->height() + 4);
    downSpeedLabel = new QLabel(downloadContainer);
    downSpeedLabel->setObjectName(QStringLiteral("DownSpeedLabel"));
    upSpeedLabel = new QLabel(downloadContainer);
    upSpeedLabel->setObjectName(QStringLiteral("UpSpeedLabel"));

    QGridLayout *speedGLayout = new QGridLayout;
    speedGLayout->addWidget(downSpeedIconLabel, 0, 0);
    speedGLayout->addWidget(downSpeedLabel, 0, 1);
    speedGLayout->addWidget(upSpeedIconLabel, 0, 2);
    speedGLayout->addWidget(upSpeedLabel, 0, 3);
    speedGLayout->setContentsMargins(0, 0, 0, 0);
    speedGLayout->setSpacing(4);
    speedGLayout->setColumnStretch(1, 1);
    speedGLayout->setColumnStretch(3, 1);
    speedGLayout->setRowStretch(1, 1);
    speedGLayout->setColumnMinimumWidth(1, 64);
    speedGLayout->setColumnMinimumWidth(3, 64);

    pageBarHLayout->addStretch(1);
    pageBarHLayout->addLayout(speedGLayout);

    QGridLayout *downContainerGLayout = new QGridLayout(downloadContainer);
    downContainerGLayout->setVerticalSpacing(4);
    downContainerGLayout->addLayout(toolBarHLayout, 0, 1);
    downContainerGLayout->addWidget(viewBottomSplitter, 1, 1);
    downContainerGLayout->addLayout(pageBarHLayout,2, 1);
    downContainerGLayout->setRowStretch(1, 1);

    return downloadContainer;
}

QWidget *DownloadWindow::setupGeneralInfoPage(QWidget *parent)
{
    QWidget *content = new QWidget(parent);
    QGridLayout *gInfoGLayout=new QGridLayout(content);
    taskTitleLabel=new QLabel(content);
    taskTitleLabel->setFont(QFont(GlobalObjects::normalFont,12));
    taskTitleLabel->setObjectName(QStringLiteral("TaskTitleLabel"));
    taskTitleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    taskTimeLabel=new QLabel(content);
    taskTimeLabel->setObjectName(QStringLiteral("TaskTimeLabel"));
    taskTimeLabel->setOpenExternalLinks(true);

    gInfoGLayout->addWidget(taskTitleLabel,0,0);
    gInfoGLayout->addWidget(taskTimeLabel,1,0);
    gInfoGLayout->setRowStretch(3,1);
    gInfoGLayout->setColumnStretch(0,1);
    return content;
}

QWidget *DownloadWindow::setupFileInfoPage(QWidget *parent)
{
    selectedTFModel = new CTorrentFileModel(this);
    fileInfoView = new TorrentTreeView(parent);
    fileInfoView->setAlternatingRowColors(true);
    fileInfoView->setModel(selectedTFModel);
    fileInfoView->setItemDelegate(new KTreeviewItemDelegate(fileInfoView));
    new FloatScrollBar(fileInfoView->verticalScrollBar(), fileInfoView);
    new FloatScrollBar(fileInfoView->horizontalScrollBar(), fileInfoView);
    fileInfoView->header()->resizeSection(0, 300);
    fileInfoView->setFont(QFont(GlobalObjects::normalFont, 10));
    QObject::connect(fileInfoView, &TorrentTreeView::ignoreColorChanged, selectedTFModel, &CTorrentFileModel::setIgnoreColor);
    QObject::connect(fileInfoView, &TorrentTreeView::normColorChanged, selectedTFModel, &CTorrentFileModel::setNormColor);

    QObject::connect(fileInfoView, &QTreeView::doubleClicked, fileInfoView, [=](const QModelIndex &index){
        TorrentFile *item = static_cast<TorrentFile*>(index.internalPointer());
        if (currentTask && item->index > 0 && item->completedSize == item->size)
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
    QObject::connect(selectedTFModel, &CTorrentFileModel::checkedIndexChanged, selectedTFModel, [this](){
        if (!currentTask) return;
        QString selIndexes(selectedTFModel->getCheckedIndex());
        if (selIndexes != currentTask->selectedIndexes)
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
    new FloatScrollBar(contentScrollArea->verticalScrollBar(), contentScrollArea, false);
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
    new FloatScrollBar(peerView->verticalScrollBar(), peerView);
    new FloatScrollBar(peerView->horizontalScrollBar(), peerView);
    peerView->header()->resizeSection(static_cast<int>(PeerModel::Columns::PROGRESS), 280);
    peerView->header()->resizeSection(static_cast<int>(PeerModel::Columns::CLIENT), 180);
    peerView->header()->resizeSection(static_cast<int>(PeerModel::Columns::IP), 160);
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
    QPlainTextEdit *logView = new KPlainTextEdit(parent);
    new FloatScrollBar(logView->verticalScrollBar(), logView);
    logView->setReadOnly(true);
    logView->setCenterOnScroll(true);
    logView->setMaximumBlockCount(50);
    // logView->setObjectName(QStringLiteral("TaskLogView"));
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
        QItemSelection selection = model->mapSelectionToSource(downloadView->selectionModel()->selection());
        if (selection.size() == 0) return;

        for(const QModelIndex &index: selection.indexes())
        {
            DownloadTask* task = GlobalObjects::downloadModel->getDownloadTask(index);
            if (task->gid.isEmpty()) continue;
            if (task->status == DownloadTask::Paused || task->status == DownloadTask::Complete) continue;
            rpc->switchPauseStatus(task->gid, true);
        }
    });
    act_Start=new QAction(tr("Start"), this);
    QObject::connect(act_Start,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QItemSelection selection = model->mapSelectionToSource(downloadView->selectionModel()->selection());
        if (selection.size() == 0) return;

        for (const QModelIndex& index : selection.indexes())
        {
            DownloadTask* task = GlobalObjects::downloadModel->getDownloadTask(index);
            if (task->gid.isEmpty())
            {
                QFileInfo fi(task->dir, task->title + ".aria2");
                if (!fi.exists())
                {
                    QMessageBox::StandardButton btn = QMessageBox::information(this, tr("Resume"), tr("Control file(*.aria2) does not exist, download the file all over again ?\n%1").arg(task->title),
                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
                    if (btn == QMessageBox::Cancel) return;
                    GlobalObjects::downloadModel->restartDownloadTask(task, btn == QMessageBox::Yes);
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
            rpc->switchPauseStatus(task->gid, false);
        }
    });
    act_Remove=new QAction(tr("Remove"),this);
    QObject::connect(act_Remove,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QItemSelection selection = model->mapSelectionToSource(downloadView->selectionModel()->selection());
        if (selection.size() == 0) return;

        QMessageBox::StandardButton btn = QMessageBox::information(this,tr("Remove"),tr("Delete the Downloaded %1 Files?").arg(selection.size()),
                                 QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::No);
        if(btn==QMessageBox::Cancel)return;
        QModelIndexList sourceIndexes = selection.indexes();
        for (const QModelIndex& index : sourceIndexes)
        {
            if (currentTask == GlobalObjects::downloadModel->getDownloadTask(index))
            {
                setDetailInfo(nullptr);
            }
        }
        GlobalObjects::downloadModel->removeItem(sourceIndexes, btn==QMessageBox::Yes);
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
        QItemSelection selection = model->mapSelectionToSource(downloadView->selectionModel()->selection());
        if (selection.size() == 0) return;

        DownloadTask *task=GlobalObjects::downloadModel->getDownloadTask(selection.indexes().last());
        QFileInfo info(task->dir,task->title);
        if(!info.exists())return;
#ifdef Q_OS_WIN
        QProcess::startDetached("Explorer", {"/select,",  QDir::toNativeSeparators(info.absoluteFilePath())});
#else
        QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(info.absolutePath())));
#endif
    });
    act_addToPlayList=new QAction(tr("Add To PlayList"),this);
    QObject::connect(act_addToPlayList,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QItemSelection selection = model->mapSelectionToSource(downloadView->selectionModel()->selection());
        if (selection.size() == 0) return;

        for (const QModelIndex& index : selection.indexes())
        {
            DownloadTask* task = GlobalObjects::downloadModel->getDownloadTask(index);
            QFileInfo info(task->dir, task->title);
            if (!info.exists())continue;
            if (info.isDir())
                GlobalObjects::playlist->addFolder(info.absoluteFilePath(), QModelIndex());
            else
                GlobalObjects::playlist->addItems(QStringList() << info.absoluteFilePath(), QModelIndex());
        }
    });
    QObject::connect(GlobalObjects::downloadModel,&DownloadModel::taskFinish,this,[this](DownloadTask *task){
        if (GlobalObjects::appSetting->value("Download/AutoAddToList", false).toBool())
        {
            QFileInfo info(task->dir, task->title);
            if (!info.exists()) return;
            if (info.isDir())
                GlobalObjects::playlist->addFolder(info.absoluteFilePath(), QModelIndex());
            else
                GlobalObjects::playlist->addItems(QStringList() << info.absoluteFilePath(), QModelIndex());
        }
        if (this->isHidden())
        {
            TipParams param;
            param.showClose = true;
            param.group = "kiko-download";
            param.message = tr("Download Complete: %1").arg(task->title);
            Notifier::getNotifier()->showMessage(Notifier::GLOBAL_NOTIFY, "", 0, QVariant::fromValue(param));
        }
    });

    act_CopyURI=new QAction(tr("Copy URI"),this);
    QObject::connect(act_CopyURI,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QItemSelection selection = model->mapSelectionToSource(downloadView->selectionModel()->selection());
        if (selection.size() == 0) return;
        DownloadTask* task = GlobalObjects::downloadModel->getDownloadTask(selection.indexes().last());
        QClipboard *cb = QApplication::clipboard();
        cb->setText(task->uri);
    });
    act_SaveTorrent=new QAction(tr("Save Torrent"),this);
    QObject::connect(act_SaveTorrent,&QAction::triggered,[this](){
        TaskFilterProxyModel *model = static_cast<TaskFilterProxyModel *>(downloadView->model());
        QItemSelection selection = model->mapSelectionToSource(downloadView->selectionModel()->selection());
        if (selection.size() == 0) return;
        DownloadTask* task = GlobalObjects::downloadModel->getDownloadTask(selection.indexes().last());
        if (task->torrentContentState == -1) GlobalObjects::downloadModel->tryLoadTorrentContent(task);
        if (task->torrentContent.isEmpty()) return;
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Torrent"), task->title, "Torrent File (*.torrent)");
        if (!fileName.isEmpty())
        {
            QFile torrnentFile(fileName);
            bool ret = torrnentFile.open(QIODevice::WriteOnly);
            if (!ret) return;
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
    if (task)
    {
        act_CopyURI->setEnabled(!task->uri.isEmpty());
        currentTask=task;
        taskTitleLabel->setText(task->title);
        QStringList taskInfo;
        const QString itemTpl = "<p><font style='color: #d0d0d0;'>%1</font>%2</p>";
        taskInfo.append(itemTpl.arg(tr("Create Time: ")).arg(QDateTime::fromSecsSinceEpoch(task->createTime).toString("yyyy-MM-dd hh:mm:ss")));
        taskInfo.append(itemTpl.arg(tr("Finish Time: ")).arg(task->finishTime < task->createTime ? "----" : QDateTime::fromSecsSinceEpoch(task->finishTime).toString("yyyy-MM-dd hh:mm:ss")));
        const QString locTpl = "<a style='color: rgb(96, 208, 252);' href=\"file:///%1\">%2</a>";
        taskInfo.append(itemTpl.arg(tr("Save Location: ")).arg(locTpl.arg(task->dir, task->dir)));
        taskTimeLabel->setText(taskInfo.join("\n"));
        taskTimeLabel->adjustSize();
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
        taskTimeLabel->clear();
        blockView->setBlock(0, "0");
        blockView->setToolTip("");
        selectedTFModel->setContent(nullptr);
        act_CopyURI->setEnabled(false);
        act_SaveTorrent->setEnabled(false);
    }
}

void DownloadWindow::addUrlTask(const QStringList &urls, const QString &path)
{
    AddUriTask addUriTaskDialog(this, urls, path);
    if (QDialog::Accepted==addUriTaskDialog.exec())
    {
        bool directDownload = GlobalObjects::appSetting->value("Download/SkipMagnetFileSelect", false).toBool();
        for (const QString &uri : addUriTaskDialog.uriList)
        {
            QString errInfo(GlobalObjects::downloadModel->addUriTask(uri, addUriTaskDialog.dir, directDownload));
            if(!errInfo.isEmpty())
            {
                QMessageBox::information(this,tr("Error"),tr("An error occurred while adding : URI:\n %1 \n %2").arg(uri).arg(errInfo));
            }
        }
    }
}

void DownloadWindow::addTorrentTask()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select Torrent File"),"","Torrent(*.torrent) ");
    if (!file.isEmpty())
    {
        try
        {
            TorrentDecoder decoder(file);
            SelectTorrentFile selectTorrentFile(decoder.root,this);
            QCoreApplication::processEvents();
            if (QDialog::Accepted == selectTorrentFile.exec())
            {
                QString errInfo(GlobalObjects::downloadModel->addTorrentTask(decoder.rawContent, decoder.infoHash,
                                selectTorrentFile.dir, selectTorrentFile.selectIndexes, QString()));
                if (!errInfo.isEmpty())
                {
                    QMessageBox::information(this,tr("Error"),tr("An error occurred while adding Torrent : \n %1 ").arg(errInfo));
                }
            }
            delete decoder.root;
        }
        catch(TorrentError &err)
        {
            QMessageBox::information(this,tr("Error"),err.errorInfo);
        }
    }
}

void DownloadWindow::showSettings()
{
    Settings settings(Settings::PAGE_DOWN, this);
    const SettingPage *downPage = settings.getPage(Settings::PAGE_DOWN);
    settings.exec();

    QJsonObject globalOptions,taskOptions;
    const auto &changedValues = downPage->getChangedValues();
    if (changedValues.contains("downSpeed"))
    {
        globalOptions.insert("max-overall-download-limit", QString::number(changedValues["downSpeed"].toInt()) + "K");
    }
    if (changedValues.contains("upSpeed"))
    {
        globalOptions.insert("max-overall-upload-limit", QString::number(changedValues["upSpeed"].toInt()) + "K");
    }
    if (changedValues.contains("concurrent"))
    {
        globalOptions.insert("max-concurrent-downloads", QString::number(changedValues["concurrent"].toInt()));
    }
    if (changedValues.contains("seedTime"))
    {
        taskOptions.insert("seed-time", QString::number(changedValues["seedTime"].toInt()));
    }
    if (changedValues.contains("btTracker"))
    {
        taskOptions.insert("bt-tracker", changedValues["btTracker"].toStringList().join(','));
    }
    if (globalOptions.count() > 0)
    {
        rpc->changeGlobalOption(globalOptions);
    }
    if (taskOptions.count() > 0)
    {
        auto &items = GlobalObjects::downloadModel->getItems();
        for (auto iter=items.cbegin(); iter!=items.cend(); ++iter)
        {
            if (iter.value()->status != DownloadTask::Complete)
            {
                rpc->changeOption(iter.key(), taskOptions);
            }
        }
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

void DownloadWindow::showMessage(const QString &content, int flag, const QVariant &)
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


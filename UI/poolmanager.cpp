#include "poolmanager.h"
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QAction>
#include <QSortFilterProxyModel>
#include <QApplication>
#include "Play/Danmu/Manager/managermodel.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Playlist/playlistitem.h"
#include "Play/Playlist/playlist.h"
#include "Common/notifier.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaMenu.h"
#include "UI/ela/ElaSpinBox.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "UI/widgets/kpushbutton.h"
#include "dialogs/timelineedit.h"
#include "dialogs/adddanmu.h"
#include "addpool.h"
#include "dialogs/danmuview.h"
#include "inputdialog.h"
#include "globalobjects.h"
namespace
{
    static QCollator comparer;
}
PoolManager::PoolManager(QWidget *parent) : CFramelessDialog(tr("Danmu Pool Manager"),parent)
{
    comparer.setNumericMode(true);
    setFont(QFont(GlobalObjects::normalFont, 10));
    QTreeView *poolView=new QTreeView(this);
    poolView->setItemDelegate(new KTreeviewItemDelegate(poolView));
    poolView->setSelectionMode(QAbstractItemView::SingleSelection);
    poolView->setFont(this->font());
    poolView->setAnimated(true);
    poolView->setAlternatingRowColors(true);
    poolView->header()->setSortIndicator(0, Qt::SortOrder::AscendingOrder);

    DanmuManagerModel *managerModel=new DanmuManagerModel(this);
    PoolSortProxyModel *proxyModel=new PoolSortProxyModel(this);
    proxyModel->setSourceModel(managerModel);
    proxyModel->setFilterKeyColumn(0);
    proxyModel->setRecursiveFilteringEnabled(true);
    poolView->setModel(proxyModel);
    poolView->setSortingEnabled(true);
    poolView->setContextMenuPolicy(Qt::CustomContextMenu);

    QLabel *stateLabel=new QLabel(this);
    stateLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    QObject::connect(GlobalObjects::danmuManager,&DanmuManager::workerStateMessage,stateLabel,[stateLabel,managerModel](const QString &msg){
       if(msg=="Done")
           stateLabel->setText(tr("Pool: %1 Danmu: %2").arg(managerModel->totalPoolCount()).arg(managerModel->totalDanmuCount()));
       else
           stateLabel->setText(msg);
    });

    QAction *act_editTimeLine=new QAction(tr("Edit TimeLine"),this);
    QObject::connect(act_editTimeLine,&QAction::triggered,this,[this,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        DanmuPoolSourceNode *srcNode=managerModel->getSourceNode(proxyModel->mapToSource(indexList.first()));
        if (!srcNode) return;
        QVector<SimpleDanmuInfo> simpleDanmuList;
        Pool *pool=GlobalObjects::danmuManager->getPool(srcNode->parent->idInfo);
        if(pool)
        {
            pool->exportSimpleInfo(srcNode->srcId,simpleDanmuList);
            DanmuSource srcInfo(pool->sources()[srcNode->srcId]);
            TimelineEdit timeLineEdit(&srcInfo,simpleDanmuList,this);
            if(QDialog::Accepted==timeLineEdit.exec())
            {
                pool->setTimeline(srcNode->srcId,timeLineEdit.timelineInfo);
                srcNode->hasTimeline = !pool->sources()[srcNode->srcId].timelineInfo.isEmpty();
            }
        }

    });
    QAction *act_addWebSource=new QAction(tr("Add Web Source"),this);
    QObject::connect(act_addWebSource,&QAction::triggered,this,[this,stateLabel,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        DanmuPoolNode *poolNode=managerModel->getPoolNode(proxyModel->mapToSource(indexList.first()));
        if(!poolNode)return;
        PlayListItem item;
        item.title=poolNode->title;
        item.animeTitle=poolNode->parent->title;
        item.poolID = poolNode->idInfo;
        QMap<QString,DanmuPoolNode *> poolNodeMap;
        QStringList poolTitles;
        for(auto node:*poolNode->parent->children)
        {
            poolNodeMap.insert(node->title,node);
            poolTitles<<node->title;
        }
        std::sort(poolTitles.begin(),poolTitles.end(), [&](const QString &s1, const QString &s2){
            return comparer.compare(s1, s2)>=0?false:true;
        });
        AddDanmu addDanmuDialog(&item, this,false,poolTitles);
        if (QDialog::Accepted == addDanmuDialog.exec())
        {
            poolView->setEnabled(false);
            this->showBusyState(true);
            auto &infoList = addDanmuDialog.danmuInfoList;
            for (SearchDanmuInfo &info : infoList)
            {
                DanmuPoolNode *curNode = poolNodeMap.value(info.pool);
                Q_ASSERT(curNode);
                Pool *pool = GlobalObjects::danmuManager->getPool(curNode->idInfo);
                Q_ASSERT(pool);
                int srcId = pool->addSource(info.src, info.danmus, true);
                info.src.id = srcId;
                if (srcId < 0)
                {
                    showMessage(tr("Add %1 Failed").arg(info.src.title), NM_ERROR | NM_HIDE);
                    qDeleteAll(info.danmus);
                    continue;
                }
                DanmuPoolSourceNode *sourceNode(nullptr);
                for (auto n : *curNode->children)
                {
                    DanmuPoolSourceNode *srcNode = static_cast<DanmuPoolSourceNode *>(n);
                    if (srcNode->idInfo == info.src.scriptId && srcNode->scriptData == info.src.scriptData)
                    {
                        sourceNode=srcNode;
                        break;
                    }
                }
                managerModel->addSrcNode(curNode, sourceNode ? nullptr : new DanmuPoolSourceNode(info.src));
            }
            stateLabel->setText(tr("Pool: %1 Danmu: %2").arg(managerModel->totalPoolCount()).arg(managerModel->totalDanmuCount()));
            poolView->setEnabled(true);
            this->showBusyState(false);
        }
    });

    QAction *act_pastePoolCode=new QAction(tr("Paste Danmu Pool Code"),this);
    QObject::connect(act_pastePoolCode,&QAction::triggered,this,[this,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        DanmuPoolNode *poolNode=managerModel->getPoolNode(proxyModel->mapToSource(indexList.first()));
        if(!poolNode)return;
        QClipboard *cb = QApplication::clipboard();
        QString code(cb->text());
        if(code.isEmpty() ||
                (!code.startsWith("kikoplay:pool=") &&
                !code.startsWith("kikoplay:anime="))) return;
        poolView->setEnabled(false);
        this->showBusyState(true);
        Pool *pool = GlobalObjects::danmuManager->getPool(poolNode->idInfo,false);
        bool ret = pool->addPoolCode(
                    code.mid(code.startsWith("kikoplay:pool=")?14:15),
                    code.startsWith("kikoplay:anime="));
        if(ret)
        {
            for(const DanmuSource &sourceInfo:pool->sources())
            {
                bool isNewSource=true;
                for(auto n:*poolNode->children)
                {
                    DanmuPoolSourceNode *srcNode=static_cast<DanmuPoolSourceNode *>(n);
                    if(srcNode->isSameSource(sourceInfo))
                    {
                        isNewSource=false;
                        break;
                    }
                }
                if(isNewSource)
                {
                    DanmuPoolSourceNode *sourceNode=new DanmuPoolSourceNode(sourceInfo);
                    managerModel->addSrcNode(poolNode,sourceNode);
                }
            }
        }
        poolView->setEnabled(true);
        this->showBusyState(false);
        this->showMessage(ret?tr("Code Added"):tr("Code Error"),ret?NM_HIDE:NM_ERROR | NM_HIDE);
    });
    QAction *act_copyPoolCode=new QAction(tr("Copy Danmu Pool Code"),this);
    QObject::connect(act_copyPoolCode,&QAction::triggered,this,[this,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        DanmuPoolNode *poolNode=managerModel->getPoolNode(proxyModel->mapToSource(indexList.first()));
        if(!poolNode)return;
        Pool *pool = GlobalObjects::danmuManager->getPool(poolNode->idInfo,false);
        QString code(pool->getPoolCode());
        if(code.isEmpty())
        {
            showMessage(tr("No Danmu Source to Share"), NM_ERROR | NM_HIDE);
        }
        else
        {
            QClipboard *cb = QApplication::clipboard();
            cb->setText("kikoplay:pool="+code);
            showMessage(tr("Pool Code has been Copied to Clipboard"));
        }
    });

    QAction *act_addPool=new QAction(tr("Add Pool"),this);
    QObject::connect(act_addPool,&QAction::triggered,this,[this,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        QString animeTitle=managerModel->getAnime(proxyModel->mapToSource(indexList.first()));;
        AddPool addPool(this,animeTitle);
        if(QDialog::Accepted==addPool.exec())
        {
            EpInfo ep(addPool.epType,addPool.epIndex, addPool.ep);
            QString pid(GlobalObjects::danmuManager->createPool(addPool.anime,ep.type,ep.index,ep.name));
            managerModel->addPoolNode(addPool.anime,ep,pid);
        }

    });

    QAction *act_renamePool=new QAction(tr("Rename Pool"),this);
    QObject::connect(act_renamePool,&QAction::triggered,this,[this,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        DanmuPoolNode *poolNode=managerModel->getPoolNode(proxyModel->mapToSource(indexList.first()));
        if(!poolNode)return;
        Q_ASSERT(poolNode->type==DanmuPoolNode::EpNode);
        DanmuPoolEpNode *epNode = static_cast<DanmuPoolEpNode *>(poolNode);
        AddPool addPool(this,poolNode->parent->title, EpInfo(epNode->epType, epNode->epIndex, epNode->epName));
        if(QDialog::Accepted==addPool.exec())
        {
            QString opid(poolNode->idInfo);
            EpInfo nEp(addPool.epType, addPool.epIndex, addPool.ep);
            QString npid = GlobalObjects::danmuManager->renamePool(opid,addPool.anime,nEp.type,nEp.index,nEp.name);
            if(npid.isEmpty())
            {
                showMessage(tr("Rename Failed, Try Again?"), NM_ERROR | NM_HIDE);
                return;
            }
            if(opid==npid && epNode->epName==nEp.name) return;
            managerModel->renamePoolNode(poolNode,addPool.anime,nEp.toString(),npid);
            GlobalObjects::playlist->renameItemPoolId(opid,npid);
        }

    });
    QAction *actView=new QAction(tr("View Danmu"),this);
    QObject::connect(actView,&QAction::triggered,this,[this,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        DanmuPoolNode *poolNode=managerModel->getPoolNode(proxyModel->mapToSource(indexList.first()));
        if(!poolNode)return;
        Pool *pool=GlobalObjects::danmuManager->getPool(poolNode->idInfo);
        DanmuPoolSourceNode *sourceNode=managerModel->getSourceNode(proxyModel->mapToSource(indexList.first()));
        DanmuView view(&pool->comments(),this,  sourceNode?sourceNode->srcId:-1);
        view.exec();
    });

    ElaMenu *actionMenu = new ElaMenu(poolView);
    QObject::connect(poolView, &QTreeView::customContextMenuRequested, this, [=](){
        if (!poolView->selectionModel()->hasSelection()) return;
        const QModelIndex selectIndex = proxyModel->mapToSource(poolView->selectionModel()->selectedRows().first());
        bool isPoolNode = managerModel->getPoolNode(selectIndex);
        bool isSourceNode = managerModel->getSourceNode(selectIndex);

        actView->setEnabled(isPoolNode);
        act_addWebSource->setEnabled(isPoolNode);
        act_editTimeLine->setEnabled(isSourceNode);
        act_renamePool->setEnabled(isPoolNode);
        act_copyPoolCode->setEnabled(isPoolNode);
        act_pastePoolCode->setEnabled(isPoolNode);

        actionMenu->exec(QCursor::pos());
    });

    actionMenu->addAction(actView);
    actionMenu->addAction(act_addWebSource);
    actionMenu->addAction(act_editTimeLine);
    actionMenu->addSeparator();

    actionMenu->addAction(act_addPool);
    actionMenu->addAction(act_renamePool);
    actionMenu->addSeparator();

    actionMenu->addAction(act_copyPoolCode);
    actionMenu->addAction(act_pastePoolCode);

    KPushButton *cancel = new KPushButton(tr("Cancel"),this);
    cancel->hide();

    QWidget *exportPage=new QWidget(this);
    QHBoxLayout *exportHLayout=new QHBoxLayout(exportPage);
    exportHLayout->setContentsMargins(0,0,0,0);
    QCheckBox *exportKdFile = new ElaCheckBox(tr("Export KikoPlay Format"), exportPage);
    QCheckBox *useTimelineCheck = new ElaCheckBox(tr("Apply delay and timeline info"), exportPage);
    useTimelineCheck->setChecked(true);
    QCheckBox *useBlockRule = new ElaCheckBox(tr("Apply block rules"),exportPage);
    KPushButton *exportConfirm=new KPushButton(tr("Export"),exportPage);
    exportHLayout->addWidget(exportKdFile);
    exportHLayout->addWidget(useTimelineCheck);
    exportHLayout->addWidget(useBlockRule);
    exportHLayout->addStretch(1);
    exportHLayout->addWidget(exportConfirm);

    QObject::connect(exportKdFile, &QCheckBox::stateChanged, this, [=](int state){
       useBlockRule->setEnabled(state!=Qt::Checked);
       useTimelineCheck->setEnabled(state!=Qt::Checked);
    });

    QObject::connect(exportConfirm, &KPushButton::clicked, this, [=](){
        if (!managerModel->hasSelected()) return;
        const QString lastDirKey = "FileDialogPath/PoolManagerExport";
        const QString lastDir = GlobalObjects::appSetting->value(lastDirKey).toString();
        QString directory = QFileDialog::getExistingDirectory(this,
                                    tr("Select folder"), lastDir,
                                    QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly);
        if (directory.isEmpty()) return;
        GlobalObjects::appSetting->setValue(lastDirKey, directory);
        this->showBusyState(true);
        exportConfirm->setText(tr("Exporting..."));
        exportConfirm->setEnabled(false);
        poolView->setEnabled(false);
        cancel->setEnabled(false);
        useTimelineCheck->setEnabled(false);
        useBlockRule->setEnabled(false);
        if(exportKdFile->isChecked())
        {
            InputDialog inputDialog(tr("Set Comment"),tr("Comment(Optional)"),"",true,this);
            inputDialog.exec();
            managerModel->exportKdFile(directory, inputDialog.text);
        }
        else
        {
            managerModel->exportPool(directory,useTimelineCheck->isChecked(),useBlockRule->isChecked());
        }
        this->showBusyState(false);
        exportConfirm->setText(tr("Export"));
        exportConfirm->setEnabled(true);
        poolView->setEnabled(true);
        cancel->setEnabled(true);
        useTimelineCheck->setEnabled(true);
        useBlockRule->setEnabled(true);
    });

    QWidget *deletePage=new QWidget(this);
    QHBoxLayout *deleteHLayout=new QHBoxLayout(deletePage);
    deleteHLayout->setContentsMargins(0,0,0,0);
    QLabel *deleteTipLabel=new QLabel(tr("Check the items to delete"),deletePage);
    KPushButton *deleteConfirm=new KPushButton(tr("Delete"),deletePage);
    deleteHLayout->addWidget(deleteTipLabel);
    deleteHLayout->addStretch(1);
    deleteHLayout->addWidget(deleteConfirm);
    QObject::connect(deleteConfirm,&KPushButton::clicked,[this,stateLabel,poolView,managerModel,deleteConfirm,cancel](){
        if(!managerModel->hasSelected())return;
        this->showBusyState(true);
        deleteConfirm->setText(tr("Deleting..."));
        deleteConfirm->setEnabled(false);
        poolView->setEnabled(false);
        cancel->setEnabled(false);
        managerModel->deletePool();
        this->showBusyState(false);
        deleteConfirm->setText(tr("Delete"));
        deleteConfirm->setEnabled(true);
        poolView->setEnabled(true);
        cancel->setEnabled(true);
        stateLabel->setText(tr("Pool: %1 Danmu: %2").arg(managerModel->totalPoolCount()).arg(managerModel->totalDanmuCount()));
    });

    QWidget *updatePage=new QWidget(this);
    QHBoxLayout *updateHLayout=new QHBoxLayout(updatePage);
    updateHLayout->setContentsMargins(0,0,0,0);
    QLabel *updateTipLabel=new QLabel(tr("Check the items to update danmu"),updatePage);
    KPushButton *updateConfirm=new KPushButton(tr("Update"),updatePage);
    updateHLayout->addWidget(updateTipLabel);
    updateHLayout->addStretch(1);
    updateHLayout->addWidget(updateConfirm);
    QObject::connect(updateConfirm,&KPushButton::clicked,[this,stateLabel,poolView,managerModel,updateConfirm,cancel](){
        if(!managerModel->hasSelected())return;
        this->showBusyState(true);
        updateConfirm->setText(tr("Updating..."));
        cancel->setEnabled(false);
        poolView->setEnabled(false);
        updateConfirm->setEnabled(false);
        managerModel->updatePool();
        this->showBusyState(false);
        updateConfirm->setText(tr("Update"));
        poolView->setEnabled(true);
        updateConfirm->setEnabled(true);
        cancel->setEnabled(true);
        stateLabel->setText(tr("Pool: %1 Danmu: %2").arg(managerModel->totalPoolCount()).arg(managerModel->totalDanmuCount()));
    });

    QWidget *setDelayPage = new QWidget(this);
    QHBoxLayout *delayHLayout = new QHBoxLayout(setDelayPage);
    setDelayPage->setContentsMargins(0, 0, 0, 0);
    QLabel *setDelayTipLabel = new QLabel(tr("Check the items to set delay(s)"), setDelayPage);
    ElaSpinBox *delaySpin = new ElaSpinBox(setDelayPage);
    delaySpin->setRange(INT_MIN, INT_MAX);
    KPushButton *delayConfirm = new KPushButton(tr("Set"), setDelayPage);
    delayHLayout->addWidget(setDelayTipLabel);
    delayHLayout->addStretch(1);
    delayHLayout->addWidget(delaySpin);
    delayHLayout->addWidget(delayConfirm);
    QObject::connect(delayConfirm, &KPushButton::clicked, this, [=](){
        if (!managerModel->hasSelected()) return;
        this->showBusyState(true);
        cancel->setEnabled(false);
        delayConfirm->setEnabled(false);

        int delay = delaySpin->value();
        managerModel->setDelay(delay);

        this->showBusyState(false);
        cancel->setEnabled(true);
        delayConfirm->setEnabled(true);
        stateLabel->setText(tr("Pool: %1 Danmu: %2").arg(managerModel->totalPoolCount()).arg(managerModel->totalDanmuCount()));
    });


    QWidget *mainPage=new QWidget(this);
    KPushButton *importKdFile=new KPushButton(tr("Import"), mainPage);
    KPushButton *exportPool=new KPushButton(tr("Export Pool(s)"),mainPage);
    KPushButton *addDanmuPool=new KPushButton(tr("Add"), mainPage);
    KPushButton *deletePool=new KPushButton(tr("Delete Pool(s)"),mainPage);
    KPushButton *updatePool=new KPushButton(tr("Update Pool(s)"),mainPage);
    KPushButton *setDelay=new KPushButton(tr("Set Delay"),mainPage);
    QLineEdit *searchEdit=new ElaLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search"));
    searchEdit->setMinimumWidth(150);
    searchEdit->setClearButtonEnabled(true);
    QObject::connect(searchEdit, &QLineEdit::textChanged, this, [proxyModel](const QString &keyword){
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterRegularExpression(keyword);
    });
    QHBoxLayout *mainHLayout=new QHBoxLayout(mainPage);
    mainHLayout->setContentsMargins(0,0,0,0);
    mainHLayout->addWidget(importKdFile);
    mainHLayout->addWidget(exportPool);
    mainHLayout->addWidget(addDanmuPool);
    mainHLayout->addWidget(deletePool);
    mainHLayout->addWidget(updatePool);
    mainHLayout->addWidget(setDelay);
    mainHLayout->addStretch(1);
    mainHLayout->addWidget(searchEdit);

    QStackedLayout *funcStackLayout=new QStackedLayout();
    funcStackLayout->setContentsMargins(0,0,0,0);
    funcStackLayout->addWidget(mainPage);
    funcStackLayout->addWidget(exportPage);
    funcStackLayout->addWidget(deletePage);
    funcStackLayout->addWidget(updatePage);
    funcStackLayout->addWidget(setDelayPage);

    auto setControlEnable = [=](bool enable){
        poolView->setEnabled(enable);
        importKdFile->setEnabled(enable);
        exportPool->setEnabled(enable);
        deletePool->setEnabled(enable);
        updatePool->setEnabled(enable);
        addDanmuPool->setEnabled(enable);
        setDelay->setEnabled(enable);
    };

    QObject::connect(cancel, &KPushButton::clicked, this, [=](){
       funcStackLayout->setCurrentIndex(0);
        managerModel->setCheckable(false);
    });
    QObject::connect(importKdFile, &KPushButton::clicked, this, [=](){
        QStringList files = QFileDialog::getOpenFileNames(this,tr("Select KikoPlay Danmu Pool File"),"","KikoPlay Danmu Pool File(*.kd)");
        if(files.isEmpty()) return;
        this->showBusyState(true);
        importKdFile->setText(tr("Importing..."));
        setControlEnable(false);
        bool refreshList=false;
        for(auto &file:files)
        {
            if(GlobalObjects::danmuManager->importKdFile(file, this)>0)
                refreshList=true;
        }
        if(refreshList) managerModel->refreshList();
        this->showBusyState(false);
        importKdFile->setText(tr("Import"));
        setControlEnable(true);
    });
    QObject::connect(exportPool, &KPushButton::clicked, this, [=](){
        exportHLayout->addWidget(cancel);
        cancel->show();
        funcStackLayout->setCurrentIndex(1);
        managerModel->setCheckable(true);
    });
    QObject::connect(addDanmuPool, &KPushButton::clicked, this, [=](){
        AddPool addPool(this);
        if(QDialog::Accepted==addPool.exec())
        {
            EpInfo ep(addPool.epType, addPool.epIndex, addPool.ep);
            QString pid(GlobalObjects::danmuManager->createPool(addPool.anime, ep.type, ep.index, ep.name));
            managerModel->addPoolNode(addPool.anime, ep, pid);
        }
    });
    QObject::connect(deletePool, &KPushButton::clicked, this, [=](){
        deleteHLayout->addWidget(cancel);
        cancel->show();
        funcStackLayout->setCurrentIndex(2);
        managerModel->setCheckable(true);
    });
    QObject::connect(updatePool, &KPushButton::clicked, this, [=](){
        updateHLayout->addWidget(cancel);
        cancel->show();
        funcStackLayout->setCurrentIndex(3);
        managerModel->setCheckable(true);
    }); 
    QObject::connect(setDelay, &KPushButton::clicked, this, [=](){
        delayHLayout->addWidget(cancel);
        cancel->show();
        funcStackLayout->setCurrentIndex(4);
        managerModel->setCheckable(true);
    });

    QGridLayout *managerGLayout=new QGridLayout(this);
    auto margins = managerGLayout->contentsMargins();
    margins.setTop(0);
    managerGLayout->setContentsMargins(margins);
    managerGLayout->addLayout(funcStackLayout,0,0);
    managerGLayout->addWidget(poolView,1,0);
    managerGLayout->addWidget(stateLabel,2,0);
    managerGLayout->setRowStretch(1,1);
    managerGLayout->setColumnStretch(0,1);

    QHeaderView *poolHeader = poolView->header();
    poolHeader->setFont(this->font());
    poolHeader->resizeSection(0, 260); //Pool
    poolHeader->resizeSection(1, 150); //Source
    poolHeader->resizeSection(2, 50); //Delay
    poolHeader->resizeSection(3, 120); //Count

    setSizeSettingKey("DialogSize/PoolManager",QSize(620, 420));
    QVariant headerState(GlobalObjects::appSetting->value("HeaderViewState/PoolManagerView"));
    if(!headerState.isNull())
        poolView->header()->restoreState(headerState.toByteArray());
    addOnCloseCallback([poolView](){
        GlobalObjects::appSetting->setValue("HeaderViewState/PoolManagerView", poolView->header()->saveState());
    });

    QTimer::singleShot(0,[this,managerModel,importKdFile,addDanmuPool](){
        this->showBusyState(true);
        importKdFile->setEnabled(false);
        addDanmuPool->setEnabled(false);
        managerModel->refreshList();
        this->showBusyState(false);
        importKdFile->setEnabled(true);
        addDanmuPool->setEnabled(true);
    });
}

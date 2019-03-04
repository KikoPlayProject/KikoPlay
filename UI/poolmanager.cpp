#include "poolmanager.h"
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QCheckBox>
#include <QLabel>
#include <QAction>
#include <QSortFilterProxyModel>
#include "Play/Danmu/Manager/managermodel.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Playlist/playlistitem.h"
#include "timelineedit.h"
#include "adddanmu.h"
#include "globalobjects.h"

PoolManager::PoolManager(QWidget *parent) : CFramelessDialog(tr("Danmu Pool Manager"),parent)
{
    setFont(QFont("Microsoft Yahei UI",10));
    QTreeView *poolView=new QTreeView(this);
    poolView->setSelectionMode(QAbstractItemView::SingleSelection);
    poolView->setFont(this->font());
    poolView->setAlternatingRowColors(true);

    DanmuManagerModel *managerModel=new DanmuManagerModel(this);
    QSortFilterProxyModel *proxyModel=new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(managerModel);
    proxyModel->setFilterKeyColumn(0);
    proxyModel->setRecursiveFilteringEnabled(true);
    poolView->setModel(proxyModel);
    poolView->setSortingEnabled(true);
    poolView->setContextMenuPolicy(Qt::ActionsContextMenu);

    QLabel *stateLabel=new QLabel(this);
    stateLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    QObject::connect(GlobalObjects::danmuManager,&DanmuManager::workerStateMessage,stateLabel,[stateLabel,managerModel](const QString &msg){
       if(msg=="Done")
           stateLabel->setText(tr("Pool: %1 Danmu: %2").arg(managerModel->totalPoolCount()).arg(managerModel->totalDanmuCount()));
       else
           stateLabel->setText(msg);
    });

    QAction *act_editTimeLine=new QAction(tr("Edit TimeLine"),this);
    poolView->addAction(act_editTimeLine);
    QObject::connect(act_editTimeLine,&QAction::triggered,this,[this,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        DanmuPoolSourceNode *srcNode=managerModel->getSourceNode(proxyModel->mapToSource(indexList.first()));
        if(!srcNode)return;
        if(GlobalObjects::danmuPool->getPoolID()==srcNode->parent->idInfo)
        {
            DanmuSourceInfo *srcInfo=&GlobalObjects::danmuPool->getSources()[srcNode->srcId];
            TimelineEdit timeLineEdit(srcInfo,GlobalObjects::danmuPool->getSimpleDanmuInfo(srcInfo->id),this);
            if(QDialog::Accepted==timeLineEdit.exec())
            {
                GlobalObjects::danmuPool->refreshTimeLineDelayInfo(srcInfo);
            }
        }
        else
        {
            QList<SimpleDanmuInfo> simpleDanmuList;
            GlobalObjects::danmuManager->loadSimpleDanmuInfo(srcNode->parent->idInfo,srcNode->srcId,simpleDanmuList);
            DanmuSourceInfo srcInfo(srcNode->toSourceInfo());
            TimelineEdit timeLineEdit(&srcInfo,simpleDanmuList,this);
            if(QDialog::Accepted==timeLineEdit.exec())
            {
                srcNode->setTimeline(srcInfo);
                GlobalObjects::danmuManager->updateSourceTimeline(srcNode);
            }
        }
    });
    QAction *act_addWebSource=new QAction(tr("Add Web Source"),this);
    poolView->addAction(act_addWebSource);
    QObject::connect(act_addWebSource,&QAction::triggered,this,[this,stateLabel,managerModel,poolView,proxyModel](){
        QModelIndexList indexList = poolView->selectionModel()->selectedRows();
        if(indexList.size()==0)return;
        DanmuPoolNode *poolNode=managerModel->getPoolNode(proxyModel->mapToSource(indexList.first()));
        if(!poolNode)return;
        PlayListItem item;
        item.title=poolNode->title;
        item.animeTitle=poolNode->parent->title;
        AddDanmu addDanmuDialog(&item, this,false);
        if(QDialog::Accepted==addDanmuDialog.exec())
        {
            poolView->setEnabled(false);
            this->showBusyState(true);
            if(GlobalObjects::danmuPool->getPoolID()==poolNode->idInfo)
            {
                for(auto iter=addDanmuDialog.selectedDanmuList.begin();iter!=addDanmuDialog.selectedDanmuList.end();++iter)
                {
                    GlobalObjects::danmuPool->addDanmu((*iter).first,(*iter).second,false);
                }
                GlobalObjects::danmuPool->resetModel();
            }
            else
            {
                for(auto iter=addDanmuDialog.selectedDanmuList.begin();iter!=addDanmuDialog.selectedDanmuList.end();++iter)
                {
                    DanmuPoolSourceNode *srcNode = GlobalObjects::danmuManager->addSource(poolNode,&(*iter).first,&(*iter).second);
                    qDeleteAll((*iter).second);
                    managerModel->addSrcNode(poolNode,srcNode);
                }
                stateLabel->setText(tr("Pool: %1 Danmu: %2").arg(managerModel->totalPoolCount()).arg(managerModel->totalDanmuCount()));
            }
            poolView->setEnabled(true);
            this->showBusyState(false);
        }
    });
    QPushButton *cancel=new QPushButton(tr("Cancel"),this);
    cancel->hide();

    QWidget *exportPage=new QWidget(this);
    QHBoxLayout *exportHLayout=new QHBoxLayout(exportPage);
    exportHLayout->setContentsMargins(0,0,0,0);
    QCheckBox *useTimelineCheck=new QCheckBox(tr("Apply delay and timeline info"), exportPage);
    useTimelineCheck->setChecked(true);
    QCheckBox *useBlockRule=new QCheckBox(tr("Apply block rules"),exportPage);
    QPushButton *exportConfirm=new QPushButton(tr("Export"),exportPage);
    exportHLayout->addWidget(useTimelineCheck);
    exportHLayout->addWidget(useBlockRule);
    exportHLayout->addStretch(1);
    exportHLayout->addWidget(exportConfirm);
    QObject::connect(exportConfirm,&QPushButton::clicked,[this,poolView,managerModel,exportConfirm,cancel,useTimelineCheck,useBlockRule](){
        if(!managerModel->hasSelected())return;
        QString directory = QFileDialog::getExistingDirectory(this,
                                    tr("Select folder"), "",
                                    QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly);
        if (directory.isEmpty())return;
        this->showBusyState(true);
        exportConfirm->setText(tr("Exporting..."));
        exportConfirm->setEnabled(false);
        poolView->setEnabled(false);
        cancel->setEnabled(false);
        useTimelineCheck->setEnabled(false);
        useBlockRule->setEnabled(false);
        managerModel->exportPool(directory,useTimelineCheck->isChecked(),useBlockRule->isChecked());
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
    QPushButton *deleteConfirm=new QPushButton(tr("Delete"),deletePage);
    deleteHLayout->addWidget(deleteTipLabel);
    deleteHLayout->addStretch(1);
    deleteHLayout->addWidget(deleteConfirm);
    QObject::connect(deleteConfirm,&QPushButton::clicked,[this,stateLabel,poolView,managerModel,deleteConfirm,cancel](){
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
    QPushButton *updateConfirm=new QPushButton(tr("Update"),updatePage);
    updateHLayout->addWidget(updateTipLabel);
    updateHLayout->addStretch(1);
    updateHLayout->addWidget(updateConfirm);
    QObject::connect(updateConfirm,&QPushButton::clicked,[this,stateLabel,poolView,managerModel,updateConfirm,cancel](){
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

    QWidget *mainPage=new QWidget(this);
    QPushButton *exportPool=new QPushButton(tr("Export Pool(s)"),mainPage);
    QPushButton *deletePool=new QPushButton(tr("Delete Pool(s)"),mainPage);
    QPushButton *updatePool=new QPushButton(tr("Update Pool(s)"),mainPage);
    QLineEdit *searchEdit=new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search"));
    searchEdit->setMinimumWidth(150*logicalDpiX()/96);
    searchEdit->setClearButtonEnabled(true);
    QObject::connect(searchEdit,&QLineEdit::textChanged,[proxyModel](const QString &keyword){
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterRegExp(keyword);
    });
    QHBoxLayout *mainHLayout=new QHBoxLayout(mainPage);
    mainHLayout->setContentsMargins(0,0,0,0);
    mainHLayout->addWidget(exportPool);
    mainHLayout->addWidget(deletePool);
    mainHLayout->addWidget(updatePool);
    mainHLayout->addStretch(1);
    mainHLayout->addWidget(searchEdit);

    QStackedLayout *funcStackLayout=new QStackedLayout();
    funcStackLayout->setContentsMargins(0,0,0,0);
    funcStackLayout->addWidget(mainPage);
    funcStackLayout->addWidget(exportPage);
    funcStackLayout->addWidget(deletePage);
    funcStackLayout->addWidget(updatePage);

    QObject::connect(cancel,&QPushButton::clicked,[funcStackLayout](){
       funcStackLayout->setCurrentIndex(0);
    });
    QObject::connect(exportPool,&QPushButton::clicked,[cancel, exportHLayout,funcStackLayout](){
        exportHLayout->addWidget(cancel);
        cancel->show();
        funcStackLayout->setCurrentIndex(1);
    });
    QObject::connect(deletePool,&QPushButton::clicked,[cancel, deleteHLayout,funcStackLayout](){
        deleteHLayout->addWidget(cancel);
        cancel->show();
        funcStackLayout->setCurrentIndex(2);
    });
    QObject::connect(updatePool,&QPushButton::clicked,[cancel, updateHLayout,funcStackLayout](){
        updateHLayout->addWidget(cancel);
        cancel->show();
        funcStackLayout->setCurrentIndex(3);
    }); 

    QGridLayout *managerGLayout=new QGridLayout(this);
    managerGLayout->addLayout(funcStackLayout,0,0);
    managerGLayout->addWidget(poolView,1,0);
    managerGLayout->addWidget(stateLabel,2,0);
    managerGLayout->setRowStretch(1,1);
    managerGLayout->setColumnStretch(0,1);
    managerGLayout->setContentsMargins(0, 0, 0, 0);

    resize(620*logicalDpiX()/96, 420*logicalDpiY()/96);
    QHeaderView *poolHeader = poolView->header();
    poolHeader->setFont(this->font());
    poolHeader->resizeSection(0, 260*logicalDpiX()/96); //Pool
    poolHeader->resizeSection(1, 150*logicalDpiX()/96); //Source
    poolHeader->resizeSection(2, 50*logicalDpiX()/96); //Delay
    poolHeader->resizeSection(3, 120*logicalDpiX()/96); //Count


    QTimer::singleShot(0,[this,managerModel](){
        this->showBusyState(true);
        managerModel->refreshList();
        this->showBusyState(false);
    });
}

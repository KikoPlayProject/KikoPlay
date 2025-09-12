#include "blockeditor.h"
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include <QComboBox>
#include <QAction>
#include "Play/Danmu/Manager/pool.h"
#include "Play/Danmu/danmupool.h"
#include "UI/dialogs/danmuview.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "Play/Danmu/blocker.h"
#include "Common/notifier.h"

BlockEditor::BlockEditor(QWidget *parent) : CFramelessDialog(tr("Block Rules"), parent)
{
    BlockProxyModel *proxyModel = new BlockProxyModel(this);
    proxyModel->setSourceModel(GlobalObjects::blocker);
    QTreeView *blockView=new QTreeView(this);
    blockView->setRootIsDecorated(false);
    blockView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	blockView->setItemDelegate(new ComboBoxDelegate(this));
    blockView->setFont(QFont(GlobalObjects::normalFont, 11));
    blockView->setModel(proxyModel);
    blockView->setAlternatingRowColors(true);
    blockView->setContextMenuPolicy(Qt::CustomContextMenu);
    blockView->hideColumn(3);


    ElaMenu *actionMenu = new ElaMenu(blockView);
    QObject::connect(blockView, &QTreeView::customContextMenuRequested, this, [=](){
        if (!blockView->selectionModel()->hasSelection()) return;
        actionMenu->exec(QCursor::pos());
    });

    QAction *actViewBlocked = actionMenu->addAction(tr("View the blocked comments"));
    QObject::connect(actViewBlocked, &QAction::triggered, this, [=](){
        QModelIndexList selectedRows = blockView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        QModelIndex srcIndex = proxyModel->mapToSource(selectedRows.last()).siblingAtColumn((int)Blocker::Columns::ID);
        int ruleId = GlobalObjects::blocker->data(srcIndex, Blocker::BlockRuleIdRole).toInt();
        QList<QSharedPointer<DanmuComment>> blockedComments;
        auto &allComments = GlobalObjects::danmuPool->getPool()->comments();
        std::copy_if(allComments.begin(), allComments.end(), std::back_inserter(blockedComments),
                     [=](const QSharedPointer<DanmuComment> &c) { return c->blockBy == ruleId; });
        DanmuView view(&blockedComments, this);
        view.exec();
    });

    QAction *actRemove = actionMenu->addAction(tr("Remove Rule(s)"));
    QObject::connect(actRemove, &QAction::triggered, this, [=](){
        QModelIndexList selectedRows= blockView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        QModelIndexList out;
        std::transform(selectedRows.begin(), selectedRows.end(), std::back_inserter(out),
                       std::bind(&BlockProxyModel::mapToSource, proxyModel, std::placeholders::_1));
        GlobalObjects::blocker->removeBlockRule(out);
    });

    QPushButton *add = new KPushButton(tr("Add Rule"), this);
    QPushButton *remove=new KPushButton(tr("Remove Rule(s)"),this);
    QPushButton *importRule=new KPushButton(tr("Import"),this);
    QPushButton *exportRule=new KPushButton(tr("Export"),this);
    QComboBox *ruleTypeCombo = new ElaComboBox(this);

    QGridLayout *blockGLayout=new QGridLayout(this);
    blockGLayout->addWidget(add,0,0);
    blockGLayout->addWidget(remove,0,1);
    blockGLayout->addWidget(importRule,0,2);
    blockGLayout->addWidget(exportRule,0,3);
    blockGLayout->addWidget(ruleTypeCombo,0,5);
    blockGLayout->addWidget(blockView,1,0,1,6);
    blockGLayout->setRowStretch(1,1);
    blockGLayout->setColumnStretch(4,1);
    QHeaderView *blockHeader = blockView->header();
    blockHeader->setFont(this->font());
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::ID), 140);
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::BLOCKED), 80);
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::ENABLE), 60);
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::RELATION), 80);
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::REGEXP), 100);
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::PREFILTER), 80);
    setSizeSettingKey("DialogSize/BlockEditor",QSize(700, 320));


    ruleTypeCombo->addItems(GlobalObjects::blocker->fields);
    QObject::connect(ruleTypeCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[=](int index){
         proxyModel->setField(BlockRule::Field(index));
    });
    ruleTypeCombo->setCurrentIndex(0);

    QObject::connect(add,&QPushButton::clicked,GlobalObjects::blocker, [=](){
        GlobalObjects::blocker->addBlockRule(BlockRule::Field(ruleTypeCombo->currentIndex()));
        blockView->scrollToBottom();
    });

    QObject::connect(remove, &QPushButton::clicked, actRemove, &QAction::trigger);

    QObject::connect(importRule,&QPushButton::clicked,[this](){
        QString fileName = QFileDialog::getOpenFileName(this,tr("Select KikoPlay Block Rule File"),"","Block Rule(*.kbr);;Xml Rule(*.xml)");
        if(!fileName.isEmpty())
        {
            auto func = fileName.endsWith(".kbr")? &Blocker::importRules : &Blocker::importXmlRules;
            int c = (GlobalObjects::blocker->*func)(fileName);
            if(c < 0) showMessage(tr("Import Failed: File Error"), NM_ERROR | NM_HIDE);
            else showMessage(tr("Import %1 rule(s)").arg(c));
        }
    });

    QObject::connect(exportRule,&QPushButton::clicked,[this](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Exopot Block Rules"),"","Block Rule(*.kbr);;Xml Rule(*.xml)");
        if(!fileName.isEmpty())
        {
            auto func = fileName.endsWith(".kbr")? &Blocker::exportRules : &Blocker::exportXmlRules;
            if(!(GlobalObjects::blocker->*func)(fileName))
                showMessage(tr("Export Failed: File Error"));
        }
    });



}

void BlockEditor::onClose()
{
    GlobalObjects::blocker->save();
    CFramelessDialog::onClose();
}


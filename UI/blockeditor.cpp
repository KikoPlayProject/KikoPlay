#include "blockeditor.h"
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include <QComboBox>
#include <QAction>
#include "globalobjects.h"
#include "Play/Danmu/blocker.h"
#include "Common/notifier.h"

BlockEditor::BlockEditor(QWidget *parent) : CFramelessDialog(tr("Block Rules"),parent)
{
    BlockProxyModel *proxyModel = new BlockProxyModel(this);
    proxyModel->setSourceModel(GlobalObjects::blocker);
    setFont(QFont(GlobalObjects::normalFont,10));
    QTreeView *blockView=new QTreeView(this);
    blockView->setRootIsDecorated(false);
    blockView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	blockView->setItemDelegate(new ComboBoxDelegate(this));
    blockView->setFont(this->font());
    blockView->setModel(proxyModel);
    blockView->setAlternatingRowColors(true);
    blockView->setContextMenuPolicy(Qt::ActionsContextMenu);
    blockView->hideColumn(3);
    QPushButton *add=new QPushButton(tr("Add Rule"),this);

    QAction *actRemove = new QAction(tr("Remove Rule(s)"),this);
    QObject::connect(actRemove, &QAction::triggered, this, [=](){
        QModelIndexList selectedRows= blockView->selectionModel()->selectedRows();
        if (selectedRows.size() == 0)return;
        QModelIndexList out;
        std::transform(selectedRows.begin(), selectedRows.end(), std::back_inserter(out),
                       std::bind(&BlockProxyModel::mapToSource, proxyModel, std::placeholders::_1));
        GlobalObjects::blocker->removeBlockRule(out);
    });
    blockView->addAction(actRemove);

    QPushButton *remove=new QPushButton(tr("Remove Rule(s)"),this);
    QObject::connect(remove, &QPushButton::clicked, actRemove, &QAction::trigger);
    QPushButton *importRule=new QPushButton(tr("Import"),this);
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
    QPushButton *exportRule=new QPushButton(tr("Export"),this);
    QObject::connect(exportRule,&QPushButton::clicked,[this](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Exopot Block Rules"),"","Block Rule(*.kbr);;Xml Rule(*.xml)");
        if(!fileName.isEmpty())
        {
            auto func = fileName.endsWith(".kbr")? &Blocker::exportRules : &Blocker::exportXmlRules;
            if(!(GlobalObjects::blocker->*func)(fileName))
                showMessage(tr("Export Failed: File Error"));
        }
    });

    QComboBox *ruleTypeCombo = new QComboBox(this);
    ruleTypeCombo->addItems(GlobalObjects::blocker->fields);
    QObject::connect(ruleTypeCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[=](int index){
         proxyModel->setField(BlockRule::Field(index));
    });
    ruleTypeCombo->setCurrentIndex(0);

    QObject::connect(add,&QPushButton::clicked,GlobalObjects::blocker, [=](){
        GlobalObjects::blocker->addBlockRule(BlockRule::Field(ruleTypeCombo->currentIndex()));
        blockView->scrollToBottom();
    });

    QGridLayout *blockGLayout=new QGridLayout(this);
    blockGLayout->addWidget(add,0,0);
    blockGLayout->addWidget(remove,0,1);
    blockGLayout->addWidget(importRule,0,2);
    blockGLayout->addWidget(exportRule,0,3);
    blockGLayout->addWidget(ruleTypeCombo,0,5);
    blockGLayout->addWidget(blockView,1,0,1,6);
    blockGLayout->setRowStretch(1,1);
    blockGLayout->setColumnStretch(4,1);
	blockGLayout->setContentsMargins(0, 0, 0, 0);
    QHeaderView *blockHeader = blockView->header();
    blockHeader->setFont(this->font());
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::ID), 140*logicalDpiX()/96); //ID
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::BLOCKED), 80*logicalDpiX()/96); //BLOCKED
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::ENABLE), 60*logicalDpiX()/96); //Enable
    //blockHeader->resizeSection(2, 80*logicalDpiX()/96); //Field
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::RELATION), 80*logicalDpiX()/96); //Relation
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::REGEXP), 100*logicalDpiX()/96); //RegExp
    blockHeader->resizeSection(static_cast<int>(Blocker::Columns::PREFILTER), 80*logicalDpiX()/96); //UsePreFilter
    setSizeSettingKey("DialogSize/BlockEditor",QSize(700*logicalDpiX()/96,320*logicalDpiY()/96));

}

void BlockEditor::onClose()
{
    GlobalObjects::blocker->save();
    CFramelessDialog::onClose();
}


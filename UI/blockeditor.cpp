#include "blockeditor.h"
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include "globalobjects.h"
#include "Play/Danmu/blocker.h"

BlockEditor::BlockEditor(QWidget *parent) : CFramelessDialog(tr("Block Rules"),parent)
{
    setFont(QFont("Microsoft Yahei UI",10));
    QTreeView *blockView=new QTreeView(this);
    blockView->setRootIsDecorated(false);
    blockView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	blockView->setItemDelegate(new ComboBoxDelegate(this));
    blockView->setFont(this->font());
	blockView->setModel(GlobalObjects::blocker);
    blockView->setAlternatingRowColors(true);
    QPushButton *add=new QPushButton(tr("Add Rule"),this);
    QObject::connect(add,SIGNAL(clicked()),GlobalObjects::blocker,SLOT(addBlockRule()));
    QPushButton *remove=new QPushButton(tr("Remove Rule(s)"),this);
    QObject::connect(remove,&QPushButton::clicked,[blockView](){
        auto selection = blockView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        GlobalObjects::blocker->removeBlockRule(selection);
    });
    QPushButton *importRule=new QPushButton(tr("Import"),this);
    QObject::connect(importRule,&QPushButton::clicked,[this](){
        QString fileName = QFileDialog::getOpenFileName(this,tr("Select KikoPlay Block Rule File"),"","Block Rule(*.kbr)");
        if(!fileName.isEmpty())
        {
            if(GlobalObjects::blocker->importRules(fileName)<0)
                showMessage(tr("Import Failed: File Error"));
        }
    });
    QPushButton *exportRule=new QPushButton(tr("Export"),this);
    QObject::connect(exportRule,&QPushButton::clicked,[this](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Exopot Block Rules"),"","Block Rule(*.kbr)");
        if(!fileName.isEmpty())
        {
            if(GlobalObjects::blocker->exportRules(fileName)<0)
                showMessage(tr("Export Failed: File Error"));
        }
    });

    QGridLayout *blockGLayout=new QGridLayout(this);
    blockGLayout->addWidget(add,0,0);
    blockGLayout->addWidget(remove,0,1);
    blockGLayout->addWidget(importRule,0,2);
    blockGLayout->addWidget(exportRule,0,3);
    blockGLayout->addWidget(blockView,1,0,1,5);
    blockGLayout->setRowStretch(1,1);
    blockGLayout->setColumnStretch(4,1);
	blockGLayout->setContentsMargins(0, 0, 0, 0);
    resize(700*logicalDpiX()/96, 320*logicalDpiY()/96);
    QHeaderView *blockHeader = blockView->header();
    blockHeader->setFont(this->font());
    blockHeader->resizeSection(0, 110*logicalDpiX()/96); //ID
    blockHeader->resizeSection(1, 60*logicalDpiX()/96); //Enable
    blockHeader->resizeSection(2, 80*logicalDpiX()/96); //Field
    blockHeader->resizeSection(3, 90*logicalDpiX()/96); //Relation
    blockHeader->resizeSection(4, 100*logicalDpiX()/96); //RegExp
    blockHeader->resizeSection(5, 80*logicalDpiX()/96); //UsePreFilter
}

BlockEditor::~BlockEditor()
{
    GlobalObjects::blocker->save();
}

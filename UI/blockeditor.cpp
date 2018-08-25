#include "blockeditor.h"
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QGridLayout>
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
    QObject::connect(remove,&QPushButton::clicked,[this,blockView](){
        auto selection = blockView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        GlobalObjects::blocker->removeBlockRule(selection);
    });
    QGridLayout *blockGLayout=new QGridLayout(this);
    blockGLayout->addWidget(add,0,0);
    blockGLayout->addWidget(remove,0,1);
    blockGLayout->addWidget(blockView,1,0,1,3);
    blockGLayout->setRowStretch(1,1);
    blockGLayout->setColumnStretch(2,1);
	blockGLayout->setContentsMargins(0, 0, 0, 0);
    resize(620*logicalDpiX()/96, 320*logicalDpiY()/96);
	QHeaderView *blockHeader = blockView->header();
	blockHeader->setFont(this->font());
    blockHeader->resizeSection(0, 40*logicalDpiX()/96); //ID
    blockHeader->resizeSection(1, 60*logicalDpiX()/96); //Enable
    blockHeader->resizeSection(2, 80*logicalDpiX()/96); //Field
    blockHeader->resizeSection(3, 100*logicalDpiX()/96); //Relation
    blockHeader->resizeSection(4, 100*logicalDpiX()/96); //RegExp
    blockHeader->resizeSection(5, 200*logicalDpiX()/96); //Content
}

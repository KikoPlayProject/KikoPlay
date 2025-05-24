#include "selectepisodedialog.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "UI/widgets/kpushbutton.h"
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QAction>
#include <QHeaderView>
SelectEpisode::SelectEpisode(QList<DanmuSource> &epResults, QWidget *parent)
    :CFramelessDialog(tr("Select Episode"),parent,true), episodeResult(epResults)
{
    episodeWidget = new PressTreeWidget(this);
    episodeWidget->setItemDelegate(new KTreeviewItemDelegate(episodeWidget));
    episodeWidget->setRootIsDecorated(false);
    episodeWidget->setFont(font());
    episodeWidget->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    episodeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    episodeWidget->setHeaderLabels(QStringList()<<tr("Title")<<tr("Duration")<<tr("Delay(s)"));
    QObject::connect(episodeWidget,&QTreeWidget::itemChanged,[this](QTreeWidgetItem *item, int column){
        int index= episodeWidget->indexOfTopLevelItem(item);
        if(column==2)
        {
            QVariant data=item->data(column,0);
            if(data.canConvert(QMetaType::Int))
                this->episodeResult[index].delay=data.toInt()*1000;
        }
    });
    QObject::connect(episodeWidget,&QTreeWidget::itemDoubleClicked,[](QTreeWidgetItem *item, int column){
        if(column==2)
            item->setFlags(item->flags()|Qt::ItemIsEditable);
        else
            item->setFlags(item->flags()&~(Qt::ItemIsEditable));
    });

    QCheckBox *selectAllCheck = new ElaCheckBox(tr("Select All"), this);
    QObject::connect(selectAllCheck, &QCheckBox::stateChanged, [this](int state) {
		int count = episodeWidget->topLevelItemCount();
		for (int i = count - 1; i >= 0; i--)
		{
            episodeWidget->topLevelItem(i)->setCheckState(0, Qt::CheckState(state));
		}
	});
    QPushButton *setDelayButton = new KPushButton(tr("Auto Set Delay"),this);
    QObject::connect(setDelayButton,&QPushButton::clicked,[this](){
        int timeSum=0;
        int i=0;
        for(auto &item:episodeResult)
        {
            item.delay=timeSum;
            timeSum+=item.duration*1000;  //ms
            episodeWidget->topLevelItem(i++)->setData(2,0,QString::number(item.delay/1000));
        }
    });
    QPushButton *resetDelay = new KPushButton(tr("Reset Delay"),this);
    QObject::connect(resetDelay,&QPushButton::clicked,[this](){
        int i=0;
        for(auto &item:episodeResult)
        {
            item.delay=0;
            episodeWidget->topLevelItem(i++)->setData(2,0,QString("0"));
        }
    });

    QAction *checkSelection = new QAction(tr("Check Selection Episodes"), this);
    episodeWidget->addAction(checkSelection);
    QObject::connect(checkSelection, &QAction::triggered, this, [=](){
        auto selectedRows = episodeWidget->selectionModel()->selectedRows();
        for (auto &r : selectedRows)
        {
            episodeWidget->topLevelItem(r.row())->setCheckState(0, Qt::CheckState::Checked);
        }
    });

    QAction *actDelay = new QAction(tr("Auto Set Delay from this Episode"), this);
    episodeWidget->addAction(actDelay);
    QObject::connect(actDelay, &QAction::triggered, this, [this](){
        if(episodeWidget->selectedItems().isEmpty()) return;
        int pos = episodeWidget->indexOfTopLevelItem(episodeWidget->selectedItems().first());
        int timeSum=0;
        for(++pos; pos < episodeWidget->topLevelItemCount(); ++pos)
        {
            episodeResult[pos].delay = timeSum;
            timeSum += episodeResult[pos].duration*1000;  //ms
            episodeWidget->topLevelItem(pos)->setData(2,0,QString::number(timeSum/1000));
        }
    });

    QGridLayout *episodeGLayout=new QGridLayout(this);
    episodeGLayout->addWidget(selectAllCheck,0,0);
    episodeGLayout->addWidget(setDelayButton,0,1);
    episodeGLayout->addWidget(resetDelay,0,2);
    episodeGLayout->addWidget(episodeWidget,1,0,1,3);
    episodeGLayout->setColumnStretch(0,1);
    episodeGLayout->setRowStretch(1,1);

    for(auto &item:episodeResult)
    {
        QTreeWidgetItem *widgetItem=new QTreeWidgetItem(episodeWidget,QStringList()<<item.title<<item.durationStr()<<"0");
        widgetItem->setCheckState(0,Qt::Unchecked);
		item.delay = 0;
    }
	resize(600, 500);
	QHeaderView *episodeHeader = episodeWidget->header();
	episodeHeader->resizeSection(0, 360);
	episodeHeader->resizeSection(1, 90);
	episodeHeader->resizeSection(2, 70);
    episodeHeader->setFont(this->font());
}

void SelectEpisode::onAccept()
{
    int count=episodeWidget->topLevelItemCount();
    for(int i=count-1;i>=0;i--)
    {
        if(episodeWidget->topLevelItem(i)->checkState(0)!=Qt::Checked)
            episodeResult.removeAt(i);
    }
    CFramelessDialog::onAccept();
}

void PressTreeWidget::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key == Qt::Key_Enter || key == Qt::Key_Return)
    {
        QTreeWidgetItem *item = currentItem();
        if (item)
        {
            item->setCheckState(0, item->checkState(0) == Qt::Checked? Qt::Unchecked : Qt::Checked);
        }
    }
    QTreeWidget::keyPressEvent(event);
}

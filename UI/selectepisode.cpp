#include "selectepisode.h"
#include <QTreeWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QHeaderView>
SelectEpisode::SelectEpisode(DanmuAccessResult *episodeResult,QWidget *parent):CFramelessDialog(tr("Select Episode"),parent,true),autoSetDelay(false)
{
    this->episodeResult=episodeResult;
    //setFont(QFont("Microsoft Yahei UI",10));
    episodeWidget=new QTreeWidget(this);
    episodeWidget->setRootIsDecorated(false);
    episodeWidget->setFont(font());
    episodeWidget->setHeaderLabels(QStringList()<<tr("Title")<<tr("Duration")<<tr("Delay(s)"));
    QObject::connect(episodeWidget,&QTreeWidget::itemChanged,[this](QTreeWidgetItem *item, int column){
        if(autoSetDelay)return;
        int index= episodeWidget->indexOfTopLevelItem(item);
        if(column==2)
        {
            QVariant data=item->data(column,0);
            if(data.canConvert(QMetaType::Int))
                this->episodeResult->list[index].delay=data.toInt();
        }
    });
    QObject::connect(episodeWidget,&QTreeWidget::itemDoubleClicked,[this](QTreeWidgetItem *item, int column){
        if(column==2)
            item->setFlags(item->flags()|Qt::ItemIsEditable);
        else
            item->setFlags(item->flags()&~(Qt::ItemIsEditable));
    });
	QCheckBox *selectAllCheck = new QCheckBox(tr("Select All"), this);
    QObject::connect(selectAllCheck, &QCheckBox::stateChanged, [this](int state) {
		int count = episodeWidget->topLevelItemCount();
		for (int i = count - 1; i >= 0; i--)
		{
            episodeWidget->topLevelItem(i)->setCheckState(0, Qt::CheckState(state));
		}
	});
    QPushButton *setDelayButton=new QPushButton(tr("Auto Set Delay"),this);
    QObject::connect(setDelayButton,&QPushButton::clicked,[this](){
        int timeSum=0;
        int i=0;
        autoSetDelay=true;
        for(DanmuSourceItem &item:this->episodeResult->list)
        {
            item.delay=timeSum;
            timeSum+=item.extra;
            episodeWidget->topLevelItem(i++)->setData(2,0,QString::number(item.delay));
        }
        autoSetDelay=false;
    });
    QPushButton *resetDelay=new QPushButton(tr("Reset Delay"),this);
    QObject::connect(resetDelay,&QPushButton::clicked,[this](){
        autoSetDelay=true;
        int i=0;
        for(DanmuSourceItem &item:this->episodeResult->list)
        {
            item.delay=0;
            episodeWidget->topLevelItem(i++)->setData(2,0,QString("0"));
        }
        autoSetDelay=false;
    });
    QGridLayout *episodeGLayout=new QGridLayout(this);
    episodeGLayout->addWidget(selectAllCheck,0,0);
    episodeGLayout->addWidget(setDelayButton,0,1);
    episodeGLayout->addWidget(resetDelay,0,2);
    episodeGLayout->addWidget(episodeWidget,1,0,1,3);
    episodeGLayout->setColumnStretch(0,1);
    episodeGLayout->setRowStretch(1,1);

    for(DanmuSourceItem &item:episodeResult->list)
    {
        int min=item.extra/60;
        int sec=item.extra-min*60;
        QString duration=QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
        QTreeWidgetItem *widgetItem=new QTreeWidgetItem(episodeWidget,QStringList()<<item.title<<duration<<"0");
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
            episodeResult->list.removeAt(i);
    }
    CFramelessDialog::onAccept();
}

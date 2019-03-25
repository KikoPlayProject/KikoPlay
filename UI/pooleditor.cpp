#include "pooleditor.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileDialog>
#include <QAction>
#include "globalobjects.h"
#include "Play/Danmu/providermanager.h"
#include "Play/Video/mpvplayer.h"
#include "timelineedit.h"
namespace
{
    QList<QPair<int,int> > timelineClipBoard;
}
PoolEditor::PoolEditor(QWidget *parent) : CFramelessDialog(tr("Edit Pool"),parent)
{
    QWidget *contentWidget=new QWidget(this);
    QHBoxLayout *cHLayout=new QHBoxLayout(this);
    cHLayout->setContentsMargins(0,0,0,0);
    contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QScrollArea *contentScrollArea=new QScrollArea(this);
    contentScrollArea->setWidget(contentWidget);
    contentScrollArea->setWidgetResizable(true);
    contentScrollArea->setAlignment(Qt::AlignCenter);
    cHLayout->addWidget(contentScrollArea);


    QVBoxLayout *poolItemVLayout=new QVBoxLayout(contentWidget);
    QHash<int,DanmuSourceInfo> &sources=GlobalObjects::danmuPool->getSources();
    for(auto iter=sources.begin();iter!=sources.end();++iter)
    {
        PoolItem *poolItem=new PoolItem(&iter.value(),this);
        poolItemVLayout->addWidget(poolItem);
    }
    poolItemVLayout->addItem(new QSpacerItem(1, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));

    addIgnoreWidget(contentWidget);
    setMinimumSize(300*logicalDpiX()/96,120*logicalDpiY()/96);
    resize(GlobalObjects::appSetting->value("DialogSize/PoolEdit",QSize(600*logicalDpiX()/96,320*logicalDpiY()/96)).toSize());
}

void PoolEditor::onClose()
{
     GlobalObjects::appSetting->setValue("DialogSize/PoolEdit",size());
     CFramelessDialog::onClose();
}

PoolItem::PoolItem(DanmuSourceInfo *sourceInfo, QWidget *parent):QFrame(parent),source(sourceInfo)
{
    QAction *copyTimeline=new QAction(tr("Copy TimeLine Info"), this);
    QObject::connect(copyTimeline,&QAction::triggered,this,[sourceInfo](){
        timelineClipBoard=sourceInfo->timelineInfo;
    });
    QAction *pasteTimeline=new QAction(tr("Paste TimeLine Info"), this);
    QObject::connect(pasteTimeline,&QAction::triggered,this,[sourceInfo](){
        if(timelineClipBoard.isEmpty()) return;
        for(auto &pair:timelineClipBoard)
        {
            int i=0;
            int c=sourceInfo->timelineInfo.count();
            while(i<c && sourceInfo->timelineInfo.at(i).first<pair.first) i++;
            if(i<c && sourceInfo->timelineInfo.at(i).first==pair.first) continue;
            sourceInfo->timelineInfo.insert(i,pair);
        }
        GlobalObjects::danmuPool->refreshTimeLineDelayInfo(sourceInfo);
    });
    setContextMenuPolicy(Qt::ActionsContextMenu);
    addAction(copyTimeline);
    addAction(pasteTimeline);

    QFont normalFont("Microsoft YaHei",16);
    QLabel *name=new QLabel(QString("%1(%2)").arg(source->name).arg(source->count),this);
    name->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    name->setFont(normalFont);

    QCheckBox *itemSwitch=new QCheckBox(tr("Show"),this);
    itemSwitch->setChecked(sourceInfo->show);
    QObject::connect(itemSwitch,&QCheckBox::stateChanged,[this](int state){
       if(state==Qt::Unchecked)
           source->show=false;
       else
           source->show=true;
    });
    QHBoxLayout *itemControlHLayout1=new QHBoxLayout();
    itemControlHLayout1->addWidget(name);
    itemControlHLayout1->addStretch(1);
    itemControlHLayout1->addWidget(itemSwitch);

    QLabel *url=new QLabel(tr("Source: <a href = %1>%1</a>").arg(source->url),this);
    url->setOpenExternalLinks(true);
    url->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);

    QLabel *delayLabel=new QLabel(tr("Delay(s): "),this);
    QSpinBox *delaySpinBox=new QSpinBox(this);
	delaySpinBox->setRange(INT_MIN,INT_MAX);
    delaySpinBox->setValue(source->delay/1000);
    delaySpinBox->setObjectName(QStringLiteral("Delay"));
    delaySpinBox->setAlignment(Qt::AlignCenter);
    delaySpinBox->setFixedWidth(80*logicalDpiX()/96);
    QObject::connect(delaySpinBox,&QSpinBox::editingFinished,[delaySpinBox,sourceInfo](){
       GlobalObjects::danmuPool->setDelay(sourceInfo,delaySpinBox->value()*1000);
    });
    QPushButton *editTimeline=new QPushButton(tr("Edit Timeline"),this);
    editTimeline->setFixedWidth(80*logicalDpiX()/96);
    QObject::connect(editTimeline,&QPushButton::clicked,[this](){
        int curTime=curTime=GlobalObjects::mpvplayer->getTime();
        TimelineEdit timelineEdit(source,GlobalObjects::danmuPool->getSimpleDanmuInfo(source->id),this,curTime);
        if(QDialog::Accepted==timelineEdit.exec())
        {
            GlobalObjects::danmuPool->refreshTimeLineDelayInfo(source);
        }
    });

    QPushButton *deleteButton=new QPushButton(tr("Delete"),this);
    //deleteButton->setFont(normalFont);
    deleteButton->setFixedWidth(80*logicalDpiX()/96);
    deleteButton->setObjectName(QStringLiteral("DialogButton"));
    QObject::connect(deleteButton,&QPushButton::clicked,[this,sourceInfo](){
       GlobalObjects::danmuPool->deleteSource(sourceInfo->id);
       this->deleteLater();
    });
    QPushButton *updateButton=new QPushButton(tr("Update"),this);
    updateButton->setFixedWidth(80*logicalDpiX()/96);
    updateButton->setObjectName(QStringLiteral("DialogButton"));
	QFileInfo fi(sourceInfo->url);
    if(fi.exists())
		updateButton->setEnabled(false);
    QObject::connect(updateButton,&QPushButton::clicked,[this,updateButton,deleteButton,
                     editTimeline,delaySpinBox,name](){
        QList<DanmuComment *> tmpList;
        updateButton->setEnabled(false);
        deleteButton->setEnabled(false);
        editTimeline->setEnabled(false);
        delaySpinBox->setEnabled(false);
        QString errInfo = GlobalObjects::providerManager->downloadBySourceURL(source->url,tmpList);
        if(errInfo.isEmpty())
        {
            if(tmpList.count()>0)
            {
                DanmuSourceInfo si;
                si.count = tmpList.count();
                si.url = source->url;
                int addCount = GlobalObjects::danmuPool->addDanmu(si,tmpList);
                if(addCount>0)
                {
                    QMessageBox::information(this,tr("Update - %1").arg(source->name),tr("Add %1 New Danmu").arg(addCount));
                    name->setText(QString("%1(%2)").arg(source->name).arg(source->count));
                }
            }
        }
        else
        {
            QMessageBox::information(this,tr("Error"),tr("Error:%1").arg(errInfo));
        }
        updateButton->setEnabled(true);
        deleteButton->setEnabled(true);
        editTimeline->setEnabled(true);
        delaySpinBox->setEnabled(true);
    });

    QPushButton *exportButton=new QPushButton(tr("Export"),this);
    exportButton->setFixedWidth(80*logicalDpiX()/96);
    exportButton->setObjectName(QStringLiteral("DialogButton"));
    QObject::connect(exportButton,&QPushButton::clicked,[this](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Danmu"),source->name,tr("Xml File (*.xml)"));
        if(!fileName.isEmpty())
        {
            GlobalObjects::danmuPool->exportDanmu(source->id,fileName);
        }
    });

    QHBoxLayout *itemControlHLayout2=new QHBoxLayout();
    itemControlHLayout2->addWidget(delayLabel);
    itemControlHLayout2->addSpacing(8*logicalDpiX()/96);
    itemControlHLayout2->addWidget(delaySpinBox);
    itemControlHLayout2->addSpacing(4*logicalDpiX()/96);
    itemControlHLayout2->addWidget(editTimeline);
    itemControlHLayout2->addStretch(1);
    itemControlHLayout2->addWidget(updateButton);
    itemControlHLayout2->addWidget(exportButton);
    itemControlHLayout2->addWidget(deleteButton);

    QVBoxLayout *itemVLayout=new QVBoxLayout(this);
    itemVLayout->setSpacing(2*logicalDpiY()/96);
    itemVLayout->addLayout(itemControlHLayout1);
    itemVLayout->addWidget(url);
    itemVLayout->addLayout(itemControlHLayout2);
    //setFixedHeight(140);
    //setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    setObjectName(QStringLiteral("PoolItem"));
}

#include "pooleditor.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileDialog>
#include"globalobjects.h"
#include "Play/Danmu/providermanager.h"
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

PoolItem::PoolItem(DanmuSourceInfo *sourceInfo, QWidget *parent):source(sourceInfo),QFrame(parent)
{
    QFont normalFont("Microsoft YaHei",16);
    QLabel *name=new QLabel(QString("%1(%2)").arg(source->name).arg(source->count),this);
    name->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    name->setFont(normalFont);

    QCheckBox *itemSwitch=new QCheckBox("Open",this);
    itemSwitch->setChecked(sourceInfo->open);
    QObject::connect(itemSwitch,&QCheckBox::stateChanged,[sourceInfo](int state){
       if(state==Qt::Unchecked)
           sourceInfo->open=false;
       else
           sourceInfo->open=true;
    });
    QHBoxLayout *itemControlHLayout1=new QHBoxLayout();
    itemControlHLayout1->addWidget(name);
    QSpacerItem *listHSpacer1 = new QSpacerItem(0, 1, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    itemControlHLayout1->addItem(listHSpacer1);
    itemControlHLayout1->addWidget(itemSwitch);

    QLabel *url=new QLabel(QString("Source: <a href = %1>%1</a>").arg(source->url),this);
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
    QObject::connect(updateButton,&QPushButton::clicked,[this,sourceInfo,updateButton, name](){
        QList<DanmuComment *> tmpList;
        updateButton->setEnabled(false);
        QString errInfo = GlobalObjects::providerManager->downloadBySourceURL(sourceInfo->url,tmpList);
        if(errInfo.isEmpty())
        {
            if(tmpList.count()>0)
            {
                QSet<QString> danmuHashSet(GlobalObjects::danmuPool->getDanmuHash(sourceInfo->id));
                for(auto iter=tmpList.begin();iter!=tmpList.end();)
                {
                    QByteArray hashData(QString("%0%1%2%3").arg((*iter)->text).arg((*iter)->date).arg((*iter)->sender).arg((*iter)->color).toUtf8());
                    QString danmuHash(QString(QCryptographicHash::hash(hashData,QCryptographicHash::Md5).toHex()));
                    if(danmuHashSet.contains(danmuHash))
                    {
                        delete *iter;
                        iter=tmpList.erase(iter);
                    }
                    else
                        iter++;
                }
            }
            if(tmpList.count()>0)
            {
                DanmuSourceInfo si;
                si.count = tmpList.count();
                si.url = sourceInfo->url;
                GlobalObjects::danmuPool->addDanmu(si,tmpList);
                QMessageBox::information(this,tr("Update"),tr("Add %1 New Danmu").arg(tmpList.count()));
                name->setText(QString("%1(%2)").arg(sourceInfo->name).arg(sourceInfo->count));
            }
        }
        else
        {
            QMessageBox::information(this,tr("Error"),tr("Error:%1").arg(errInfo));
        }
        updateButton->setEnabled(true);
    });

    QPushButton *exportButton=new QPushButton(tr("Export"),this);
    exportButton->setFixedWidth(80*logicalDpiX()/96);
    exportButton->setObjectName(QStringLiteral("DialogButton"));
    QObject::connect(exportButton,&QPushButton::clicked,[this,sourceInfo](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Danmu"),sourceInfo->name,tr("Xml File (*.xml)"));
        if(!fileName.isEmpty())
        {
            GlobalObjects::danmuPool->exportDanmu(sourceInfo->id,fileName);
        }
    });

    QHBoxLayout *itemControlHLayout2=new QHBoxLayout();
    itemControlHLayout2->addWidget(delayLabel);
    itemControlHLayout2->addSpacing(8*logicalDpiX()/96);
    itemControlHLayout2->addWidget(delaySpinBox);
    QSpacerItem *listHSpacer2 = new QSpacerItem(0, 1, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    itemControlHLayout2->addItem(listHSpacer2);
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

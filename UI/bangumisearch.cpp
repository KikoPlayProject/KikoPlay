#include "bangumisearch.h"
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTreeWidget>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include "MediaLibrary/Service/bangumi.h"
#include "MediaLibrary/animeinfo.h"
#include "globalobjects.h"
#include "MediaLibrary/animelibrary.h"

BangumiSearch::BangumiSearch(Anime *anime, QWidget *parent) : CFramelessDialog(tr("Bangumi Search"),parent,true,true,false),currentAnime(anime)
{
    searchWordEdit=new QLineEdit(anime->name,this);
    searchButton=new QPushButton(tr("Search"),this);
    QObject::connect(searchButton,&QPushButton::clicked,this,&BangumiSearch::search);
    QObject::connect(searchWordEdit,&QLineEdit::returnPressed,this,&BangumiSearch::search);

    QCheckBox *downloadTag=new QCheckBox(tr("Download Tag"),this);
    downloadTag->setCheckState((Qt::CheckState)GlobalObjects::appSetting->value("Library/DownloadTag",Qt::Checked).toInt());
    QObject::connect(downloadTag,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::appSetting->setValue("Library/DownloadTag",state);
    });

    QLabel *coverQuality=new QLabel(tr("Cover Quality"),this);
    qualityCombo=new QComboBox(this);
    qualityCombo->addItems(QStringList()<<tr("Medium")<<tr("Common")<<tr("Large"));
    qualityCombo->setCurrentIndex(GlobalObjects::appSetting->value("Library/CoverQuality",1).toInt());
    QObject::connect(qualityCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged,[](int index){
        GlobalObjects::appSetting->setValue("Library/CoverQuality", index);
    });

    bangumiList=new QTreeWidget(this);
    bangumiList->setRootIsDecorated(false);
    bangumiList->setFont(font());
    bangumiList->setHeaderLabels(QStringList()<<tr("Title")<<tr("Title-CN")<<tr("BangumiID"));
    bangumiList->setAlternatingRowColors(true);
    QHeaderView *bgmHeader = bangumiList->header();
    bgmHeader->resizeSection(0, 170*logicalDpiX()/96);
    bgmHeader->resizeSection(1, 170*logicalDpiX()/96);
    bgmHeader->resizeSection(2, 80*logicalDpiX()/96);
    bgmHeader->setFont(font());

    downloadInfoLabel=new QLabel(this);
    downloadInfoLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    downloadInfoLabel->hide();
    QObject::connect(GlobalObjects::library,&AnimeLibrary::downloadDetailMessage,this,[this](const QString &msg){
       downloadInfoLabel->setText(msg);
       if(downloadInfoLabel->isHidden()) downloadInfoLabel->show();
    });

    QGridLayout *bgmGLayout=new QGridLayout(this);
    bgmGLayout->addWidget(searchWordEdit,0,0,1,2);
    bgmGLayout->addWidget(searchButton,0,2);
    bgmGLayout->addWidget(downloadTag,1,0);
    bgmGLayout->addWidget(coverQuality,1,1);
    bgmGLayout->addWidget(qualityCombo,1,2);
    bgmGLayout->addWidget(bangumiList,2,0,1,3);
    bgmGLayout->addWidget(downloadInfoLabel,3,0,1,3);
    bgmGLayout->setRowStretch(1,1);
    bgmGLayout->setColumnStretch(0,1);

    resize(460*logicalDpiX()/96, 320*logicalDpiY()/96);
}

void BangumiSearch::search()
{
    QString keyword=searchWordEdit->text().trimmed();
    if(keyword.isEmpty())return;
    searchButton->setEnabled(false);
    searchWordEdit->setEnabled(false);
    showBusyState(true);
    QList<Bangumi::BangumiInfo> bgms;
    QString err(Bangumi::animeSearch(keyword, bgms));
    if(err.isEmpty())
    {
        bangumiList->clear();
        for(auto &bgm : bgms)
        {
            new QTreeWidgetItem(bangumiList, {bgm.name, bgm.name_cn, QString::number(bgm.bgmID)});
        }
    }
    else
    {
        showMessage(err, 1);
    }
    searchButton->setEnabled(true);
    searchWordEdit->setEnabled(true);
    showBusyState(false);
}

void BangumiSearch::onAccept()
{
    if(bangumiList->selectedItems().count()==0)
    {
        showMessage(tr("You need to choose a bangumi"),1);
        return;
    }
    else
    {
        showBusyState(true);
        searchButton->setEnabled(false);
        searchWordEdit->setEnabled(false);
        int bgmId=bangumiList->selectedItems().last()->text(2).toInt();
        qualityCombo->setEnabled(false);
        QString errInfo;
        currentAnime= GlobalObjects::library->downloadDetailInfo(currentAnime,bgmId,&errInfo);
        showBusyState(false);
        searchButton->setEnabled(true);
        searchWordEdit->setEnabled(true);
        qualityCombo->setEnabled(true);
        if(!errInfo.isEmpty())
        {
            showMessage(errInfo,1);
            return;
        }
        CFramelessDialog::onAccept();
    }
}

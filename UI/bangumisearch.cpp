#include "bangumisearch.h"
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTreeWidget>
#include <QMessageBox>
#include <QHeaderView>
#include <QLabel>
#include <QMovie>
#include "Common/network.h"
#include "MediaLibrary/animeinfo.h"
#include "globalobjects.h"
#include "MediaLibrary/animelibrary.h"

BangumiSearch::BangumiSearch(Anime *anime, QWidget *parent) : CFramelessDialog(tr("Bangumi Search"),parent,true),currentAnime(anime)
{
    searchWordEdit=new QLineEdit(anime->title,this);
    searchButton=new QPushButton(tr("Search"),this);
    QObject::connect(searchButton,&QPushButton::clicked,this,&BangumiSearch::search);
    QObject::connect(searchWordEdit,&QLineEdit::returnPressed,this,&BangumiSearch::search);

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

    QGridLayout *bgmGLayout=new QGridLayout(this);
    bgmGLayout->addWidget(searchWordEdit,0,0);
    bgmGLayout->addWidget(searchButton,0,1);
    bgmGLayout->addWidget(bangumiList,1,0,1,2);
    bgmGLayout->setRowStretch(1,1);
    bgmGLayout->setColumnStretch(0,1);

    resize(460*logicalDpiX()/96, 300*logicalDpiY()/96);
}

void BangumiSearch::search()
{
    QString keyword=searchWordEdit->text().trimmed();
    if(keyword.isEmpty())return;
    searchButton->setEnabled(false);
    searchWordEdit->setEnabled(false);

    QString baseUrl("https://api.bgm.tv/search/subject/"+keyword);
    QUrlQuery query;
    query.addQueryItem("type","2");
    query.addQueryItem("responseGroup","small");
    query.addQueryItem("start","0");
    query.addQueryItem("max_results","25");
    showBusyState(true);
    try
    {
        QJsonDocument document(Network::toJson(Network::httpGet(baseUrl,query,QStringList()<<"Accept"<<"application/json")));
        QJsonArray results=document.object().value("list").toArray();
        bangumiList->clear();
        for(auto iter=results.begin();iter!=results.end();++iter)
        {
            QJsonObject searchObj=(*iter).toObject();
            QTreeWidgetItem *widgetItem=new QTreeWidgetItem(bangumiList,QStringList()<<searchObj.value("name").toString()<<searchObj.value("name_cn").toString()<<QString::number(searchObj.value("id").toInt()));
        }
    }
    catch(Network::NetworkError &error)
    {
        QMessageBox::information(this,tr("Error"),error.errorInfo);
    }
    searchButton->setEnabled(true);
    searchWordEdit->setEnabled(true);
    showBusyState(false);
}

void BangumiSearch::onAccept()
{
    if(bangumiList->selectedItems().count()==0)
    {
        QMessageBox::information(this,tr("Tip"),tr("You need to choose a bangumi"));
        return;
    }
    else
    {
        showBusyState(true);
        searchButton->setEnabled(false);
        searchWordEdit->setEnabled(false);
        int bgmId=bangumiList->selectedItems().last()->text(2).toInt();
        QString errInfo;
        currentAnime= GlobalObjects::library->downloadDetailInfo(currentAnime,bgmId,&errInfo);
        if(!errInfo.isEmpty())
            QMessageBox::information(this,tr("Error"),errInfo);
        showBusyState(false);
        searchButton->setEnabled(true);
        searchWordEdit->setEnabled(true);
        CFramelessDialog::onAccept();
    }
}

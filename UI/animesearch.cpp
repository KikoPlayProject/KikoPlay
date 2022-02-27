#include "animesearch.h"
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTreeWidget>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include "globalobjects.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/animeprovider.h"
#include "Common/notifier.h"
#define AnimeRole Qt::UserRole+1

AnimeSearch::AnimeSearch(Anime *anime, QWidget *parent) : CFramelessDialog(tr("Bangumi Search"),parent,true,true,false)
{
    scriptCombo=new QComboBox(this);
    for(const auto &p : GlobalObjects::animeProvider->getSearchProviders())
    {
        scriptCombo->addItem(p.first, p.second);
    }
    int animeScriptIndex = scriptCombo->findData(anime->scriptId());
    if(animeScriptIndex != -1)
    {
        scriptCombo->setCurrentIndex(animeScriptIndex);
    }

    searchWordEdit=new QLineEdit(anime?anime->name():"",this);
    searchButton=new QPushButton(tr("Search"),this);
    QObject::connect(searchButton,&QPushButton::clicked,this,&AnimeSearch::search);

    bangumiList=new QTreeWidget(this);
    bangumiList->setRootIsDecorated(false);
    bangumiList->setFont(font());
    bangumiList->setHeaderLabels(QStringList()<<tr("Title")<<tr("Extra"));
    bangumiList->setAlternatingRowColors(true);
    QHeaderView *bgmHeader = bangumiList->header();
    bgmHeader->resizeSection(0, 180*logicalDpiX()/96);
    bgmHeader->resizeSection(1, 170*logicalDpiX()/96);
    bgmHeader->setFont(font());

    QGridLayout *bgmGLayout=new QGridLayout(this);
    bgmGLayout->setContentsMargins(0, 0, 0, 0);
    bgmGLayout->addWidget(scriptCombo, 0, 0);
    bgmGLayout->addWidget(searchWordEdit, 0, 1);
    bgmGLayout->addWidget(searchButton, 0, 2);
    bgmGLayout->addWidget(bangumiList,1,0,1,3);
    bgmGLayout->setRowStretch(1,1);
    bgmGLayout->setColumnStretch(1,1);

    setSizeSettingKey("DialogSize/AnimeSearch", QSize(460*logicalDpiX()/96, 320*logicalDpiY()/96));
}

void AnimeSearch::search()
{
    QString keyword=searchWordEdit->text().trimmed();
    if(keyword.isEmpty())return;
    if(scriptCombo->currentData().isNull()) return;
    searchButton->setEnabled(false);
    searchWordEdit->setEnabled(false);
    showBusyState(true);
    QList<AnimeLite> animes;
    ScriptState state = GlobalObjects::animeProvider->animeSearch(scriptCombo->currentData().toString(), keyword, animes);
    if(state)
    {
        bangumiList->clear();
        for(auto &anime : animes)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(bangumiList, {anime.name, anime.extras});
            item->setToolTip(0, anime.name);
            item->setToolTip(1, anime.extras);
            item->setData(0, AnimeRole, QVariant::fromValue(anime));
        }
    } else {
        showMessage(state.info, NM_ERROR | NM_HIDE);
    }
    searchButton->setEnabled(true);
    searchWordEdit->setEnabled(true);
    showBusyState(false);
}

void AnimeSearch::onAccept()
{
    if(bangumiList->selectedItems().count()==0)
    {
        showMessage(tr("You need to choose one"), NM_ERROR | NM_HIDE);
        return;
    }
    else
    {
        curSelectedAnime = bangumiList->selectedItems().last()->data(0, AnimeRole).value<AnimeLite>();
    }
    CFramelessDialog::onAccept();
}

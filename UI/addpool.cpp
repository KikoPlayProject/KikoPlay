#include "addpool.h"
#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QClipboard>
#include <QApplication>
#include <QAction>
#include <QTreeWidget>
#include <QButtonGroup>
#include "globalobjects.h"
#include "Common/kcache.h"
#include "Play/Danmu/Manager/danmumanager.h"
namespace
{
    QSet<QString> hitWords;
}
AddPool::AddPool(QWidget *parent, const QString &srcAnime, const QString &srcEp) : CFramelessDialog(tr("Add Danmu Pool"),parent,true)
{
    QVBoxLayout *addPoolVLayout=new QVBoxLayout(this);
    addPoolVLayout->setContentsMargins(0,0,0,0);
    addPoolVLayout->setSpacing(0);
    setFont(QFont(GlobalObjects::normalFont,12));
    QSize pageButtonSize(90 *logicalDpiX()/96,36*logicalDpiY()/96);

    searchPage=new QToolButton(this);
    searchPage->setText(tr("Search"));
    searchPage->setCheckable(true);
    searchPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    searchPage->setFixedSize(pageButtonSize);
    searchPage->setObjectName(QStringLiteral("DialogPageButton"));
    searchPage->setChecked(true);

    customPage=new QToolButton(this);
    customPage->setText(tr("Custom"));
    customPage->setCheckable(true);
    customPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    customPage->setFixedSize(pageButtonSize);
    customPage->setObjectName(QStringLiteral("DialogPageButton"));

    QButtonGroup *btnGroup=new QButtonGroup(this);
    btnGroup->addButton(searchPage,0);
    btnGroup->addButton(customPage,1);
    QObject::connect(btnGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[this](int id, bool checked){
        if(checked)
        {
            contentStackLayout->setCurrentIndex(id);
        }
    });

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    pageButtonHLayout->addWidget(searchPage);
    pageButtonHLayout->addWidget(customPage);
    pageButtonHLayout->addStretch(1);
    addPoolVLayout->addLayout(pageButtonHLayout);

    contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupSearchPage());
    contentStackLayout->addWidget(setupCustomPage());
    addPoolVLayout->addLayout(contentStackLayout);

    keywordEdit->setText(srcAnime);
    animeEdit->setText(srcAnime);
    epEdit->setText(srcEp);
    renamePool=(!srcAnime.isEmpty() && !srcEp.isEmpty());
    if(renamePool) setTitle(tr("Rename Danmu Pool"));

    if(!srcAnime.isEmpty())
    {
        QScopedPointer<MatchInfo> match(KCache::getInstance()->get<MatchInfo>(QString("addpool/%1_%2").arg(searchLocation).arg(srcAnime)));
        if(match)
        {
            for(MatchInfo::DetailInfo &detailInfo:match->matches)
            {
                new QTreeWidgetItem(searchResult,QStringList()<<detailInfo.animeTitle<<detailInfo.title);
            }
        }
    }
    resize(GlobalObjects::appSetting->value("DialogSize/AddPool",QSize(400*logicalDpiX()/96,400*logicalDpiY()/96)).toSize());
}

QWidget *AddPool::setupSearchPage()
{
    QFont normalFont(GlobalObjects::normalFont,10);
    QWidget *searchPage=new QWidget(this);
    searchPage->setFont(normalFont);

    QLabel *sourceTip=new QLabel(tr("Source:"),searchPage);
    QRadioButton *bgmSource=new QRadioButton(tr("Bangumi"),searchPage);
    QObject::connect(bgmSource,&QRadioButton::toggled,[this](bool checked){
       if(checked)searchLocation=1;
    });
    QRadioButton *dandanSource=new QRadioButton(tr("DanDanPlay"),searchPage);
    QObject::connect(dandanSource,&QRadioButton::toggled,[this](bool checked){
       if(checked)searchLocation=0;
    });
    bgmSource->toggle();

    keywordEdit=new QLineEdit(searchPage);
    QPushButton *searchButton=new QPushButton(tr("Search"),searchPage);

    searchResult=new QTreeWidget(searchPage);
    searchResult->setRootIsDecorated(false);
    searchResult->setFont(searchPage->font());
    searchResult->setHeaderLabels(QStringList()<<tr("Anime Title")<<tr("Subtitle"));
    searchResult->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *copy=new QAction(tr("Copy"), this);
    QObject::connect(copy, &QAction::triggered, this, [this](){
        auto selectedItems=searchResult->selectedItems();
        if(selectedItems.count()==0) return;
        QTreeWidgetItem *selectedItem=selectedItems.first();
        QClipboard *cb = QApplication::clipboard();
        cb->setText(QString("%1 %2").arg(selectedItem->data(0,0).toString(), selectedItem->data(1,0).toString()));
    });
    searchResult->addAction(copy);

    QObject::connect(searchButton,&QPushButton::clicked,this,[this,searchButton](){
        QString keyword=keywordEdit->text().trimmed();
        if(keyword.isEmpty())return;
        if(!hitWords.contains(keyword))
        {
            QScopedPointer<MatchInfo> match(KCache::getInstance()->get<MatchInfo>(QString("addpool/%1_%2").arg(searchLocation).arg(keyword)));
            if(match)
            {
                searchResult->clear();
                for(MatchInfo::DetailInfo &detailInfo:match->matches)
                {
                    new QTreeWidgetItem(searchResult,QStringList()<<detailInfo.animeTitle<<detailInfo.title);
                }
                hitWords<<keyword;
                return;
            }
        }
        keywordEdit->setEnabled(false);
        searchButton->setEnabled(false);
        searchButton->setText(tr("Searching"));
        showBusyState(true);
        MatchInfo *sInfo=nullptr;
        sInfo=GlobalObjects::danmuManager->searchMatch(DanmuManager::MatchProvider(searchLocation),keyword);
        if(sInfo)
        {
            if(sInfo->error)
                showMessage(sInfo->errorInfo,1);
            else
            {
                searchResult->clear();
                for(MatchInfo::DetailInfo &detailInfo:sInfo->matches)
                {
                    new QTreeWidgetItem(searchResult,QStringList()<<detailInfo.animeTitle<<detailInfo.title);
                }
                KCache::getInstance()->put(QString("addpool/%1_%2").arg(searchLocation).arg(keyword), *sInfo);
                hitWords.remove(keyword);
            }
            delete sInfo;
        }
        showBusyState(false);
        searchButton->setEnabled(true);
        keywordEdit->setEnabled(true);
        searchButton->setText(tr("Search"));
    });
    QObject::connect(keywordEdit,&QLineEdit::returnPressed,searchButton,&QPushButton::click);

    QGridLayout *searchPageGLayout=new QGridLayout(searchPage);
    searchPageGLayout->addWidget(sourceTip,0,0);
    searchPageGLayout->addWidget(bgmSource,0,1);
    searchPageGLayout->addWidget(dandanSource,0,2);
    searchPageGLayout->addWidget(keywordEdit,1,0,1,3);
    searchPageGLayout->addWidget(searchButton,1,3);
    searchPageGLayout->addWidget(searchResult,2,0,1,4);
    searchPageGLayout->setRowStretch(2,1);
    searchPageGLayout->setColumnStretch(0,1);
    return searchPage;
}

QWidget *AddPool::setupCustomPage()
{
    QWidget *customPage=new QWidget(this);
    QFont normalFont(GlobalObjects::normalFont,10);
    customPage->setFont(normalFont);

    QLabel *animeTip=new QLabel(tr("Anime Title"),customPage);
    animeEdit=new QLineEdit(customPage);
    QLabel *subtitleTip=new QLabel(tr("Subtitle"),customPage);
    epEdit=new QLineEdit(customPage);

    QVBoxLayout *customVLayout=new QVBoxLayout(customPage);
    customVLayout->addWidget(animeTip);
    customVLayout->addWidget(animeEdit);
    customVLayout->addWidget(subtitleTip);
    customVLayout->addWidget(epEdit);
    customVLayout->addStretch(1);

    return customPage;
}

void AddPool::onAccept()
{
    if(searchPage->isChecked())
    {
        auto selectedItems=searchResult->selectedItems();
        if(selectedItems.count()==0)
        {
            showMessage(tr("You need to choose one"),1);
            return;
        }
        QTreeWidgetItem *selectedItem=selectedItems.first();
        animeTitle=selectedItem->data(0,0).toString();
        epTitle=selectedItem->data(1,0).toString();
    }
    else if(customPage->isChecked())
    {
        animeTitle=animeEdit->text().trimmed();
        epTitle=epEdit->text().trimmed();
    }
    if(animeTitle.isEmpty() || epTitle.isEmpty())
    {
        showMessage(tr("AnimeTitle and Subtitle can't be empty"),1);
        return;
    }
    if(!renamePool && GlobalObjects::danmuManager->getPool(animeTitle,epTitle,false))
    {
        showMessage(tr("Pool Already Exists"),1);
        return;
    }
    GlobalObjects::appSetting->setValue("DialogSize/AddPool",size());
    CFramelessDialog::onAccept();
}

void AddPool::onClose()
{
    GlobalObjects::appSetting->setValue("DialogSize/AddPool",size());
    CFramelessDialog::onClose();
}

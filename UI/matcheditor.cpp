#include "matcheditor.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/Provider/matchprovider.h"
#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QMessageBox>
#include <QButtonGroup>
#include "globalobjects.h"
MatchEditor::MatchEditor(const PlayListItem *item, MatchInfo *matchInfo, QWidget *parent) : CFramelessDialog(tr("Edit Match"),parent,true)
{
    QVBoxLayout *matchVLayout=new QVBoxLayout(this);
    matchVLayout->setContentsMargins(0,0,0,0);
    matchVLayout->setSpacing(0);

    setFont(QFont("Microsoft YaHei UI",12));

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
    QSpacerItem *pageButtonHSpacer = new QSpacerItem(0, 10, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    pageButtonHLayout->addItem(pageButtonHSpacer);
    matchVLayout->addLayout(pageButtonHLayout);

    contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupSearchPage(matchInfo));
    contentStackLayout->addWidget(setupCustomPage());
    matchVLayout->addLayout(contentStackLayout);

    QString matchStr=item->poolID.isEmpty()?tr("No Match Info"):QString("%1-%2").arg(item->animeTitle).arg(item->title);
    QLabel *matchInfoLabel=new QLabel(matchStr,this);
    matchInfoLabel->setFont(QFont("Microsoft YaHei UI",10,QFont::Bold));
    matchVLayout->addWidget(matchInfoLabel);
    keywordEdit->setText(item->animeTitle.isEmpty()?item->title:item->animeTitle);
    animeEdit->setText(item->animeTitle);
    subtitleEdit->setText(item->title);
    this->matchInfo=nullptr;
    resize(GlobalObjects::appSetting->value("DialogSize/MatchEditor",QSize(400*logicalDpiX()/96,400*logicalDpiY()/96)).toSize());

}

MatchEditor::~MatchEditor()
{
    if(matchInfo)delete matchInfo;
}

QWidget *MatchEditor::setupSearchPage(MatchInfo *matchInfo)
{
    QFont normalFont("Microsoft Yahei UI",10);
    QWidget *searchPage=new QWidget(this);
    searchPage->setFont(normalFont);

    QLabel *sourceTip=new QLabel(tr("Source:"),searchPage);
    QRadioButton *dandanSource=new QRadioButton(tr("DanDanPlay"),searchPage);
    QObject::connect(dandanSource,&QRadioButton::toggled,[this](bool checked){
       if(checked)searchLocation=0;
    });
    QRadioButton *bgmSource=new QRadioButton(tr("Bangumi"),searchPage);
    QObject::connect(bgmSource,&QRadioButton::toggled,[this](bool checked){
       if(checked)searchLocation=1;
    });
    QRadioButton *localSource=new QRadioButton(tr("Local DB"),searchPage);
    QObject::connect(localSource,&QRadioButton::toggled,[this](bool checked){
       if(checked)searchLocation=2;
    });
    dandanSource->toggle();
    keywordEdit=new QLineEdit(searchPage);
	keywordEdit->installEventFilter(this);

    searchButton=new QPushButton(tr("Search"),searchPage);
    searchButton->setDefault(false);
    searchButton->setAutoDefault(false);
    QObject::connect(searchButton,&QPushButton::clicked,this,&MatchEditor::search);

    searchResult=new QTreeWidget(searchPage);
    searchResult->setRootIsDecorated(false);
    searchResult->setFont(searchPage->font());
    searchResult->setHeaderLabels(QStringList()<<tr("Anime Title")<<tr("Subtitle"));
    if(matchInfo)
    {
        for(MatchInfo::DetailInfo &detailInfo:matchInfo->matches)
        {
            QTreeWidgetItem *item=new QTreeWidgetItem(searchResult,
                                                      QStringList()<<detailInfo.animeTitle<<detailInfo.title);
        }
    }
    QGridLayout *searchPageGLayout=new QGridLayout(searchPage);
    searchPageGLayout->addWidget(sourceTip,0,0);
    searchPageGLayout->addWidget(dandanSource,0,1);
    searchPageGLayout->addWidget(bgmSource,0,2);
    searchPageGLayout->addWidget(localSource,0,3);
    searchPageGLayout->addWidget(keywordEdit,1,0,1,3);
    searchPageGLayout->addWidget(searchButton,1,3);
    searchPageGLayout->addWidget(searchResult,2,0,1,4);
    searchPageGLayout->setRowStretch(2,1);
    searchPageGLayout->setColumnStretch(0,1);
    return searchPage;
}

QWidget *MatchEditor::setupCustomPage()
{
    QWidget *customPage=new QWidget(this);
    QFont normalFont("Microsoft Yahei UI",10);
    customPage->setFont(normalFont);

    QLabel *animeTip=new QLabel(tr("Anime Title"),customPage);
    animeEdit=new QLineEdit(customPage);
    QLabel *subtitleTip=new QLabel(tr("Subtitle"),customPage);
    subtitleEdit=new QLineEdit(customPage);

    QVBoxLayout *customVLayout=new QVBoxLayout(customPage);
    customVLayout->addWidget(animeTip);
    customVLayout->addWidget(animeEdit);
    customVLayout->addWidget(subtitleTip);
    customVLayout->addWidget(subtitleEdit);
    QSpacerItem *vSpacer = new QSpacerItem(1, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    customVLayout->addItem(vSpacer);

    return customPage;
}

void MatchEditor::search()
{
    QString keyword=keywordEdit->text().trimmed();
    if(keyword.isEmpty())return;
    searchButton->setEnabled(false);
    searchButton->setText(tr("Searching"));
    showBusyState(true);
    MatchInfo *sInfo=nullptr;
    if(searchLocation==0)//DanDan
    {
        sInfo = MatchProvider::SearchFormDandan(keyword);
    }
    else if(searchLocation==1)//Bangumi
    {
        sInfo=MatchProvider::SearchFormBangumi(keyword);
    }
    else if(searchLocation==2)//Local
    {
        sInfo=MatchProvider::SerchFromDB(keyword);
    }
    if(sInfo)
    {
        if(sInfo->error)
            QMessageBox::information(this,"Error",sInfo->errorInfo);
        else
        {
            searchResult->clear();
            for(MatchInfo::DetailInfo &detailInfo:sInfo->matches)
            {
                QTreeWidgetItem *item=new QTreeWidgetItem(searchResult,
                                                          QStringList()<<detailInfo.animeTitle<<detailInfo.title);
            }
        }
        delete sInfo;
    }
    showBusyState(false);
    searchButton->setEnabled(true);
    searchButton->setText(tr("Search"));
}

bool MatchEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == keywordEdit && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key()==Qt::Key_Enter || keyEvent->key()==Qt::Key_Return)
        {
            search();
            return true;
        }
        return false;
    }
    return CFramelessDialog::eventFilter(watched, event);
}

void MatchEditor::onAccept()
{
    QString animeTitle,title;
    if(searchPage->isChecked())
    {
        auto selectedItems=searchResult->selectedItems();
        if(selectedItems.count()==0)
        {
            QMessageBox::information(this,tr("Tip"),tr("You need to choose one"));
            return;
        }
        QTreeWidgetItem *selectedItem=selectedItems.first();
        animeTitle=selectedItem->data(0,0).toString();
        title=selectedItem->data(1,0).toString();
    }
    else if(customPage->isChecked())
    {
        animeTitle=animeEdit->text().trimmed();
        title=subtitleEdit->text().trimmed();
        if(animeTitle.isEmpty()||title.isEmpty())
        {
            QMessageBox::information(this,tr("Tip"),tr("AnimeTitle and Subtitle can't be empty"));
            return;
        }
    }
    matchInfo=new MatchInfo;
    matchInfo->error=false;
    MatchInfo::DetailInfo detailInfo;
    detailInfo.animeTitle=animeTitle;
    detailInfo.title=title;
    matchInfo->matches.append(detailInfo);
    GlobalObjects::appSetting->setValue("DialogSize/MatchEditor",size());
    CFramelessDialog::onAccept();
}

void MatchEditor::onClose()
{
    GlobalObjects::appSetting->setValue("DialogSize/MatchEditor",size());
    CFramelessDialog::onClose();
}

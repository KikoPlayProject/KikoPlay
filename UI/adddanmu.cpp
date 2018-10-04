#include "adddanmu.h"

#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QMessageBox>
#include <QTextEdit>
#include <QHeaderView>
#include <QButtonGroup>

#include "Play/Danmu/providermanager.h"
#include "Play/Playlist/playlist.h"
#include "selectepisode.h"
#include "globalobjects.h"

AddDanmu::AddDanmu(const PlayListItem *item,QWidget *parent) : CFramelessDialog(tr("Add Danmu"),parent,true),processCounter(0)
{
    QVBoxLayout *danmuVLayout=new QVBoxLayout(this);
    danmuVLayout->setContentsMargins(0,0,0,0);
    danmuVLayout->setSpacing(0);

   // QWidget *contentWidget=new QWidget(this);
    QFont normalFont("Microsoft YaHei UI",12);

    QSize pageButtonSize(90 *logicalDpiX()/96,36*logicalDpiY()/96);
    onlineDanmuPage=new QToolButton(this);
    onlineDanmuPage->setFont(normalFont);
    onlineDanmuPage->setText(tr("Search"));
    onlineDanmuPage->setCheckable(true);
    onlineDanmuPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    onlineDanmuPage->setFixedSize(pageButtonSize);
    onlineDanmuPage->setObjectName(QStringLiteral("DialogPageButton"));
	onlineDanmuPage->setChecked(true);

    urlDanmuPage=new QToolButton(this);
    urlDanmuPage->setFont(normalFont);
    urlDanmuPage->setText(tr("URL"));
    urlDanmuPage->setCheckable(true);
    urlDanmuPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    urlDanmuPage->setFixedSize(pageButtonSize);
    urlDanmuPage->setObjectName(QStringLiteral("DialogPageButton"));

    selectedDanmuPage=new QToolButton(this);
    selectedDanmuPage->setFont(normalFont);
    selectedDanmuPage->setText(tr("Selected"));
    selectedDanmuPage->setCheckable(true);
    selectedDanmuPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    selectedDanmuPage->setFixedHeight(pageButtonSize.height());
    selectedDanmuPage->setObjectName(QStringLiteral("DialogPageButton"));

    QMovie *downloadingIcon=new QMovie(this);
    movieLabel=new QLabel(this);
    movieLabel->setMovie(downloadingIcon);
    downloadingIcon->setFileName(":/res/images/loading-spinner.gif");
    movieLabel->setFixedSize(32,32);
    movieLabel->setScaledContents(true);
    downloadingIcon->start();

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    pageButtonHLayout->addWidget(onlineDanmuPage);
    pageButtonHLayout->addWidget(urlDanmuPage);
    pageButtonHLayout->addWidget(selectedDanmuPage);   
    pageButtonHLayout->addStretch(1);
    pageButtonHLayout->addWidget(movieLabel);
    danmuVLayout->addLayout(pageButtonHLayout);

    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupSearchPage());
    contentStackLayout->addWidget(setupURLPage());
    contentStackLayout->addWidget(setupSelectedPage());
    danmuVLayout->addLayout(contentStackLayout);

	QButtonGroup *btnGroup = new QButtonGroup(this);
	btnGroup->addButton(onlineDanmuPage, 0);
	btnGroup->addButton(urlDanmuPage, 1);
	btnGroup->addButton(selectedDanmuPage, 2);
	QObject::connect(btnGroup, (void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled, [contentStackLayout](int id, bool checked) {
		if (checked)
		{
			contentStackLayout->setCurrentIndex(id);
		}
	});

    QLabel *itemInfoLabel=new QLabel(item?(item->animeTitle.isEmpty()?item->title:QString("%1-%2").arg(item->animeTitle).arg(item->title)):"",this);
    itemInfoLabel->setFont(QFont("Microsoft YaHei UI",10,QFont::Bold));
    danmuVLayout->addWidget(itemInfoLabel);

    keywordEdit->setText(item?(item->animeTitle.isEmpty()?item->title:item->animeTitle):"");
    keywordEdit->installEventFilter(this);
    searchButton->setAutoDefault(false);
    searchButton->setDefault(false);
    movieLabel->hide();
    resize(GlobalObjects::appSetting->value("DialogSize/AddDanmu",QSize(600*logicalDpiX()/96,500*logicalDpiY()/96)).toSize());
}

void AddDanmu::search()
{
    QString keyword=keywordEdit->text().trimmed();
    if(keyword.isEmpty())return;
    beginProcrss();
    searchButton->setEnabled(false);
    keywordEdit->setEnabled(false);
    if(searchResultWidget->count()>0)
        searchResultWidget->setEnabled(false);
    QString tmpProviderId=sourceCombo->currentText();
    DanmuAccessResult *searchResult=GlobalObjects::providerManager->search(tmpProviderId,keyword);
    if(searchResult->error)
        QMessageBox::information(this,"Error",searchResult->errorInfo);
    else
    {
        providerId=tmpProviderId;
        searchResultWidget->clear();
        searchResultWidget->setEnabled(true);
        for(DanmuSourceItem &item:searchResult->list)
        {
            SearchItemWidget *itemWidget=new SearchItemWidget(&item);
            QObject::connect(itemWidget, &SearchItemWidget::addSearchItem, [this](DanmuSourceItem *item){
                beginProcrss();
                searchButton->setEnabled(false);
                keywordEdit->setEnabled(false);
                DanmuAccessResult *result=GlobalObjects::providerManager->getEpInfo(providerId,item);
                addSearchItem(result);
                delete result;
                searchButton->setEnabled(true);
                keywordEdit->setEnabled(true);
                endProcess();
            });
            QListWidgetItem *listItem=new QListWidgetItem(searchResultWidget);
            searchResultWidget->setItemWidget(listItem,itemWidget);
            listItem->setSizeHint(QSize(0, 80*logicalDpiY()/96));
            QCoreApplication::processEvents();
        }
        //searchResultWidget->update();
    }
    delete searchResult;
    searchButton->setEnabled(true);
    keywordEdit->setEnabled(true);
    searchResultWidget->setEnabled(true);
    endProcess();
}

void AddDanmu::addSearchItem(DanmuAccessResult *result)
{
    QString errorInfo;
    beginProcrss();
    if(result->error)
    {
        errorInfo=result->errorInfo;
    }
    else if(result->list.count()==1)
    {
        QList<DanmuComment *> tmplist;
        DanmuSourceItem &sourceItem=result->list.first();
        errorInfo = GlobalObjects::providerManager->downloadDanmu(result->providerId,&sourceItem,tmplist);
        if(errorInfo.isEmpty())
        {
            DanmuSourceInfo sourceInfo;
            sourceInfo.count=tmplist.count();
            sourceInfo.name=sourceItem.title;
            sourceInfo.url=GlobalObjects::providerManager->getSourceURL(result->providerId,&sourceItem);
            sourceInfo.delay=0;
            sourceInfo.show=true;
            selectedDanmuList.append(QPair<DanmuSourceInfo,QList<DanmuComment *> >(sourceInfo,tmplist));
            QTreeWidgetItem *widgetItem=new QTreeWidgetItem(selectedDanmuWidget,QStringList()<<sourceInfo.name<<QString::number(sourceInfo.count)<<result->providerId);
            widgetItem->setCheckState(0,Qt::Checked);
        }
    }
    else
    {
        SelectEpisode selectEpisode(result,this);
        if(QDialog::Accepted==selectEpisode.exec())
        {
            for(DanmuSourceItem &sourceItem:result->list)
            {
                QList<DanmuComment *> tmplist;
                errorInfo = GlobalObjects::providerManager->downloadDanmu(result->providerId,&sourceItem,tmplist);
                if(errorInfo.isEmpty())
                {
                    DanmuSourceInfo sourceInfo;
                    sourceInfo.count=tmplist.count();
                    sourceInfo.name=sourceItem.title;
                    sourceInfo.url=GlobalObjects::providerManager->getSourceURL(result->providerId,&sourceItem);
                    sourceInfo.delay=sourceItem.delay*1000;
                    sourceInfo.show=true;
                    selectedDanmuList.append(QPair<DanmuSourceInfo,QList<DanmuComment *> >(sourceInfo,tmplist));
                    QTreeWidgetItem *widgetItem=new QTreeWidgetItem(selectedDanmuWidget,QStringList()<<sourceInfo.name<<QString::number(sourceInfo.count)<<result->providerId);
                    widgetItem->setCheckState(0,Qt::Checked);
                    selectedDanmuPage->setText(tr("Selected(%1)").arg(selectedDanmuWidget->topLevelItemCount()));
                }
            }
        }
    }
    if(!errorInfo.isEmpty())
        QMessageBox::information(this,"Error",errorInfo);
    selectedDanmuPage->setText(tr("Selected(%1)").arg(selectedDanmuWidget->topLevelItemCount()));
    endProcess();
}

void AddDanmu::addURL()
{
    QString url=urlEdit->text().trimmed();
    if(url.isEmpty()) return;
    addUrlButton->setEnabled(false);
    urlEdit->setEnabled(false);
    beginProcrss();
    DanmuAccessResult *result=GlobalObjects::providerManager->getURLInfo(url);
    if(result->error)
    {
        QMessageBox::information(this,"Error",result->errorInfo);
    }
    else
    {
        addSearchItem(result);
    }
    delete result;
    addUrlButton->setEnabled(true);
    urlEdit->setEnabled(true);
    endProcess();
}

QWidget *AddDanmu::setupSearchPage()
{
    QWidget *searchPage=new QWidget(this);
    searchPage->setFont(QFont("Microsoft Yahei UI",10));
    sourceCombo=new QComboBox(searchPage);
    sourceCombo->addItems(GlobalObjects::providerManager->getSearchProviders());
    keywordEdit=new QLineEdit(searchPage);
    searchButton=new QPushButton(tr("Search"),searchPage);
    QObject::connect(searchButton,&QPushButton::clicked,this,&AddDanmu::search);
    searchResultWidget=new QListWidget(searchPage);
    QGridLayout *searchPageGLayout=new QGridLayout(searchPage);
    searchPageGLayout->addWidget(sourceCombo,0,0);
    searchPageGLayout->addWidget(keywordEdit,0,1);
    searchPageGLayout->addWidget(searchButton,0,2);
    searchPageGLayout->addWidget(searchResultWidget,1,0,1,3);
    searchPageGLayout->setColumnStretch(1,1);
    searchPageGLayout->setRowStretch(1,1);

    return searchPage;
}

QWidget *AddDanmu::setupURLPage()
{
    QWidget *urlPage=new QWidget(this);
    urlPage->setFont(QFont("Microsoft Yahei UI",10));

    QLabel *tipLabel=new QLabel(tr("Input URL:"),urlPage);
    tipLabel->setFont(QFont("Microsoft Yahei UI",12,QFont::Medium));

    urlEdit=new QLineEdit(urlPage);

    addUrlButton=new QPushButton(tr("Add URL"),urlPage);
    addUrlButton->setMinimumWidth(150);
    addUrlButton->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    QObject::connect(addUrlButton,&QPushButton::clicked,this,&AddDanmu::addURL);

    QLabel *urlTipLabel=new QLabel(tr("Supported URL:"),urlPage);
    QTextEdit *supportUrlInfo=new QTextEdit(urlPage);
    supportUrlInfo->setText(GlobalObjects::providerManager->getSupportedURLs().join('\n'));
    supportUrlInfo->setFont(QFont("Microsoft Yahei UI",10));
    supportUrlInfo->setReadOnly(true);


    QVBoxLayout *localVLayout=new QVBoxLayout(urlPage);
    localVLayout->addWidget(tipLabel);
    localVLayout->addWidget(urlEdit);
    localVLayout->addWidget(addUrlButton);
    localVLayout->addSpacing(10);
    localVLayout->addWidget(urlTipLabel);
    localVLayout->addWidget(supportUrlInfo);

    return urlPage;
}

QWidget *AddDanmu::setupSelectedPage()
{
    QWidget *selectedPage=new QWidget(this);
    selectedPage->setFont(QFont("Microsoft Yahei UI",12));
    QLabel *tipLabel=new QLabel(tr("Select danmu you want to add:"),selectedPage);
    selectedDanmuWidget =new QTreeWidget(selectedPage);
    selectedDanmuWidget->setRootIsDecorated(false);
    selectedDanmuWidget->setFont(selectedPage->font());
    selectedDanmuWidget->setHeaderLabels(QStringList()<<tr("Title")<<tr("DanmuCount")<<tr("Source"));
    selectedDanmuWidget->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    QHeaderView *selectedHeader = selectedDanmuWidget->header();
    selectedHeader->setFont(this->font());
    selectedHeader->resizeSection(0, 300*logicalDpiX()/96);
    QVBoxLayout *spVLayout=new QVBoxLayout(selectedPage);
    spVLayout->addWidget(tipLabel);
    spVLayout->addWidget(selectedDanmuWidget);
    return selectedPage;
}

void AddDanmu::beginProcrss()
{
    processCounter++;
    movieLabel->show();
}

void AddDanmu::endProcess()
{
    processCounter--;
    if(processCounter==0)movieLabel->hide();
}

void AddDanmu::onAccept()
{
    int count=selectedDanmuWidget->topLevelItemCount();
    for(int i=count-1;i>=0;i--)
    {
        if(selectedDanmuWidget->topLevelItem(i)->checkState(0)!=Qt::Checked)
        {
            qDeleteAll(selectedDanmuList[i].second);
            selectedDanmuList.removeAt(i);
        }
    }
    GlobalObjects::appSetting->setValue("DialogSize/AddDanmu",size());
    CFramelessDialog::onAccept();
}

void AddDanmu::onClose()
{
    for(auto iter=selectedDanmuList.begin();iter!=selectedDanmuList.end();++iter)
    {
        qDeleteAll((*iter).second);
    }
    GlobalObjects::appSetting->setValue("DialogSize/AddDanmu",size());
    CFramelessDialog::onClose();
}

bool AddDanmu::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key()==Qt::Key_Enter || keyEvent->key()==Qt::Key_Return)
        {
            if(watched == keywordEdit)
            {
                search();
                return true;
            }
            else if(watched==urlEdit)
            {
                addURL();
                return true;
            }
        }
        return false;
    }
    return CFramelessDialog::eventFilter(watched, event);
}


SearchItemWidget::SearchItemWidget(DanmuSourceItem *item):searchItem(*item)
{
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QLabel *titleLabel=new QLabel(this);
	QLabel *descLabel = new QLabel(this);
    titleLabel->setToolTip(item->title);
	titleLabel->adjustSize();
	titleLabel->setText(QString("<font size=\"5\" face=\"Microsoft Yahei\" color=\"#f33aa0\">%1</font>").arg(item->title));
	titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
	descLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
	descLabel->setText(QString("<font size=\"3\" face=\"Microsoft Yahei\">%1</font>").arg(item->description.trimmed()));
	descLabel->setToolTip(item->description);
    QPushButton *addItemButton=new QPushButton(tr("Add"),this);
    QObject::connect(addItemButton,&QPushButton::clicked,[this,addItemButton](){
		addItemButton->setEnabled(false);
		emit addSearchItem(&searchItem);
		addItemButton->setEnabled(true);
	});
    QHBoxLayout *itemHLayout=new QHBoxLayout(this);
	QVBoxLayout *lbVLayout = new QVBoxLayout();
	lbVLayout->addWidget(titleLabel);
	lbVLayout->addWidget(descLabel);
	lbVLayout->setContentsMargins(0, 0, 0, 0);
	itemHLayout->addLayout(lbVLayout);
    itemHLayout->addWidget(addItemButton);
}

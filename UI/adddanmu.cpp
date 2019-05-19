#include "adddanmu.h"

#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTreeView>
#include <QTextEdit>
#include <QHeaderView>
#include <QButtonGroup>
#include <QAction>

#include "Play/Danmu/providermanager.h"
#include "Play/Playlist/playlist.h"
#include "selectepisode.h"
#include "danmuview.h"
#include "Play/Danmu/blocker.h"
#include "globalobjects.h"

AddDanmu::AddDanmu(const PlayListItem *item,QWidget *parent,bool autoPauseVideo,const QStringList &poolList) : CFramelessDialog(tr("Add Danmu"),parent,true,true,autoPauseVideo),
    danmuPools(poolList),processCounter(0)
{
    danmuItemModel=new DanmuItemModel(this,!danmuPools.isEmpty(),item?item->title:"",this);

    QVBoxLayout *danmuVLayout=new QVBoxLayout(this);
    danmuVLayout->setContentsMargins(0,0,0,0);
    danmuVLayout->setSpacing(0);

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


    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    pageButtonHLayout->addWidget(onlineDanmuPage);
    pageButtonHLayout->addWidget(urlDanmuPage);
    pageButtonHLayout->addWidget(selectedDanmuPage);   
    pageButtonHLayout->addStretch(1);
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
    QString itemInfo(item?(item->animeTitle.isEmpty()?item->title:QString("%1-%2").arg(item->animeTitle).arg(item->title)):"");
    QLabel *itemInfoLabel=new QLabel(itemInfo,this);
    itemInfoLabel->setFont(QFont("Microsoft YaHei UI",10,QFont::Bold));
    itemInfoLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    danmuVLayout->addWidget(itemInfoLabel);

    keywordEdit->setText(item?(item->animeTitle.isEmpty()?item->title:item->animeTitle):"");
    keywordEdit->installEventFilter(this);
    searchButton->setAutoDefault(false);
    searchButton->setDefault(false);
    resize(GlobalObjects::appSetting->value("DialogSize/AddDanmu",QSize(600*logicalDpiX()/96,500*logicalDpiY()/96)).toSize());
}

void AddDanmu::search()
{
    QString keyword=keywordEdit->text().trimmed();
    if(keyword.isEmpty())return;
    beginProcrss();
    if(searchResultWidget->count()>0)
        searchResultWidget->setEnabled(false);
    QString tmpProviderId=sourceCombo->currentText();
    DanmuAccessResult *searchResult=GlobalObjects::providerManager->search(tmpProviderId,keyword);
    if(searchResult->error)
        showMessage(searchResult->errorInfo,1);
    else
    {
        providerId=tmpProviderId;
        searchResultWidget->clear();
        searchResultWidget->setEnabled(true);
        for(DanmuSourceItem &item:searchResult->list)
        {
            SearchItemWidget *itemWidget=new SearchItemWidget(&item);
            QObject::connect(itemWidget, &SearchItemWidget::addSearchItem, itemWidget, [this](DanmuSourceItem *item){
                beginProcrss();
                DanmuAccessResult *result=GlobalObjects::providerManager->getEpInfo(providerId,item);
                addSearchItem(result);
                delete result;
                endProcess();
            });
            QListWidgetItem *listItem=new QListWidgetItem(searchResultWidget);
            searchResultWidget->setItemWidget(listItem,itemWidget);
            listItem->setSizeHint(itemWidget->sizeHint());
            QCoreApplication::processEvents();
        }
        //searchResultWidget->update();
    }
    delete searchResult;
    searchResultWidget->setEnabled(true);
    endProcess();
}

void AddDanmu::addSearchItem(DanmuAccessResult *result)
{
    QString errorInfo;
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
            int srcCount=tmplist.count();
            GlobalObjects::blocker->preFilter(tmplist);
            int filterCount=srcCount - tmplist.count();
            if(filterCount>0) showMessage(tr("Pre-filter %1 Danmu").arg(filterCount));
            DanmuSourceInfo sourceInfo;
            sourceInfo.count=tmplist.count();
            sourceInfo.name=sourceItem.title;
            sourceInfo.url=GlobalObjects::providerManager->getSourceURL(result->providerId,&sourceItem);
            sourceInfo.delay=0;
            sourceInfo.show=true;
            selectedDanmuList.append(QPair<DanmuSourceInfo,QList<DanmuComment *> >(sourceInfo,tmplist));
            danmuItemModel->addItem(sourceItem.title,sourceItem.extra,result->providerId,sourceInfo.count);
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
                    int srcCount=tmplist.count();
                    GlobalObjects::blocker->preFilter(tmplist);
                    int filterCount=srcCount - tmplist.count();
                    if(filterCount>0) showMessage(tr("Pre-filter %1 Danmu").arg(filterCount));
                    DanmuSourceInfo sourceInfo;
                    sourceInfo.count=tmplist.count();
                    sourceInfo.name=sourceItem.title;
                    sourceInfo.url=GlobalObjects::providerManager->getSourceURL(result->providerId,&sourceItem);
                    sourceInfo.delay=sourceItem.delay*1000;
                    sourceInfo.show=true;
                    selectedDanmuList.append(QPair<DanmuSourceInfo,QList<DanmuComment *> >(sourceInfo,tmplist));
                    danmuItemModel->addItem(sourceItem.title,sourceItem.extra,result->providerId,sourceInfo.count);

                    selectedDanmuPage->setText(tr("Selected(%1)").arg(selectedDanmuList.count()));
                }
            }
        }
    }
    if(!errorInfo.isEmpty())
        showMessage(errorInfo,1);
    selectedDanmuPage->setText(tr("Selected(%1)").arg(selectedDanmuList.count()));
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
        showMessage(result->errorInfo,1);
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
    selectedDanmuView=new QTreeView(selectedPage);
    selectedDanmuView->setRootIsDecorated(false);
    selectedDanmuView->setFont(selectedPage->font());
    selectedDanmuView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    selectedDanmuView->setModel(danmuItemModel);
    selectedDanmuView->setItemDelegate(new PoolComboDelegate(danmuPools,this));
    QHeaderView *selectedHeader = selectedDanmuView->header();
    selectedHeader->setFont(this->font());
    selectedHeader->resizeSection(0, 220*logicalDpiX()/96);

    QVBoxLayout *spVLayout=new QVBoxLayout(selectedPage);
    spVLayout->addWidget(tipLabel);
    spVLayout->addWidget(selectedDanmuView);

    QAction *actView=new QAction(tr("View Danmu"),this);
    QObject::connect(actView,&QAction::triggered,this,[this](){
        auto selection = selectedDanmuView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        int row=selection.first().row();
        DanmuView view(&selectedDanmuList.at(row).second,this);
        view.exec();
    });
    selectedDanmuView->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    selectedDanmuView->addAction(actView);
    return selectedPage;
}

void AddDanmu::beginProcrss()
{
    processCounter++;
    showBusyState(true);
    searchButton->setEnabled(false);
    keywordEdit->setEnabled(false);
}

void AddDanmu::endProcess()
{
    processCounter--;
    if(processCounter==0)
    {
        searchButton->setEnabled(true);
        keywordEdit->setEnabled(true);
        showBusyState(false);
    }
}

void AddDanmu::onAccept()
{
    for(int i=selectedDanmuList.count()-1;i>=0;i--)
    {
        if(!danmuCheckedList[i])
        {
            qDeleteAll(selectedDanmuList[i].second);
            selectedDanmuList.removeAt(i);
            danmuToPoolList.removeAt(i);
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
    titleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    descLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
	descLabel->setText(QString("<font size=\"3\" face=\"Microsoft Yahei\">%1</font>").arg(item->description.trimmed()));
	descLabel->setToolTip(item->description);
    QPushButton *addItemButton=new QPushButton(tr("Add"),this);
    QObject::connect(addItemButton,&QPushButton::clicked,this,[this,addItemButton](){
		addItemButton->setEnabled(false);
		emit addSearchItem(&searchItem);
		addItemButton->setEnabled(true);
	});
    QGridLayout *searchPageGLayout=new QGridLayout(this);
    searchPageGLayout->addWidget(titleLabel,0,0);
    searchPageGLayout->addWidget(descLabel,1,0);
    searchPageGLayout->addWidget(addItemButton,0,1,2,1,Qt::AlignCenter);
    searchPageGLayout->setColumnMinimumWidth(1,50*logicalDpiX()/96);
    searchPageGLayout->setColumnStretch(0,6);
    searchPageGLayout->setColumnStretch(1,1);
}

QSize SearchItemWidget::sizeHint() const
{
    return layout()->sizeHint();
}

DanmuItemModel::DanmuItemModel(AddDanmu *dmDialog, bool hasPool, const QString &normalPool, QObject *parent) : QAbstractItemModel (parent),
    danmuToPoolList(&dmDialog->danmuToPoolList),danmuCheckedList(&dmDialog->danmuCheckedList),hasPoolInfo(hasPool),nPool(normalPool)
{

}

void DanmuItemModel::addItem(const QString &title, int duration, const QString &provider, int count)
{
    ItemInfo newItem;
    newItem.title=title;
    newItem.count=count;
    newItem.provider=provider;
    int min=duration/60;
    int sec=duration-min*60;
    newItem.duration=QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
    beginInsertRows(QModelIndex(),items.count(),items.count());
    items.append(newItem);
    endInsertRows();
    danmuCheckedList->append(true);
    danmuToPoolList->append(nPool);
}

QVariant DanmuItemModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    const ItemInfo &info=items.at(index.row());
    int col=index.column();
    if(role==Qt::DisplayRole)
    {
        switch (col)
        {
        case 0:
            return info.title;
        case 1:
            return info.count;
        case 2:
            return info.provider;
        case 3:
            return info.duration;
        case 4:
            return danmuToPoolList->at(index.row());
        }
    }
    else if(role==Qt::CheckStateRole)
    {
        if(col==0)
            return danmuCheckedList->at(index.row())?Qt::Checked:Qt::Unchecked;
    }
    return QVariant();
}

bool DanmuItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row=index.row(),col=index.column();
    switch (col)
    {
    case 0:
        (*danmuCheckedList)[row]=(value==Qt::Checked);
        break;
    case 4:
        (*danmuToPoolList)[row]=value.toString();
        break;
    default:
        return false;
    }
    emit dataChanged(index,index);
    return true;
}

QVariant DanmuItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers={tr("Title"),tr("DanmuCount"),tr("Source"),tr("Duration"),tr("DanmuPool")};
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<5)return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags DanmuItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    int col = index.column();
    if (index.isValid())
    {
        if(col==0)
            return  Qt::ItemIsUserCheckable | defaultFlags;
        else if(col==4)
            return  Qt::ItemIsEditable | defaultFlags;
    }
    return defaultFlags;
}

QWidget *PoolComboDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.column()==4)
    {
        QComboBox *combo=new QComboBox(parent);
        combo->setFrame(false);
        combo->addItems(poolList);
        return combo;
    }
    return QStyledItemDelegate::createEditor(parent,option,index);
}

void PoolComboDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if(index.column()==4)
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        combo->setCurrentIndex(poolList.indexOf(index.data(Qt::DisplayRole).toString()));
        return;
    }
    QStyledItemDelegate::setEditorData(editor,index);
}

void PoolComboDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if(index.column()==4)
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        model->setData(index,poolList.value(combo->currentIndex()),Qt::EditRole);
        return;
    }
    QStyledItemDelegate::setModelData(editor,model,index);
}

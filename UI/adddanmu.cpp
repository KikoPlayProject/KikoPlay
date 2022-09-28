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

#include "Play/Danmu/danmuprovider.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Playlist/playlist.h"
#include "selectepisode.h"
#include "danmuview.h"
#include "Play/Danmu/blocker.h"
#include "Script/scriptmanager.h"
#include "widgets/scriptsearchoptionpanel.h"
#include "globalobjects.h"
#include "Common/notifier.h"
namespace
{

    class RelWordCache
    {
    public:
        explicit RelWordCache():cacheChanged(false)
        {
             QFile cacheFile(GlobalObjects::dataPath + "relCache");
             if(cacheFile.open(QIODevice::ReadOnly))
             {
                QDataStream fs(&cacheFile);
                fs>>relWordHash;
             }
        }
        QStringList get(const QString &word)
        {
            return relWordHash.value(word);
        }
        void put(const QString &word, const QString &rel)
        {
            if(!relWordHash.contains(word))
            {
                relWordHash.insert(word, {rel});
            }
            else
            {
                QStringList &rels=relWordHash[word];
                if(rels.contains(rel)) rels.removeOne(rel);
                rels.push_front(rel);
            }
            cacheChanged=true;
        }
        void save()
        {
            if(cacheChanged)
            {
                QFile cacheFile(GlobalObjects::dataPath + "relCache");
                if(cacheFile.open(QIODevice::WriteOnly))
                {
                    QDataStream fs(&cacheFile);
                    for(auto &list:relWordHash)
                    {
                        if(list.count()>5)
                        {
                            list.erase(list.begin()+1, list.end());
                        }
                    }
                    fs<<relWordHash;
                }
            }
            cacheChanged=false;
        }
    private:
        QHash<QString, QStringList> relWordHash;
        bool cacheChanged;
    };
    RelWordCache *relCache=nullptr;
}
AddDanmu::AddDanmu(const PlayListItem *item,QWidget *parent,bool autoPauseVideo,const QStringList &poolList) : CFramelessDialog(tr("Add Danmu"),parent,true,true,autoPauseVideo),
    danmuPools(poolList),processCounter(0)
{
    if(!relCache) relCache=new RelWordCache();
    Pool *pool = nullptr;
    if(item) pool=GlobalObjects::danmuManager->getPool(item->poolID, false);
    danmuItemModel=new DanmuItemModel(this,!danmuPools.isEmpty(),pool?pool->toEp().toString():(item?item->title:""),this);

    QVBoxLayout *danmuVLayout=new QVBoxLayout(this);
    danmuVLayout->setContentsMargins(0,0,0,0);
    danmuVLayout->setSpacing(0);

    QFont normalFont(GlobalObjects::normalFont,12);
    QStringList pageButtonTexts = {
        tr("Search"),
        tr("URL"),
        tr("Selected")
    };
    QToolButton **infoBtnPtrs[] = {
        &onlineDanmuPage, &urlDanmuPage, &selectedDanmuPage
    };
    QFontMetrics fm(normalFont);
    int btnHeight = fm.height() + 10*logicalDpiY()/96;
    int btnWidth = 0;
    for(const QString &t : pageButtonTexts)
    {
        btnWidth = qMax(btnWidth, fm.horizontalAdvance(t));
    }
    btnWidth += 40*logicalDpiX()/96;

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    QButtonGroup *btnGroup = new QButtonGroup(this);
    for(int i = 0; i < pageButtonTexts.size(); ++i)
    {
        *infoBtnPtrs[i] = new QToolButton(this);
        QToolButton *tb = *infoBtnPtrs[i];
        tb->setFont(normalFont);
        tb->setText(pageButtonTexts[i]);
        tb->setCheckable(true);
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setFixedSize(QSize(btnWidth, btnHeight));
        tb->setObjectName(QStringLiteral("DialogPageButton"));
        pageButtonHLayout->addWidget(tb);
        btnGroup->addButton(tb, i);
    }
    onlineDanmuPage->setChecked(true);
    pageButtonHLayout->addStretch(1);
    danmuVLayout->addLayout(pageButtonHLayout);

    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupSearchPage());
    contentStackLayout->addWidget(setupURLPage());
    contentStackLayout->addWidget(setupSelectedPage());
    danmuVLayout->addLayout(contentStackLayout);
    QObject::connect(btnGroup, (void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled, [contentStackLayout](int id, bool checked) {
        if (checked)
        {
            contentStackLayout->setCurrentIndex(id);
        }
    });

    QString itemInfo(item?(item->animeTitle.isEmpty()?item->title:QString("%1-%2").arg(item->animeTitle).arg(item->title)):"");
    QLabel *itemInfoLabel=new QLabel(itemInfo,this);
    itemInfoLabel->setFont(QFont(GlobalObjects::normalFont,10,QFont::Bold));
    itemInfoLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    danmuVLayout->addWidget(itemInfoLabel);

    keywordEdit->setText(item?(item->animeTitle.isEmpty()?item->title:item->animeTitle):"");
    keywordEdit->installEventFilter(this);
    searchButton->setAutoDefault(false);
    searchButton->setDefault(false);
    if(item && !item->animeTitle.isEmpty())
    {
        themeWord=item->animeTitle;
        relWordWidget->setRelWordList(relCache->get(themeWord));
    }
    setSizeSettingKey("DialogSize/AddDanmu", QSize(600*logicalDpiX()/96, 500*logicalDpiY()/96));
}

void AddDanmu::search()
{
    QString keyword=keywordEdit->text().trimmed();
    if(keyword.isEmpty())return;
    QString tmpProviderId=sourceCombo->currentData().toString();
    if(tmpProviderId.isEmpty()) return;
    beginProcrss();
    searchResultWidget->setEnabled(false);
    QList<DanmuSource> results;
    QMap<QString, QString> searchOptions;
    if(scriptOptionPanel->hasOptions() && scriptOptionPanel->changed())
    {
        searchOptions = scriptOptionPanel->getOptionVals();
    }
    auto ret = GlobalObjects::danmuProvider->danmuSearch(tmpProviderId, keyword, searchOptions, results);
    if(ret)
    {
        if(!themeWord.isEmpty() && themeWord!=keyword)
        {
            relCache->put(themeWord, keyword);
        }
        providerId=tmpProviderId;
        searchResultWidget->clear();
        searchResultWidget->setEnabled(true);
        for(auto &item: results)
        {
            SearchItemWidget *itemWidget=new SearchItemWidget(item);
            QObject::connect(itemWidget, &SearchItemWidget::addSearchItem, itemWidget, [this](SearchItemWidget *item){
                beginProcrss();
                QList<DanmuSource> results;
                auto ret = GlobalObjects::danmuProvider->getEpInfo(&item->source, results);
                if(ret) addSearchItem(results);
                else showMessage(ret.info, NM_ERROR | NM_HIDE);
                endProcess();
            });
            QListWidgetItem *listItem=new QListWidgetItem(searchResultWidget);
            searchResultWidget->setItemWidget(listItem,itemWidget);
            listItem->setSizeHint(itemWidget->sizeHint());
            QCoreApplication::processEvents();
        }
    } else {
        showMessage(ret.info, NM_ERROR | NM_HIDE);
    }
    searchResultWidget->setEnabled(true);
    endProcess();
}


void AddDanmu::addSearchItem(QList<DanmuSource> &sources)
{
    if(sources.empty()) return;
    ScriptState retState(ScriptState::S_NORM);
    if(sources.size() == 1)
    {
        QVector<DanmuComment *> tmplist;
        DanmuSource *nSrc = nullptr;
        auto ret = GlobalObjects::danmuProvider->downloadDanmu(&sources.first(), tmplist, &nSrc);
        if(ret)
        {
            int srcCount=tmplist.count();
            GlobalObjects::blocker->preFilter(tmplist);
            int filterCount=srcCount - tmplist.count();
            if(filterCount>0) showMessage(tr("Pre-filter %1 Danmu").arg(filterCount));
            DanmuSource src = nSrc? *nSrc:sources.first();
            src.count = tmplist.count();
            selectedDanmuList.append({src, tmplist});
            danmuItemModel->addItem(src);
        } else {
            showMessage(ret.info, NM_ERROR | NM_HIDE);
        }
        if(nSrc) delete nSrc;
    }
    else
    {
        SelectEpisode selectEpisode(sources, this);
        if(QDialog::Accepted==selectEpisode.exec())
        {
            for(auto &sourceItem:sources)
            {
                QVector<DanmuComment *> tmplist;
                DanmuSource *nSrc = nullptr;
                auto ret = GlobalObjects::danmuProvider->downloadDanmu(&sourceItem,tmplist, &nSrc);
                if(ret)
                {
                    int srcCount=tmplist.count();
                    GlobalObjects::blocker->preFilter(tmplist);
                    int filterCount=srcCount - tmplist.count();
                    if(filterCount>0) showMessage(tr("Pre-filter %1 Danmu").arg(filterCount));
                    DanmuSource src = nSrc? *nSrc:sourceItem;
                    src.count = tmplist.count();
                    selectedDanmuList.append({src, tmplist});
                    danmuItemModel->addItem(src);
                    selectedDanmuPage->setText(tr("Selected(%1)").arg(selectedDanmuList.count()));
                }
            }
        }
    }
    selectedDanmuPage->setText(tr("Selected(%1)").arg(selectedDanmuList.count()));
}

void AddDanmu::addURL()
{
    QString url=urlEdit->text().trimmed();
    if(url.isEmpty()) return;
    addUrlButton->setEnabled(false);
    urlEdit->setEnabled(false);
    beginProcrss();
    QList<DanmuSource> results;
    auto ret = GlobalObjects::danmuProvider->getURLInfo(url, results);
    if(!ret)
        showMessage(ret.info, NM_ERROR | NM_HIDE);
    else
        addSearchItem(results);
    addUrlButton->setEnabled(true);
    urlEdit->setEnabled(true);
    endProcess();
}

QWidget *AddDanmu::setupSearchPage()
{
    QWidget *searchPage=new QWidget(this);
    searchPage->setFont(QFont(GlobalObjects::normalFont,10));
    sourceCombo=new QComboBox(searchPage);
    scriptOptionPanel = new ScriptSearchOptionPanel(searchPage);
    QObject::connect(sourceCombo, &QComboBox::currentTextChanged, this, [=](const QString &){
        QString curId = sourceCombo->currentData().toString();
        scriptOptionPanel->setScript(GlobalObjects::scriptManager->getScript(curId));
        if(scriptOptionPanel->hasOptions()) scriptOptionPanel->show();
        else scriptOptionPanel->hide();
    });
    for(const auto &p : GlobalObjects::danmuProvider->getSearchProviders())
    {
        sourceCombo->addItem(p.first, p.second);  //p: <name, id>
    }
    keywordEdit=new QLineEdit(searchPage);
    searchButton=new QPushButton(tr("Search"),searchPage);
    QObject::connect(searchButton,&QPushButton::clicked,this,&AddDanmu::search);
    relWordWidget=new RelWordWidget(this);
    searchResultWidget=new QListWidget(searchPage);
    QObject::connect(relWordWidget, &RelWordWidget::relWordClicked, [this](const QString &relWord){
       keywordEdit->setText(relWord);
       search();
    });
    QGridLayout *searchPageGLayout=new QGridLayout(searchPage);
    searchPageGLayout->addWidget(sourceCombo,0,0);
    searchPageGLayout->addWidget(keywordEdit,0,1);
    searchPageGLayout->addWidget(searchButton,0,2);
    searchPageGLayout->addWidget(scriptOptionPanel,1,0,1,3);
    searchPageGLayout->addWidget(relWordWidget,2,0,1,3);
    searchPageGLayout->addWidget(searchResultWidget,3,0,1,3);
    searchPageGLayout->setColumnStretch(1,1);
    searchPageGLayout->setRowStretch(3,1);

    return searchPage;
}

QWidget *AddDanmu::setupURLPage()
{
    QWidget *urlPage=new QWidget(this);
    urlPage->setFont(QFont(GlobalObjects::normalFont,10));

    QLabel *tipLabel=new QLabel(tr("Input URL:"),urlPage);
    tipLabel->setFont(QFont(GlobalObjects::normalFont,12,QFont::Medium));

    urlEdit=new QLineEdit(urlPage);

    addUrlButton=new QPushButton(tr("Add URL"),urlPage);
    addUrlButton->setMinimumWidth(150);
    addUrlButton->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    QObject::connect(addUrlButton,&QPushButton::clicked,this,&AddDanmu::addURL);

    QLabel *urlTipLabel=new QLabel(tr("Supported URL:"),urlPage);
    QTextEdit *supportUrlInfo=new QTextEdit(urlPage);
    supportUrlInfo->setText(GlobalObjects::danmuProvider->getSampleURLs().join('\n'));
    supportUrlInfo->setFont(QFont(GlobalObjects::normalFont,10));
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
    selectedPage->setFont(QFont(GlobalObjects::normalFont,12));
    QLabel *tipLabel=new QLabel(tr("Select danmu you want to add:"),selectedPage);
    selectedDanmuView=new QTreeView(selectedPage);
    selectedDanmuView->setRootIsDecorated(false);
    selectedDanmuView->setFont(selectedPage->font());
    selectedDanmuView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    selectedDanmuView->setDragEnabled(true);
    selectedDanmuView->setAcceptDrops(true);
    selectedDanmuView->setDragDropMode(QAbstractItemView::InternalMove);
    selectedDanmuView->setDropIndicatorShown(true);
    selectedDanmuView->setSelectionMode(QTreeView::SingleSelection);
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
    QAction *actAutoSetPoolID=new QAction(tr("Set DanmuPool Sequentially From Current"),this);
    QObject::connect(actAutoSetPoolID,&QAction::triggered,this,[this](){
        if(danmuPools.size()<=1) return;
        auto selection = selectedDanmuView->selectionModel()->selectedRows();
        if (selection.size() == 0) return;
        int selectedIndex=selection.first().row();
        int poolIndex = danmuPools.indexOf(danmuToPoolList[selectedIndex]);
        for(++selectedIndex; selectedIndex<danmuToPoolList.size();++selectedIndex)
        {
            if(danmuCheckedList[selectedIndex])
            {
                if(++poolIndex >= danmuPools.size()) break;
                danmuItemModel->setData(danmuItemModel->index(selectedIndex, static_cast<int>(DanmuItemModel::Columns::DANMUPOOL), QModelIndex()),
                                        danmuPools.value(poolIndex),Qt::EditRole);
            }
        }
    });

    selectedDanmuView->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    selectedDanmuView->addAction(actView);
    selectedDanmuView->addAction(actAutoSetPoolID);
    return selectedPage;
}

void AddDanmu::beginProcrss()
{
    processCounter++;
    showBusyState(true);
    searchButton->setEnabled(false);
    relWordWidget->setEnabled(false);
    keywordEdit->setEnabled(false);
}

void AddDanmu::endProcess()
{
    processCounter--;
    if(processCounter==0)
    {
        searchButton->setEnabled(true);
        keywordEdit->setEnabled(true);
        relWordWidget->setEnabled(true);
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
    relCache->save();
    CFramelessDialog::onAccept();
}

void AddDanmu::onClose()
{
    for(auto iter=selectedDanmuList.begin();iter!=selectedDanmuList.end();++iter)
    {
        qDeleteAll((*iter).second);
    }
    relCache->save();
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

SearchItemWidget::SearchItemWidget(const DanmuSource &item):source(item)
{
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QLabel *titleLabel=new QLabel(this);
	QLabel *descLabel = new QLabel(this);
    titleLabel->setToolTip(item.title);
	titleLabel->adjustSize();
    titleLabel->setText(QString("<font size=\"5\" face=\"Microsoft Yahei\" color=\"#f33aa0\">%1</font>").arg(item.title));
    titleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    descLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    descLabel->setText(QString("<font size=\"3\" face=\"Microsoft Yahei\">%1</font>").arg(item.desc.trimmed()));
    descLabel->setToolTip(item.desc);
    QPushButton *addItemButton=new QPushButton(tr("Add"),this);
    QObject::connect(addItemButton,&QPushButton::clicked,this,[this,addItemButton](){
		addItemButton->setEnabled(false);
        emit addSearchItem(this);
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
    danmuToPoolList(&dmDialog->danmuToPoolList),danmuCheckedList(&dmDialog->danmuCheckedList),selectedDanmuList(&dmDialog->selectedDanmuList),
    hasPoolInfo(hasPool),nPool(normalPool)
{

}

void DanmuItemModel::addItem(const DanmuSource &src)
{
    beginInsertRows(QModelIndex(),items.count(),items.count());
    items.append({src.title, src.durationStr(), GlobalObjects::scriptManager->getScript(src.scriptId)->name(), src.count});
    danmuCheckedList->append(true);
    danmuToPoolList->append(nPool);
    endInsertRows();
}

QVariant DanmuItemModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    const ItemInfo &info=items.at(index.row());
    Columns col=static_cast<Columns>(index.column());
    if(role==Qt::DisplayRole || role==Qt::ToolTipRole)
    {
        switch (col)
        {
        case Columns::TITLE:
            return info.title;
        case Columns::COUNT:
            return info.count;
        case Columns::PROVIDER:
            return info.provider;
        case Columns::DURATION:
            return info.duration;
        case Columns::DANMUPOOL:
            return danmuToPoolList->at(index.row());
        }
    }
    else if(role==Qt::CheckStateRole)
    {
        if(col==Columns::TITLE)
            return danmuCheckedList->at(index.row())?Qt::Checked:Qt::Unchecked;
    }
    return QVariant();
}

bool DanmuItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row=index.row();
    Columns col=static_cast<Columns>(index.column());
    switch (col)
    {
    case Columns::TITLE:
        (*danmuCheckedList)[row]=(value==Qt::Checked);
        break;
    case Columns::DANMUPOOL:
        (*danmuToPoolList)[row]=value.toString();
        break;
    default:
        return false;
    }
    return true;
}

QVariant DanmuItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers={tr("Title"),tr("DanmuCount"),tr("Source"),tr("Duration"),tr("DanmuPool")};
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size())return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags DanmuItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if(!index.isValid()) return defaultFlags;
    Columns col = static_cast<Columns>(index.column());
    if(col==Columns::TITLE)
        defaultFlags |= Qt::ItemIsUserCheckable;
    else if(col==Columns::DANMUPOOL)
        defaultFlags |= Qt::ItemIsEditable;

    return defaultFlags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

QMimeData *DanmuItemModel::mimeData(const QModelIndexList &indexes) const
{
    int sr = indexes.first().row();
    QMimeData *mimeData = new QMimeData();
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds<<sr;
    mimeData->setData("application/x-kikoplayitem", data);
    return mimeData;
}

bool DanmuItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!data->hasFormat("application/x-kikoplayitem")) return false;
    if (action == Qt::IgnoreAction) return true;
    QByteArray encodedData = data->data("application/x-kikoplayitem");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int sr = -1;
    if(!stream.atEnd()) stream>>sr;
    if(sr == -1) return false;
    int dr = row==-1?parent.row():row;

    beginMoveRows(QModelIndex(),sr,sr,QModelIndex(),dr>sr?dr+1:dr);
    items.move(sr, dr);
    danmuToPoolList->move(sr, dr);
    danmuCheckedList->move(sr, dr);
    selectedDanmuList->move(sr, dr);
    endMoveRows();
    return true;
}

QWidget *PoolComboDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.column()==static_cast<int>(DanmuItemModel::Columns::DANMUPOOL))
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
    if(index.column()==static_cast<int>(DanmuItemModel::Columns::DANMUPOOL))
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        combo->setCurrentIndex(poolList.indexOf(index.data(Qt::DisplayRole).toString()));
        return;
    }
    QStyledItemDelegate::setEditorData(editor,index);
}

void PoolComboDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if(index.column()==static_cast<int>(DanmuItemModel::Columns::DANMUPOOL))
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        model->setData(index,poolList.value(combo->currentIndex()),Qt::EditRole);
        return;
    }
    QStyledItemDelegate::setModelData(editor,model,index);
}

RelWordWidget::RelWordWidget(QWidget *parent):QWidget(parent)
{
    QHBoxLayout *relHLayout = new QHBoxLayout(this);
    relHLayout->setContentsMargins(0,0,0,0);
    relHLayout->addStretch(1);
    setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
}

void RelWordWidget::setRelWordList(const QStringList &relWords)
{
    qDeleteAll(relBtns);
    for(auto &rel:relWords)
    {
        QPushButton *relBtn=new QPushButton(rel, this);
        relBtn->setObjectName(QStringLiteral("LinkFlatButton"));
        QObject::connect(relBtn, &QPushButton::clicked, this, [relBtn, this](){
           emit relWordClicked(relBtn->text());
        });
        relBtns<<relBtn;
        static_cast<QHBoxLayout *>(layout())->insertWidget(0,relBtn);
    }
    if(relBtns.isEmpty()) hide();
    else show();
}

QSize RelWordWidget::sizeHint() const
{
    return layout()->sizeHint();
}

#include "matcheditor.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QButtonGroup>
#include <QAction>
#include <QComboBox>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
#include "Common/kcache.h"
#include "globalobjects.h"
namespace
{
    QSet<QString> hitWords;
    static QCollator comparer;
}
MatchEditor::MatchEditor(const PlayListItem *item, QList<const PlayListItem *> *batchItems,  QWidget *parent) :
    CFramelessDialog(tr("Edit Match"),parent,true), curItem(item)
{
    comparer.setNumericMode(true);
	this->batchItems = batchItems;
    QVBoxLayout *matchVLayout=new QVBoxLayout(this);
    matchVLayout->setContentsMargins(0,0,0,0);
    matchVLayout->setSpacing(0);

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

    batchPage=new QToolButton(this);
    batchPage->setText(tr("Batch"));
    batchPage->setCheckable(true);
    batchPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    batchPage->setFixedSize(pageButtonSize);
    batchPage->setObjectName(QStringLiteral("DialogPageButton"));

    QButtonGroup *btnGroup=new QButtonGroup(this);
    btnGroup->addButton(searchPage,0);
    btnGroup->addButton(customPage,1);
    btnGroup->addButton(batchPage, 2);
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
    pageButtonHLayout->addWidget(batchPage);
    pageButtonHLayout->addStretch(1);
    matchVLayout->addLayout(pageButtonHLayout);
	if (!batchItems) batchPage->hide();


    contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupSearchPage());
    contentStackLayout->addWidget(setupCustomPage());
    contentStackLayout->addWidget(setupBatchPage());
    matchVLayout->addLayout(contentStackLayout);

    QString matchStr=item->poolID.isEmpty()?tr("No Match Info"):QString("%1-%2").arg(item->animeTitle).arg(item->title);
    QLabel *matchInfoLabel=new QLabel(matchStr,this);
    matchInfoLabel->setFont(QFont(GlobalObjects::normalFont,10,QFont::Bold));
    matchInfoLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    matchVLayout->addWidget(matchInfoLabel);
    animeEdit->setText(item->animeTitle);
    subtitleEdit->setText(item->title);
    this->matchInfo=nullptr;

    resize(GlobalObjects::appSetting->value("DialogSize/MatchEditor",QSize(400*logicalDpiX()/96,400*logicalDpiY()/96)).toSize());
    hitWords.clear();

}

MatchEditor::~MatchEditor()
{
    if(matchInfo)delete matchInfo;
}

QWidget *MatchEditor::setupSearchPage()
{
    QFont normalFont(GlobalObjects::normalFont,10);
    QWidget *searchPage=new QWidget(this);
    searchPage->setFont(normalFont);

    QComboBox *searchLocationCombo = new QComboBox(searchPage);
    searchLocationCombo->addItems({tr("DanDanPlay"), tr("Bangumi"), tr("Local DB")});
    searchLocationCombo->setCurrentIndex(1);

    QLineEdit *keywordEdit=new QLineEdit(searchPage);

    QPushButton *searchButton=new QPushButton(tr("Search"),searchPage);
    searchButton->setDefault(false);
    searchButton->setAutoDefault(false);
    QObject::connect(searchButton,&QPushButton::clicked,this,[=](){
        QString keyword=keywordEdit->text().trimmed();
        if(keyword.isEmpty())return;
        if(!hitWords.contains(keyword))
        {
            QScopedPointer<MatchInfo> match(KCache::getInstance()->get<MatchInfo>(QString("matcheditor/match_s_%1_%2").arg(searchLocationCombo->currentIndex()).arg(keyword)));
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
        searchButton->setEnabled(false);
        keywordEdit->setEnabled(false);
        customPage->setEnabled(false);
        batchPage->setEnabled(false);
        searchButton->setText(tr("Searching"));
        showBusyState(true);
        QScopedPointer<MatchInfo> sInfo(GlobalObjects::danmuManager->searchMatch(DanmuManager::MatchProvider(searchLocationCombo->currentIndex()),keyword));
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
                KCache::getInstance()->put(QString("matcheditor/match_s_%1_%2").arg(searchLocationCombo->currentIndex()).arg(keyword), *sInfo);
                hitWords.remove(keyword);
            }
        }
        showBusyState(false);
        searchButton->setEnabled(true);
        keywordEdit->setEnabled(true);
        customPage->setEnabled(true);
        batchPage->setEnabled(true);
        searchButton->setText(tr("Search"));
    });
    QObject::connect(keywordEdit, &QLineEdit::returnPressed, searchButton, &QPushButton::click);

    searchResult=new QTreeWidget(searchPage);
    searchResult->setRootIsDecorated(false);
    searchResult->setFont(searchPage->font());
    searchResult->setHeaderLabels(QStringList()<<tr("Anime Title")<<tr("Subtitle"));
    searchResult->setContextMenuPolicy(Qt::ActionsContextMenu);

    keywordEdit->setText(curItem->animeTitle.isEmpty()?curItem->title:curItem->animeTitle);

    if(!curItem->animeTitle.isEmpty())
    {
        QScopedPointer<MatchInfo> match(KCache::getInstance()->get<MatchInfo>(QString("matcheditor/match_s_%1_%2").arg(searchLocationCombo->currentIndex()).arg(curItem->animeTitle)));
        if(match)
        {
            for(MatchInfo::DetailInfo &detailInfo:match->matches)
            {
                new QTreeWidgetItem(searchResult,QStringList()<<detailInfo.animeTitle<<detailInfo.title);
            }
        }
    }
    QAction *copy=new QAction(tr("Copy"), this);
    QObject::connect(copy, &QAction::triggered, this, [this](){
        auto selectedItems=searchResult->selectedItems();
        if(selectedItems.count()==0) return;
        QTreeWidgetItem *selectedItem=selectedItems.first();
        QClipboard *cb = QApplication::clipboard();
        cb->setText(QString("%1 %2").arg(selectedItem->data(0,0).toString(), selectedItem->data(1,0).toString()));
    });
    searchResult->addAction(copy);

    QGridLayout *searchPageGLayout=new QGridLayout(searchPage);
    searchPageGLayout->addWidget(searchLocationCombo, 0, 0);
    searchPageGLayout->addWidget(keywordEdit,0,1);
    searchPageGLayout->addWidget(searchButton,0,2);
    searchPageGLayout->addWidget(searchResult,1,0,1,3);
    searchPageGLayout->setRowStretch(1,1);
    searchPageGLayout->setColumnStretch(1,1);
    return searchPage;
}

QWidget *MatchEditor::setupCustomPage()
{
    QWidget *customPage=new QWidget(this);
    QFont normalFont(GlobalObjects::normalFont,10);
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
    customVLayout->addStretch(1);

    return customPage;
}

QWidget *MatchEditor::setupBatchPage()
{
    QWidget *pageContainer = new QWidget(this);
    QFont normalFont(GlobalObjects::normalFont,10);
    pageContainer->setFont(normalFont);

    QWidget *searchSubPage = new QWidget(pageContainer);
    QLineEdit *searchKeyword = new QLineEdit(searchSubPage);
    QPushButton *searchBtn = new QPushButton(tr("Search"), searchSubPage);
    QTreeWidget *animeList = new QTreeWidget(searchSubPage);
    QGridLayout *searchSubGLayout = new QGridLayout(searchSubPage);
    searchSubGLayout->addWidget(searchKeyword, 0, 0);
    searchSubGLayout->addWidget(searchBtn, 0, 1);
    searchSubGLayout->addWidget(animeList, 1, 0, 1, 2);
    searchSubGLayout->setRowStretch(1, 1);
    searchSubGLayout->setColumnStretch(0, 1);

    animeList->setRootIsDecorated(false);
    animeList->setHeaderLabels({tr("Bangumi ID"), tr("Anime Title")});
    searchKeyword->setText(curItem->animeTitle.isEmpty()?curItem->title:curItem->animeTitle);
    QObject::connect(searchBtn, &QPushButton::clicked, this, [=](){
        QString keyword=searchKeyword->text().trimmed();
        if(keyword.isEmpty())return;
        if(!hitWords.contains(keyword+"_b"))
        {
            QScopedPointer<QList<Bangumi::BangumiInfo>> animes(KCache::getInstance()->get<QList<Bangumi::BangumiInfo>>(QString("matcheditor/batch_%1").arg(keyword)));
            if(animes)
            {
                animeList->clear();
                for(auto &bgm : *animes)
                {
                    new QTreeWidgetItem(animeList, {QString::number(bgm.bgmID), bgm.name_cn.isEmpty()?bgm.name:bgm.name_cn});
                }
                hitWords<<keyword+"_b";
                return;
            }
        }
        searchBtn->setEnabled(false);
        searchKeyword->setEnabled(false);
        customPage->setEnabled(false);
        searchPage->setEnabled(false);
        searchBtn->setText(tr("Searching"));
        showBusyState(true);

        QList<Bangumi::BangumiInfo> bgms;
        QString err(Bangumi::animeSearch(keyword, bgms));
        if(err.isEmpty())
        {
            animeList->clear();
            for(auto &bgm : bgms)
            {
                new QTreeWidgetItem(animeList, {QString::number(bgm.bgmID), bgm.name_cn.isEmpty()?bgm.name:bgm.name_cn});
            }
            KCache::getInstance()->put(QString("matcheditor/batch_%1").arg(keyword), bgms);
            hitWords.remove(keyword+"_b");
        }
        else
        {
            showMessage(err, 1);
        }
        showBusyState(false);
        searchBtn->setEnabled(true);
        searchKeyword->setEnabled(true);
        customPage->setEnabled(true);
        searchPage->setEnabled(true);
        searchBtn->setText(tr("Search"));
    });

    QWidget *matchSubPage = new QWidget(pageContainer);
    QPushButton *backBtn = new QPushButton(tr("Back"), matchSubPage);
    QLabel *animeLabel = new QLabel(matchSubPage);
    QTreeView *matchView = new QTreeView(matchSubPage);
    EpModel *epModel = new EpModel(this, this);
    matchView->setModel(epModel);
    EpComboDelegate *epDelegate = new EpComboDelegate(this);
    matchView->setItemDelegate(epDelegate);

    QGridLayout *matchSubGLayout = new QGridLayout(matchSubPage);
    matchSubGLayout->addWidget(backBtn, 0, 0);
    matchSubGLayout->addWidget(animeLabel, 0, 2, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    matchSubGLayout->addWidget(matchView, 1, 0, 1, 3);
    matchSubGLayout->setRowStretch(1, 1);
    matchSubGLayout->setColumnStretch(1, 1);

    QStackedLayout *batchSLayout = new QStackedLayout(pageContainer);
    batchSLayout->addWidget(searchSubPage);
    batchSLayout->addWidget(matchSubPage);

    QObject::connect(animeList, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem *item){
       int bgmId = item->text(0).toInt();
       QString animeTitle = item->text(1);
       QScopedPointer<QList<Bangumi::EpInfo>> epCache(KCache::getInstance()->get<QList<Bangumi::EpInfo>>(QString("matcheditor/batch_ep_%1").arg(animeTitle)));
       if(!epCache)
       {
           QList<Bangumi::EpInfo> eps;
           customPage->setEnabled(false);
           searchPage->setEnabled(false);
           matchView->setEnabled(false);
           searchBtn->setEnabled(false);
           searchKeyword->setEnabled(false);
           showBusyState(true);
           QString err(Bangumi::getEp(bgmId, eps));
           if(!err.isEmpty())
           {
               showMessage(err, 1);
           }
           else
           {
               std::sort(eps.begin(), eps.end(), [](const Bangumi::EpInfo &ep1, const Bangumi::EpInfo &ep2){
                  return ep1.type==ep2.type?(ep1.index<ep2.index):(ep1.type<ep2.type);
               });
               epCache.reset(new QList<Bangumi::EpInfo>(eps));
               KCache::getInstance()->put(QString("matcheditor/batch_ep_%1").arg(animeTitle), *epCache);
           }
           customPage->setEnabled(true);
           searchPage->setEnabled(true);
           matchView->setEnabled(true);
           searchBtn->setEnabled(true);
           searchKeyword->setEnabled(true);
           showBusyState(false);
           if(!err.isEmpty()) return;
       }
       animeEps.clear();
       for(auto &ep: *epCache)
       {
           animeEps.append(ep.toString());
       }
       epModel->resetEpList(*epCache);
       epDelegate->setEpList(*epCache);
       batchAnime = animeTitle;
       animeLabel->setText(batchAnime);
       batchSLayout->setCurrentIndex(1);
    });

    QObject::connect(backBtn, &QPushButton::clicked, this, [=](){
       batchSLayout->setCurrentIndex(0);
       batchAnime = "";
    });

    QAction *actAutoSetEp=new QAction(tr("Set Ep Sequentially From Current"),this);
    QObject::connect(actAutoSetEp,&QAction::triggered,this,[=](){
        if(animeEps.size()<=1) return;
        auto selection = matchView->selectionModel()->selectedRows();
        if (selection.size() == 0) return;
        int selectedIndex=selection.first().row();
        int epIndex = animeEps.indexOf(batchEp[selectedIndex]);
        if(epIndex < 0) return;
        for(++selectedIndex; selectedIndex<batchEp.size();++selectedIndex)
        {
            if(epCheckedList[selectedIndex])
            {
                if(++epIndex >= animeEps.size()) break;
                epModel->setData(epModel->index(selectedIndex, static_cast<int>(EpModel::Columns::EPNAME), QModelIndex()),
                                        animeEps.value(epIndex),Qt::EditRole);
            }
        }
    });
    QAction *actSelectAll=new QAction(tr("Select All/Cancel"),this);
    QObject::connect(actSelectAll,&QAction::triggered,epModel, &EpModel::toggleCheckAll);

    matchView->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    matchView->addAction(actSelectAll);
    matchView->addAction(actAutoSetEp);
    QHeaderView *matchHeader = matchView->header();
    matchHeader->resizeSections(QHeaderView::ResizeToContents);

    return pageContainer;
}

void MatchEditor::onAccept()
{
    QString animeTitle,title;
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
        title=selectedItem->data(1,0).toString();
    }
    else if(customPage->isChecked())
    {
        animeTitle=animeEdit->text().trimmed();
        title=subtitleEdit->text().trimmed();
        if(animeTitle.isEmpty()||title.isEmpty())
        {
            showMessage(tr("AnimeTitle and Subtitle can't be empty"),1);
            return;
        }
    }
    else if(batchPage->isChecked())
    {
        if(batchAnime.isEmpty())
        {
            showMessage(tr("Anime can't be empty"),1);
            return;
        }
    }
    if(!batchPage->isChecked())
    {
        batchAnime = "";
        matchInfo=new MatchInfo;
        matchInfo->error=false;
        MatchInfo::DetailInfo detailInfo;
        detailInfo.animeTitle=animeTitle;
        detailInfo.title=title;
        matchInfo->matches.append(detailInfo);
    }
    GlobalObjects::appSetting->setValue("DialogSize/MatchEditor",size());
    CFramelessDialog::onAccept();
}

void MatchEditor::onClose()
{
    GlobalObjects::appSetting->setValue("DialogSize/MatchEditor",size());
    CFramelessDialog::onClose();
}

QWidget *EpComboDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.column()==static_cast<int>(EpModel::Columns::EPNAME))
    {
        QComboBox *combo=new QComboBox(parent);
        combo->setFrame(false);
        int lastType = -1;
        static QStringList epTypes({tr("EP"), tr("SP"),  tr("OP"), tr("ED"), tr("Trailer"), tr("MAD"), tr("Other")});
        for(auto &ep : epList)
        {
            if(static_cast<int>(ep.type)!=lastType)
            {
                addParentItem(combo, epTypes[static_cast<int>(ep.type)]);
            }
            addChildItem(combo, ep.toString());
            lastType = static_cast<int>(ep.type);
        }
        combo->view()->setItemDelegate(new EpComboItemDelegate(combo));
        combo->setEditable(true);
        return combo;
    }
    return QStyledItemDelegate::createEditor(parent,option,index);
}

void EpComboDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if(index.column()==static_cast<int>(EpModel::Columns::EPNAME))
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        combo->setCurrentText(index.data(Qt::DisplayRole).toString());
        return;
    }
    QStyledItemDelegate::setEditorData(editor,index);
}

void EpComboDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if(index.column()==static_cast<int>(EpModel::Columns::EPNAME))
    {
        QComboBox *combo = static_cast<QComboBox*>(editor);
        model->setData(index,combo->currentText(),Qt::EditRole);
        return;
    }
    QStyledItemDelegate::setModelData(editor,model,index);
}

void EpComboDelegate::addParentItem(QComboBox *combo, const QString &text) const
{
    QStandardItem* item = new QStandardItem(text);
    item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
    item->setData( "parent", Qt::AccessibleDescriptionRole );
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
    itemModel->appendRow(item);
}

void EpComboDelegate::addChildItem(QComboBox *combo, const QString &text) const
{
    QStandardItem* item = new QStandardItem(text);
    item->setData("child", Qt::AccessibleDescriptionRole );
    QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
    itemModel->appendRow(item);
}

EpModel::EpModel(MatchEditor *matchEditor, QObject *parent) : QAbstractItemModel(parent)
{
    batchEp = &matchEditor->batchEp;
    epCheckedList = &matchEditor->epCheckedList;
    if(matchEditor->batchItems)
    {
        QHash<const PlayListItem *, QString> titleHash;
        for(auto i : *matchEditor->batchItems)
        {
            int pathPos = i->path.lastIndexOf('/') + 1;
            titleHash[i] = i->path.mid(pathPos);
        }
        std::sort(matchEditor->batchItems->begin(), matchEditor->batchItems->end(),
                  [&](const PlayListItem *p1, const PlayListItem *p2){
            return comparer.compare(titleHash[p1], titleHash[p2])>=0?false:true;
        });
        for(auto i : *matchEditor->batchItems)
        {
            fileTitles.append(titleHash[i]);
            batchEp->append("");
            epCheckedList->append(true);
        }
    }
}

void EpModel::resetEpList(const QList<Bangumi::EpInfo> &eps)
{
    beginResetModel();
    int pEp = -1;
    QString epLast;
    for(int i=0; i<fileTitles.size(); ++i)
    {
        if((*epCheckedList)[i])
        {
            if(++pEp<eps.size())
            {
                epLast = eps[pEp].toString();
            }
            (*batchEp)[i] = epLast;
        }
    }
    endResetModel();
}

void EpModel::toggleCheckAll()
{
    if(epCheckedList->size()==0) return;
    bool state = !(*epCheckedList)[0];
    beginResetModel();
    for(auto &b : *epCheckedList)
    {
        b = state;
    }
    endResetModel();
}

QVariant EpModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    int row = index.row();
    Columns col=static_cast<Columns>(index.column());
    if(role==Qt::DisplayRole)
    {
        switch (col)
        {
        case Columns::FILETITLE:
            return fileTitles[row];
        case Columns::EPNAME:
            return (*batchEp)[row];
        }
    }
    else if(role==Qt::CheckStateRole)
    {
        if(col==Columns::FILETITLE)
            return epCheckedList->at(index.row())?Qt::Checked:Qt::Unchecked;
    }
    return QVariant();
}

bool EpModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row=index.row();
    Columns col=static_cast<Columns>(index.column());
    switch (col)
    {
    case Columns::FILETITLE:
        (*epCheckedList)[row]=(value==Qt::Checked);
        break;
    case Columns::EPNAME:
        (*batchEp)[row]=value.toString();
        break;
    default:
        return false;
    }
    emit dataChanged(index,index);
    return true;
}

QVariant EpModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size())return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags EpModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if(!index.isValid()) return defaultFlags;
    Columns col = static_cast<Columns>(index.column());
    if(col==Columns::FILETITLE)
        defaultFlags |= Qt::ItemIsUserCheckable;
    else if(col==Columns::EPNAME)
        defaultFlags |= Qt::ItemIsEditable;
    return defaultFlags;
}

void EpComboItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString type = index.data( Qt::AccessibleDescriptionRole ).toString();
    if ( type == QLatin1String("child"))
    {
        QStyleOptionViewItem childOption = option;
        int indent = option.fontMetrics.horizontalAdvance(QString(4, QChar(' ')));
        childOption.rect.adjust(indent, 0, 0, 0);
        childOption.textElideMode = Qt::ElideNone;
        QStyledItemDelegate::paint(painter, childOption, index);
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

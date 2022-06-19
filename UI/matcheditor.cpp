#include "matcheditor.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "MediaLibrary/animeprovider.h"
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
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
#include "Common/lrucache.h"
#include "Common/notifier.h"
#include "globalobjects.h"
#define AnimeRole Qt::UserRole+1
#define EpRole Qt::UserRole+2
namespace
{
    QHash<QString, int> hitWords;
    static QCollator comparer;
    QString lastSearchCacheId;
    LRUCache<QString, QList<AnimeLite>> animeCache{"MatchAnime", 64};

    class EpComboItemDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            {
                QString type = index.data(Qt::AccessibleDescriptionRole ).toString();
                if (type == QLatin1String("child"))
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
        }
    };

    class EpModel : public QAbstractItemModel
    {
    public:
        EpModel(MatchEditor *matchEditor, QObject *parent = nullptr) : QAbstractItemModel(parent)
        {
            epList = &matchEditor->epList;
            epCheckedList = &matchEditor->epCheckedList;

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
                EpInfo ep;
                ep.localFile = i->path;
                epList->append(ep);
                epCheckedList->append(i==matchEditor->curItem);
            }

        }
        void reset(const AnimeLite &anime)
        {
            this->anime = anime;
            beginResetModel();
            int pEp = -1;
            EpInfo epLast;
            auto &eps = *anime.epList;
            for(int i=0; i<fileTitles.size(); ++i)
            {
                if(++pEp<eps.size())
                {
                    epLast = eps[pEp];
                }
                (*epList)[i].infoAssign(epLast);
            }
            endResetModel();
        }
        void toggleCheckedState()
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
        void seqSetEpisode(int startIndex)
        {
            if(!anime.epList || anime.epList->size()<=1 || startIndex>=epList->size()) return;

            int epIndex = anime.epList->indexOf(epList->value(startIndex));
            if(epIndex < 0) return;
            for(++startIndex; startIndex<epList->size();++startIndex)
            {
                if((*epCheckedList)[startIndex])
                {
                    if(++epIndex >= anime.epList->size()) break;
                    (*epList)[startIndex].infoAssign(anime.epList->value(epIndex));
                    QModelIndex modelIndex(this->index(startIndex, (int)EpModel::Columns::EPNAME, QModelIndex()));
                    emit dataChanged(modelIndex,modelIndex);
                }
            }
        }
        enum class Columns
        {
            FILETITLE,
            EPNAME
        };

    private:
        QStringList fileTitles;
        QList<EpInfo> *epList;
        AnimeLite anime;
        QList<bool> *epCheckedList;
        QStringList headers={QObject::tr("FileTitle"),QObject::tr("EpName")};
    public:
        inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
        inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
        inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:fileTitles.count();}
        inline virtual int columnCount(const QModelIndex &) const{return headers.size();}
        virtual QVariant data(const QModelIndex &index, int role) const
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
                    return (*epList)[row].toString();
                }
            }
            else if(role==Qt::CheckStateRole)
            {
                if(col==Columns::FILETITLE)
                    return epCheckedList->at(index.row())?Qt::Checked:Qt::Unchecked;
            }
            else if(role==EpRole)
            {
                return QVariant::fromValue<EpInfo>((*epList)[row]);
            }
            return QVariant();
        }
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role)
        {
            int row=index.row();
            Columns col=static_cast<Columns>(index.column());
            switch (col)
            {
            case Columns::FILETITLE:
                (*epCheckedList)[row]=(value==Qt::Checked);
                break;
            case Columns::EPNAME:
                (*epList)[row].infoAssign(value.value<EpInfo>());
                break;
            default:
                return false;
            }
            return true;
        }
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
            {
                if(section<headers.size())return headers.at(section);
            }
            return QVariant();
        }
        virtual Qt::ItemFlags flags(const QModelIndex &index) const
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
    };

    class EpComboDelegate : public QStyledItemDelegate
    {
    public:
        EpComboDelegate(QObject *parent = nullptr):QStyledItemDelegate(parent){}
        void setEpList(const QVector<EpInfo> &eps) {epList = eps;}

        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            if(index.column()==static_cast<int>(EpModel::Columns::EPNAME))
            {
                QComboBox *combo=new QComboBox(parent);
                combo->setFrame(false);
                int lastType = -1;
                int index = 0;
                for(auto &ep : epList)
                {
                    if(static_cast<int>(ep.type)!=lastType)
                    {
                        addParentItem(combo, EpTypeName[static_cast<int>(ep.type)-1]);
                    }
                    addChildItem(combo, ep.toString(), index++);
                    lastType = static_cast<int>(ep.type);
                }
                combo->view()->setItemDelegate(new EpComboItemDelegate(combo));
                return combo;
            }
            return QStyledItemDelegate::createEditor(parent,option,index);
        }
        void setEditorData(QWidget *editor, const QModelIndex &index) const override
        {
            if(index.column()==static_cast<int>(EpModel::Columns::EPNAME))
            {
                QComboBox *combo = static_cast<QComboBox*>(editor);
                combo->setCurrentIndex(combo->findText(index.data(Qt::DisplayRole).toString()));
                return;
            }
            QStyledItemDelegate::setEditorData(editor,index);
        }
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
        {
            if(index.column()==static_cast<int>(EpModel::Columns::EPNAME))
            {
                QComboBox *combo = static_cast<QComboBox*>(editor);
				int epIndex = combo->currentData(EpRole).toInt();
				if (epList.size() == 0 || epIndex<0 || epIndex>epList.size()) return;
                model->setData(index, QVariant::fromValue(epList[epIndex]), Qt::EditRole);
                return;
            }
            QStyledItemDelegate::setModelData(editor,model,index);
        }
        void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
        {
            editor->setGeometry(option.rect);
        }
    private:
        QVector<EpInfo> epList;
        void addParentItem(QComboBox *combo, const QString& text) const
        {
            QStandardItem* item = new QStandardItem(text);
            item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
            item->setData("parent", Qt::AccessibleDescriptionRole);
            QFont font = item->font();
            font.setItalic(true);
            item->setFont(font);
            QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
            itemModel->appendRow(item);
        }
        void addChildItem(QComboBox *combo, const QString& text, int epListIndex) const
        {
            QStandardItem* item = new QStandardItem(text);
            item->setData("child", Qt::AccessibleDescriptionRole );
            item->setData(epListIndex, EpRole);
            QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
            itemModel->appendRow(item);
        }
    };

    class AnimeModel : public QAbstractItemModel
    {
    public:
        AnimeModel(QObject *parent = nullptr) : QAbstractItemModel(parent) {}
        void reset(const QList<AnimeLite> &nList)
        {
            beginResetModel();
            animeBaseList = nList;
            endResetModel();
        }
        void fillEpInfo(const QModelIndex &index, const QVector<EpInfo> &epList)
        {
            if(!index.isValid()) return;
            animeBaseList[index.row()].epList.reset(new QVector<EpInfo>(epList));
        }
        const QList<AnimeLite> animeBases() const {return animeBaseList;}
        enum class Columns
        {
            ANIME, DESC
        };

    private:
        QStringList headers={QObject::tr("Anime"), QObject::tr("Descriptiopn")};
        QList<AnimeLite> animeBaseList;
    public:
        inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
        inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
        inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:animeBaseList.count();}
        inline virtual int columnCount(const QModelIndex &) const{return headers.size();}
        virtual QVariant data(const QModelIndex &index, int role) const
        {
            if(!index.isValid()) return QVariant();
            int row = index.row();
            Columns col=static_cast<Columns>(index.column());
            if(role==Qt::DisplayRole || role==Qt::ToolTipRole)
            {
                switch (col)
                {
                case Columns::ANIME:
                    return animeBaseList[row].name;
                case Columns::DESC:
                    return animeBaseList[row].extras;
                }
            }
            else if(role==AnimeRole)
            {
                return QVariant::fromValue<AnimeLite>(animeBaseList[row]);
            }
            return QVariant();
        }
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
            {
                if(section<headers.size())return headers.at(section);
            }
            return QVariant();
        }
    };
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

    searchPage=new QToolButton(this);
    searchPage->setText(tr("Search"));
    searchPage->setCheckable(true);
    searchPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    searchPage->setObjectName(QStringLiteral("DialogPageButton"));
    searchPage->setChecked(true);

    customPage=new QToolButton(this);
    customPage->setText(tr("Custom"));
    customPage->setCheckable(true);
    customPage->setToolButtonStyle(Qt::ToolButtonTextOnly);
    customPage->setObjectName(QStringLiteral("DialogPageButton"));

    QFontMetrics fm(fontMetrics());
    int btnH = fm.height() + 10 * logicalDpiY()/96;
    int btnW = qMax(fm.horizontalAdvance(customPage->text()), fm.horizontalAdvance(searchPage->text()));
    QSize pageButtonSize(btnW*2, btnH);
    searchPage->setFixedSize(pageButtonSize);
    customPage->setFixedSize(pageButtonSize);

    QString animeTitle;
    EpInfo ep;
    Pool *pool = GlobalObjects::danmuManager->getPool(item->poolID, false);
    if(pool)
    {
        ep = pool->toEp();
        animeTitle = pool->animeTitle();
    }

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    pageButtonHLayout->addWidget(searchPage);
    pageButtonHLayout->addWidget(customPage);
    pageButtonHLayout->addStretch(1);
    matchVLayout->addLayout(pageButtonHLayout);

    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupSearchPage(animeTitle));
    contentStackLayout->addWidget(setupCustomPage(animeTitle, ep));
    matchVLayout->addLayout(contentStackLayout);

    QButtonGroup *btnGroup=new QButtonGroup(this);
    btnGroup->addButton(searchPage,0);
    btnGroup->addButton(customPage,1);
    QObject::connect(btnGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[=](int id, bool checked){
        if(checked)
        {
            contentStackLayout->setCurrentIndex(id);
        }
    });

    QLabel *matchInfoLabel=new QLabel(ep.type==EpType::UNKNOWN?tr("No Match Info"):ep.toString(), this);
    matchInfoLabel->setFont(QFont(GlobalObjects::normalFont,10,QFont::Bold));
    matchInfoLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Minimum);
    matchVLayout->addWidget(matchInfoLabel);

    setSizeSettingKey("DialogSize/MatchEditor",QSize(400*logicalDpiX()/96,400*logicalDpiY()/96));
    hitWords.clear();
}


QWidget *MatchEditor::setupCustomPage(const QString &srcAnime, const EpInfo &ep)
{
    QWidget *customPage=new QWidget(this);
    QFont normalFont(GlobalObjects::normalFont,10);
    customPage->setFont(normalFont);

    QLabel *animeTip=new QLabel(tr("Anime Title"),customPage);
    animeEdit=new QLineEdit(srcAnime, customPage);

    QLabel *epTypeTip=new QLabel(tr("Episode Type"),customPage);
    epTypeCombo=new QComboBox(customPage);
    epTypeCombo->addItems(QStringList(std::begin(EpTypeName), std::end(EpTypeName)));

    QLabel *epIndexTip=new QLabel(tr("Episode Index"),customPage);
    epIndexEdit=new QLineEdit(customPage);
    epIndexEdit->setValidator(new QRegExpValidator(QRegExp("\\d+\\.?(\\d+)?"),epIndexEdit));

    QLabel *epTitleTip=new QLabel(tr("Episode Title"),customPage);
    epEdit=new QLineEdit(customPage);

    if(ep.type!=EpType::UNKNOWN)
    {
        epTypeCombo->setCurrentIndex(ep.type-1);
        epIndexEdit->setText(QString::number(ep.index));
        epEdit->setText(ep.name);
    } else {
        epEdit->setText(curItem->title);
    }

    QGridLayout *customGLayout=new QGridLayout(customPage);
    customGLayout->addWidget(animeTip, 0, 0, 1, 2);
    customGLayout->addWidget(animeEdit, 1, 0, 1, 2);
    customGLayout->addWidget(epTypeTip, 2, 0);
    customGLayout->addWidget(epTypeCombo, 3, 0);
    customGLayout->addWidget(epIndexTip, 2, 1);
    customGLayout->addWidget(epIndexEdit, 3, 1);
    customGLayout->addWidget(epTitleTip, 4, 0, 1, 2);
    customGLayout->addWidget(epEdit, 5, 0, 1, 2);

    customGLayout->setColumnStretch(0, 1);
    customGLayout->setColumnStretch(1, 1);
    customGLayout->setRowStretch(6, 1);

    return customPage;
}

QWidget *MatchEditor::setupSearchPage(const QString &srcAnime)
{
    QWidget *pageContainer = new QWidget(this);
    QFont normalFont(GlobalObjects::normalFont,10);
    pageContainer->setFont(normalFont);

    QWidget *searchSubPage = new QWidget(pageContainer);
    QComboBox *scriptCombo = new QComboBox(searchSubPage);
    QLineEdit *keywordEdit = new QLineEdit(srcAnime, searchSubPage);
    QPushButton *searchBtn = new QPushButton(tr("Search"), searchSubPage);
    QTreeView *animeView = new QTreeView(searchSubPage);
    QGridLayout *searchSubGLayout = new QGridLayout(searchSubPage);
    searchSubGLayout->addWidget(scriptCombo, 0, 0);
    searchSubGLayout->addWidget(keywordEdit, 0, 1);
    searchSubGLayout->addWidget(searchBtn, 0, 2);
    searchSubGLayout->addWidget(animeView, 1, 0, 1, 3);
    searchSubGLayout->setRowStretch(1, 1);
    searchSubGLayout->setColumnStretch(1, 1);

    for(const auto &s : GlobalObjects::animeProvider->getSearchProviders())
    {
        scriptCombo->addItem(s.first, s.second);
    }
    scriptCombo->addItem(tr("Local DB"), "Kiko.Local");

    animeModel = new AnimeModel(this);
    animeView->setRootIsDecorated(false);
    animeView->setSelectionMode(QAbstractItemView::SingleSelection);
    animeView->setModel(animeModel);
    animeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    animeView->header()->resizeSection(0, 200*logicalDpiX()/96);

    QAction *copy=new QAction(tr("Copy"), this);
    QObject::connect(copy, &QAction::triggered, this, [=](){
        auto selectedRows= animeView->selectionModel()->selectedRows((int)AnimeModel::Columns::ANIME);
        if(selectedRows.count()==0) return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(selectedRows.first().data().toString());
    });
    animeView->addAction(copy);

    if(!scriptCombo->currentData().isNull())
    {
        QString id(QString("%1/%2").arg(scriptCombo->currentData().toString(), srcAnime));
        if(animeCache.contains(id))
        {
            static_cast<AnimeModel *>(animeModel)->reset(animeCache.get(id));
        }
    }

    keywordEdit->setText(curItem->animeTitle.isEmpty()?curItem->title:curItem->animeTitle);
    QObject::connect(keywordEdit,&QLineEdit::returnPressed,searchBtn,&QPushButton::click);
    QObject::connect(searchBtn, &QPushButton::clicked, this, [=](){
        QString keyword=keywordEdit->text().trimmed();
        if(keyword.isEmpty())return;
        auto cacheId = QString("%1/%2").arg(scriptCombo->currentData().toString(), keyword);
        if(!hitWords.contains(keyword))
        {

            if(animeCache.contains(cacheId))
            {
                lastSearchCacheId = cacheId;
                static_cast<AnimeModel *>(animeModel)->reset(animeCache.get(cacheId));
                hitWords[keyword] = 3;
                return;
            }
        }
        searchSubPage->setEnabled(false);
        showBusyState(true);
        QList<AnimeLite> results;
        ScriptState state;
        if(scriptCombo->currentIndex()==scriptCombo->count()-1)
            GlobalObjects::danmuManager->localSearch(keyword, results);
        else
            state = GlobalObjects::animeProvider->animeSearch(scriptCombo->currentData().toString(), keyword, results);
        if(state)
        {
            if(!lastSearchCacheId.isEmpty())
                animeCache.put(lastSearchCacheId,  static_cast<AnimeModel *>(animeModel)->animeBases());
            lastSearchCacheId = cacheId;
            --hitWords[keyword];
            if(hitWords[keyword] <= 0)
                hitWords.remove(keyword);
            static_cast<AnimeModel *>(animeModel)->reset(results);
            animeCache.put(cacheId, results);
        } else {
            showMessage(state.info, NM_ERROR | NM_HIDE);
        }
        showBusyState(false);
        searchSubPage->setEnabled(true);
    });

    QWidget *matchSubPage = new QWidget(pageContainer);
    QPushButton *backBtn = new QPushButton(tr("Back"), matchSubPage);
    QLabel *animeLabel = new QLabel(matchSubPage);
    QTreeView *matchView = new QTreeView(matchSubPage);
    epModel = new EpModel(this, this);
    matchView->setModel(epModel);
    EpComboDelegate *epDelegate = new EpComboDelegate(this);
    matchView->setItemDelegate(epDelegate);

    QGridLayout *matchSubGLayout = new QGridLayout(matchSubPage);
    matchSubGLayout->addWidget(animeLabel, 0, 0);
    matchSubGLayout->addWidget(backBtn, 0, 1);
    matchSubGLayout->addWidget(matchView, 1, 0, 1, 2);
    matchSubGLayout->setRowStretch(1, 1);
    matchSubGLayout->setColumnStretch(0, 1);

    QStackedLayout *searchSLayout = new QStackedLayout(pageContainer);
    searchSLayout->addWidget(searchSubPage);
    searchSLayout->addWidget(matchSubPage);

    QObject::connect(animeView, &QAbstractItemView::clicked, this, [=](const QModelIndex &index){
        auto animeLite = animeModel->data(index, AnimeRole).value<AnimeLite>();
        if(animeLite.epList)
        {
            static_cast<EpModel *>(epModel)->reset(animeLite);
            this->anime = animeLite.name;
			epDelegate->setEpList(*animeLite.epList);
            animeLabel->setText(animeLite.name);
            searchSLayout->setCurrentIndex(1);
        }
        else
        {
            QScopedPointer<Anime> anime(animeLite.toAnime());
			QVector<EpInfo> results;
            searchSubPage->setEnabled(false);
            showBusyState(true);
            ScriptState state = GlobalObjects::animeProvider->getEp(anime.data(), results);
            searchSubPage->setEnabled(true);
            showBusyState(false);
            if(state)
            {
                static_cast<AnimeModel *>(animeModel)->fillEpInfo(index, results);
                auto anime = animeModel->data(index, AnimeRole).value<AnimeLite>();
                static_cast<EpModel *>(epModel)->reset(anime);
				epDelegate->setEpList(results);
                animeLabel->setText(anime.name);
                this->anime = animeLite.name;
                searchSLayout->setCurrentIndex(1);
            } else {
                showMessage(state.info, NM_ERROR | NM_HIDE);
            }
        }
    });

    QObject::connect(backBtn, &QPushButton::clicked, this, [=](){
       searchSLayout->setCurrentIndex(0);
       this->anime.clear();
    });

    QAction *actAutoSetEp=new QAction(tr("Set Episodes Sequentially From Current"),this);
    QObject::connect(actAutoSetEp,&QAction::triggered,this,[=](){
        if(epList.size()<=1) return;
        auto selection = matchView->selectionModel()->selectedRows();
        if (selection.size() == 0) return;
        static_cast<EpModel *>(epModel)->seqSetEpisode(selection.first().row());
    });
    QAction *actSelectAll=new QAction(tr("Select All/Cancel"),this);
    QObject::connect(actSelectAll,&QAction::triggered, static_cast<EpModel *>(epModel), &EpModel::toggleCheckedState);

    QAction *copyEp=new QAction(tr("Copy"), this);
    QObject::connect(copyEp, &QAction::triggered, this, [=](){
        auto selectedRows= matchView->selectionModel()->selectedRows((int)EpModel::Columns::EPNAME);
        if(selectedRows.count()==0) return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(selectedRows.first().data().toString());
    });

    matchView->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    matchView->addAction(actSelectAll);
    matchView->addAction(actAutoSetEp);
    matchView->addAction(copyEp);

    QVariant headerState(GlobalObjects::appSetting->value("HeaderViewState/MatchEditAnimeView"));
    if(!headerState.isNull())
        animeView->header()->restoreState(headerState.toByteArray());
    headerState = GlobalObjects::appSetting->value("HeaderViewState/MatchEditMatchView");
    if(!headerState.isNull())
        matchView->header()->restoreState(headerState.toByteArray());

    addOnCloseCallback([animeView, matchView](){
        GlobalObjects::appSetting->setValue("HeaderViewState/MatchEditAnimeView", animeView->header()->saveState());
        GlobalObjects::appSetting->setValue("HeaderViewState/MatchEditMatchView", matchView->header()->saveState());
    });

    return pageContainer;
}

void MatchEditor::onAccept()
{
    if(searchPage->isChecked())
    {
        if(this->anime.isEmpty())
        {
            showMessage(tr("Anime should not be empty"),NM_ERROR | NM_HIDE);
            return;
        }
    }
    else if(customPage->isChecked())
    {  
        QString animeTitle=animeEdit->text().trimmed();
        QString epTitle=epEdit->text().trimmed();
        QString epIndex = epIndexEdit->text().trimmed();
        if(animeTitle.isEmpty()|| epIndex.isEmpty())
        {
            showMessage(tr("Anime Title and Episode Index should not be empty"),NM_ERROR | NM_HIDE);
            return;
        }
        anime = animeTitle;
        singleEp.type = EpType(epTypeCombo->currentIndex()+1);
        singleEp.index = epIndex.toDouble();
        singleEp.name = epTitle;
        singleEp.localFile = curItem->path;
    }
    if(!lastSearchCacheId.isEmpty())
        animeCache.put(lastSearchCacheId,  static_cast<AnimeModel *>(animeModel)->animeBases());

    CFramelessDialog::onAccept();
}

void MatchEditor::onClose()
{
    if(!lastSearchCacheId.isEmpty())
        animeCache.put(lastSearchCacheId,  static_cast<AnimeModel *>(animeModel)->animeBases());
    CFramelessDialog::onClose();
}


#include "matcheditor.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "MediaLibrary/animeprovider.h"
#include "Play/Playlist/playlistitem.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaMenu.h"
#include "UI/ela/ElaPivot.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "UI/widgets/elidedlabel.h"
#include "UI/widgets/kpushbutton.h"
#include "widgets/scriptsearchoptionpanel.h"
#include "Extension/Script/scriptmanager.h"
#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QListWidget>
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
/*
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

    class EpComboDelegate : public KTreeviewItemDelegate
    {
    public:
        EpComboDelegate(QObject *parent = nullptr):KTreeviewItemDelegate(parent){}
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
*/

    class AnimeListModel : public QAbstractItemModel
    {
    public:
        AnimeListModel(QObject *parent = nullptr) : QAbstractItemModel(parent) {}
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
MatchEditor::MatchEditor(const PlayListItem *item, QList<const PlayListItem *> *batchItems, const QString &searchScriptId, QWidget *parent) :
    CFramelessDialog(tr("Episode Matching"),parent,true), curItem(item)
{
    comparer.setNumericMode(true);
	this->batchItems = batchItems;
    setFont(QFont(GlobalObjects::normalFont,12));

    QString animeTitle;
    EpInfo ep;
    Pool *pool = GlobalObjects::danmuManager->getPool(item->poolID, false);
    if(pool)
    {
        ep = pool->toEp();
        animeTitle = pool->animeTitle();
    }

    ElaPivot *tab = new ElaPivot(this);
    tab->appendPivot(tr("Search"));
    tab->appendPivot(tr("Custom"));
    tab->setCurrentIndex(0);

    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupSearchPage(animeTitle, searchScriptId));
    contentStackLayout->addWidget(setupCustomPage(animeTitle, ep));

    QObject::connect(tab, &ElaPivot::pCurrentIndexChanged, this, [=](){
        pageIndex = tab->getCurrentIndex();
        contentStackLayout->setCurrentIndex(pageIndex);
    });

    QLabel *matchInfoLabel = new QLabel(ep.type==EpType::UNKNOWN ? tr("No Match Info") : ep.toString(), this);
    matchInfoLabel->setFont(QFont(GlobalObjects::normalFont, 10, QFont::Bold));
    matchInfoLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);

    QVBoxLayout *matchVLayout=new QVBoxLayout(this);
    auto margins = matchVLayout->contentsMargins();
    margins.setTop(0);
    matchVLayout->setContentsMargins(margins);

    matchVLayout->addWidget(tab);
    matchVLayout->addLayout(contentStackLayout);
    matchVLayout->addWidget(matchInfoLabel);

    setSizeSettingKey("DialogSize/MatchEditor",QSize(600, 600));
    hitWords.clear();
}


QWidget *MatchEditor::setupCustomPage(const QString &srcAnime, const EpInfo &ep)
{
    QWidget *customPage = new QWidget(this);
    QFont normalFont(GlobalObjects::normalFont,10);
    customPage->setFont(normalFont);

    QLabel *animeTip = new QLabel(tr("Anime Title"),customPage);
    animeEdit = new ElaLineEdit(srcAnime, customPage);

    QLabel *epTypeTip = new QLabel(tr("Episode Type"),customPage);
    epTypeCombo = new ElaComboBox(customPage);
    epTypeCombo->addItems(QStringList(std::begin(EpTypeName), std::end(EpTypeName)));

    QLabel *epIndexTip = new QLabel(tr("Episode Index"),customPage);
    epIndexEdit = new ElaLineEdit(customPage);
    epIndexEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d+\\.?(\\d+)?"), epIndexEdit));

    QLabel *epTitleTip = new QLabel(tr("Episode Title"),customPage);
    epEdit = new ElaLineEdit(customPage);

    if (ep.type != EpType::UNKNOWN)
    {
        epTypeCombo->setCurrentIndex(ep.type - 1);
        epIndexEdit->setText(QString::number(ep.index));
        epEdit->setText(ep.name);
    }
    else
    {
        epEdit->setText(curItem->title);
    }

    QGridLayout *customGLayout=new QGridLayout(customPage);
    customGLayout->setContentsMargins(0, 0, 0, 0);
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

void MatchEditor::initEpList()
{
    QHash<const PlayListItem *, QString> titleHash;
    for (auto i : *batchItems)
    {
        int pathPos = i->path.lastIndexOf('/') + 1;
        titleHash[i] = i->path.mid(pathPos);
    }
    std::sort(batchItems->begin(), batchItems->end(), [&](const PlayListItem *p1, const PlayListItem *p2){
        return comparer.compare(titleHash[p1], titleHash[p2]) >= 0 ? false : true;
    });
    QStringList titles;
    for (auto i: *batchItems)
    {
        titles.append(titleHash[i]);
    }
    const QString commonPart = getCommonPart(titles);
    const QString replacePart = commonPart.isEmpty() ? "" : QString("%1..%2").arg(commonPart.left(2), commonPart.right(2));
    for (auto i : *batchItems)
    {
        MatchEpInfo matchedEp;
        matchedEp.checked = true;
        matchedEp.item = i;
        matchedEp.ep.localFile = i->path;
        matchedEp.displayTitle = titleHash[i];
        if (!commonPart.isEmpty())
        {
            matchedEp.displayTitle = matchedEp.displayTitle.replace(commonPart, replacePart);
        }
        matchEpList.append(matchedEp);

        EpItemWidget *itemWidget = new EpItemWidget(matchEpList, matchEpList.size() - 1, curAnimeEpList);
        QListWidgetItem *listItem = new QListWidgetItem(epListView);
        epListView->setItemWidget(listItem, itemWidget);
        listItem->setSizeHint(itemWidget->sizeHint());
        QObject::connect(itemWidget, &EpItemWidget::setIndexFrom, this, [=](int poolIndex){
            int row = epListView->row(listItem);
            int epIndex = itemWidget->getIndex();
            for (int i = row + 1; i < epListView->count(); ++i)
            {
                auto epWidget = static_cast<EpItemWidget *>(epListView->itemWidget(epListView->item(i)));
                if (epWidget->checked())
                {
                    epWidget->setIndex(++epIndex);
                }
            }
        });
    }
}

QString MatchEditor::getCommonPart(const QStringList &titles)
{
    if (titles.size() < 2) return "";
    int minTitleLen = INT_MAX;
    QString suffixItem;
    const int minLength = 6;
    for (auto &t : titles)
    {
        if (t.isEmpty()) return "";
        if (t.length() < minTitleLen)
        {
            minTitleLen = t.length();
            suffixItem = t;
        }
    }
    if (minTitleLen < minLength) return "";

    std::function<int (const QString &,const QString &,int *)> kmpMatchLen
        = [](const QString &suffix,const QString &str,int *next)
    {
        int maxMatchLen=0,i=0,j=0;
        for (; i < str.length(); ++i)
        {
            while (j > -1 && str[i] != suffix[j])
                j = next[j];
            if (j == suffix.length() - 1)
                return j;
            j = j + 1;
            if(j > maxMatchLen)maxMatchLen=j;
        }
        return maxMatchLen;
    };
    std::function<int * (const QString &)> getNext
        =[](const QString &suffix)
    {
        int i = 0, j = -1;
        int *next = new int[suffix.length()];
        for (; i < suffix.length(); ++i) {
            next[i] = j;
            while (j > -1 && suffix[i] != suffix[j])
                j = next[j];
            j = j + 1;
        }
        return next;
    };
    QString matchStr;
    for (int i = minTitleLen; i >= minLength; i--)
    {
        QString suffix(suffixItem.right(i));
        int *next = getNext(suffix);
        int tmpMaxMinMatch = INT_MAX;
        for (auto &t : titles)
        {
            if (t!=suffixItem)
            {
                int len = kmpMatchLen(suffix, t, next);
                if(len<tmpMaxMinMatch) tmpMaxMinMatch = len;
            }
        }
        delete[] next;
        if (tmpMaxMinMatch > matchStr.length()) matchStr=suffix.left(tmpMaxMinMatch);
    }
    return matchStr.length() < minLength? "" : matchStr;
}

void MatchEditor::refreshEpList()
{
    for (int i = 0; i < epListView->count(); ++i)
    {
        auto epWidget = static_cast<EpItemWidget *>(epListView->itemWidget(epListView->item(i)));
        epWidget->refreshEps();
    }
}

QWidget *MatchEditor::setupSearchPage(const QString &srcAnime, const QString &searchScriptId)
{
    QWidget *pageContainer = new QWidget(this);
    QFont normalFont(GlobalObjects::normalFont,10);
    pageContainer->setFont(normalFont);

    QWidget *searchSubPage = new QWidget(pageContainer);
    QComboBox *scriptCombo = new ElaComboBox(searchSubPage);
    QLineEdit *keywordEdit = new ElaLineEdit(srcAnime, searchSubPage);
    QPushButton *searchBtn = new KPushButton(tr("Search"), searchSubPage);
    ScriptSearchOptionPanel *scriptOptionPanel = new ScriptSearchOptionPanel(this);
    QTreeView *animeView = new QTreeView(searchSubPage);

    QGridLayout *searchSubGLayout = new QGridLayout(searchSubPage);
    searchSubGLayout->setContentsMargins(0, 0, 0, 0);
    searchSubGLayout->addWidget(scriptCombo, 0, 0);
    searchSubGLayout->addWidget(keywordEdit, 0, 1);
    searchSubGLayout->addWidget(searchBtn, 0, 2);
    searchSubGLayout->addWidget(scriptOptionPanel, 1, 0, 1, 3);
    searchSubGLayout->addWidget(animeView, 2, 0, 1, 3);
    searchSubGLayout->setRowStretch(2, 1);
    searchSubGLayout->setColumnStretch(1, 1);

    QObject::connect(scriptCombo, &QComboBox::currentTextChanged, this, [=](const QString &){
        QString curId = scriptCombo->currentData().toString();
        scriptOptionPanel->setScript(GlobalObjects::scriptManager->getScript(curId));
        if(scriptOptionPanel->hasOptions()) scriptOptionPanel->show();
        else scriptOptionPanel->hide();
    });
    int selectPos = -1, index = 0;
    for (const auto &s : GlobalObjects::animeProvider->getSearchProviders())
    {
        scriptCombo->addItem(s.first, s.second);
        if (s.second == searchScriptId) selectPos = index;
        ++index;
    }
    scriptCombo->addItem(tr("Local DB"), "Kiko.Local");
    if (selectPos != -1) scriptCombo->setCurrentIndex(selectPos);

    animeModel = new AnimeListModel(this);
    animeView->setRootIsDecorated(false);
    animeView->setFont(QFont(GlobalObjects::normalFont, 11));
    animeView->header()->setFont(QFont(GlobalObjects::normalFont, 12));
    animeView->setSelectionMode(QAbstractItemView::SingleSelection);
    animeView->setModel(animeModel);
    animeView->setContextMenuPolicy(Qt::CustomContextMenu);
    animeView->setItemDelegate(new KTreeviewItemDelegate(animeView));
    animeView->header()->resizeSection(0, 240);

    ElaMenu *actionMenu = new ElaMenu(animeView);
    QObject::connect(animeView, &QTreeView::customContextMenuRequested, this, [=](){
        if (!animeView->selectionModel()->hasSelection()) return;
        actionMenu->exec(QCursor::pos());
    });

    QAction *copy = actionMenu->addAction(tr("Copy"));
    QObject::connect(copy, &QAction::triggered, this, [=](){
        auto selectedRows= animeView->selectionModel()->selectedRows((int)AnimeListModel::Columns::ANIME);
        if(selectedRows.count()==0) return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(selectedRows.first().data().toString());
    });
    animeView->addAction(copy);

    if (!scriptCombo->currentData().isNull())
    {
        QString id(QString("%1/%2").arg(scriptCombo->currentData().toString(), srcAnime));
        if(animeCache.contains(id))
        {
            static_cast<AnimeListModel *>(animeModel)->reset(animeCache.get(id));
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
                static_cast<AnimeListModel *>(animeModel)->reset(animeCache.get(cacheId));
                hitWords[keyword] = 3;
                return;
            }
        }
        searchSubPage->setEnabled(false);
        showBusyState(true);
        QList<AnimeLite> results;
        ScriptState state;
        if(scriptCombo->currentIndex()==scriptCombo->count()-1)
        {
            GlobalObjects::danmuManager->localSearch(keyword, results);
        }
        else
        {
            QMap<QString, QString> searchOptions;
            if(scriptOptionPanel->hasOptions() && scriptOptionPanel->changed())
            {
                searchOptions = scriptOptionPanel->getOptionVals();
            }
            state = GlobalObjects::animeProvider->animeSearch(scriptCombo->currentData().toString(), keyword, searchOptions, results);
        }
        if(state)
        {
            if(!lastSearchCacheId.isEmpty())
                animeCache.put(lastSearchCacheId,  static_cast<AnimeListModel *>(animeModel)->animeBases());
            lastSearchCacheId = cacheId;
            --hitWords[keyword];
            if(hitWords[keyword] <= 0)
                hitWords.remove(keyword);
            static_cast<AnimeListModel *>(animeModel)->reset(results);
            animeCache.put(cacheId, results);
        } else {
            showMessage(state.info, NM_ERROR | NM_HIDE);
        }
        showBusyState(false);
        searchSubPage->setEnabled(true);
    });

    QWidget *matchSubPage = new QWidget(pageContainer);
    QPushButton *backBtn = new KPushButton(tr("Back"), matchSubPage);
    QPushButton *selectAllBtn = new KPushButton(tr("Select All/Cancel"), matchSubPage);
    QLabel *animeLabel = new QLabel(matchSubPage);
    animeLabel->setFont(QFont(GlobalObjects::normalFont, 12));

    epListView = new QListWidget(matchSubPage);
    epListView->setFont(QFont(GlobalObjects::normalFont, 11));
    epListView->setDragEnabled(true);
    epListView->setObjectName(QStringLiteral("MatchEpListView"));
    epListView->setAcceptDrops(true);
    epListView->setDragDropMode(QAbstractItemView::InternalMove);
    epListView->setDropIndicatorShown(true);
    epListView->setDefaultDropAction(Qt::MoveAction);
    epListView->setSelectionMode(QListWidget::SingleSelection);
    epListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    QGridLayout *matchSubGLayout = new QGridLayout(matchSubPage);
    matchSubGLayout->setContentsMargins(0, 0, 0, 0);
    matchSubGLayout->addWidget(selectAllBtn, 0, 0);
    matchSubGLayout->addWidget(animeLabel, 0, 1);
    matchSubGLayout->addWidget(backBtn, 0, 2);
    matchSubGLayout->addWidget(epListView, 1, 0, 1, 3);
    matchSubGLayout->setRowStretch(1, 1);
    matchSubGLayout->setColumnStretch(1, 1);

    QStackedLayout *searchSLayout = new QStackedLayout(pageContainer);
    searchSLayout->addWidget(searchSubPage);
    searchSLayout->addWidget(matchSubPage);

    initEpList();

    QObject::connect(animeView, &QAbstractItemView::clicked, this, [=](const QModelIndex &index){
        auto animeLite = animeModel->data(index, AnimeRole).value<AnimeLite>();
        if(animeLite.epList)
        {
            this->curAnime = animeLite;
            curAnimeEpList = *animeLite.epList;
            animeLabel->setText(animeLite.name);
            refreshEpList();
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
                static_cast<AnimeListModel *>(animeModel)->fillEpInfo(index, results);
                auto anime = animeModel->data(index, AnimeRole).value<AnimeLite>();
                curAnimeEpList = *anime.epList;
                animeLabel->setText(anime.name);
                this->curAnime = anime;
                refreshEpList();
                searchSLayout->setCurrentIndex(1);
            } else {
                showMessage(state.info, NM_ERROR | NM_HIDE);
            }
        }
    });

    QObject::connect(backBtn, &QPushButton::clicked, this, [=](){
       searchSLayout->setCurrentIndex(0);
       this->curAnime = AnimeLite();
    });

    QObject::connect(selectAllBtn, &QPushButton::clicked, this, [=](){
        bool select = false;
        for (int i = 0; i < epListView->count(); ++i)
        {
            auto epWidget = static_cast<EpItemWidget *>(epListView->itemWidget(epListView->item(i)));
            if (i == 0)
            {
                select = !epWidget->checked();
            }
            epWidget->setChecked(select);
        }
    });

    addOnCloseCallback([animeView](){
        GlobalObjects::appSetting->setValue("HeaderViewState/MatchEditAnimeView", animeView->header()->saveState());
    });

    return pageContainer;
}

void MatchEditor::onAccept()
{
    if (pageIndex == 0)
    {
        if (this->curAnime.name.isEmpty())
        {
            showMessage(tr("Anime should not be empty"),NM_ERROR | NM_HIDE);
            return;
        }
    }
    else if (pageIndex == 1)
    {  
        QString animeTitle=animeEdit->text().trimmed();
        QString epTitle=epEdit->text().trimmed();
        QString epIndex = epIndexEdit->text().trimmed();
        if (animeTitle.isEmpty()|| epIndex.isEmpty())
        {
            showMessage(tr("Anime Title and Episode Index should not be empty"), NM_ERROR | NM_HIDE);
            return;
        }
        curAnime.name = animeTitle;
        singleEp.type = EpType(epTypeCombo->currentIndex()+1);
        singleEp.index = epIndex.toDouble();
        singleEp.name = epTitle;
        singleEp.localFile = curItem->path;
    }
    if (!lastSearchCacheId.isEmpty())
    {
        animeCache.put(lastSearchCacheId,  static_cast<AnimeListModel *>(animeModel)->animeBases());
    }

    CFramelessDialog::onAccept();
}

void MatchEditor::onClose()
{
    if(!lastSearchCacheId.isEmpty())
        animeCache.put(lastSearchCacheId,  static_cast<AnimeListModel *>(animeModel)->animeBases());
    CFramelessDialog::onClose();
}


EpItemWidget::EpItemWidget(QList<MatchEpInfo> &matchList, int index, const QList<EpInfo> &eps) :
    _matchList(matchList), _index(index), _eps(eps)
{
    MatchEpInfo &info = _matchList[index];

    int pathPos = info.ep.localFile.lastIndexOf('/') + 1;
    const QString fileName = info.ep.localFile.mid(pathPos);

    ElidedLabel *titleLabel = new ElidedLabel(info.displayTitle, this);
    titleLabel->setFont(QFont(GlobalObjects::normalFont, 13));
    titleLabel->setFontColor(QColor(240, 240, 240));
    titleLabel->setToolTip(fileName);


    epCombo = new ElaComboBox(this);
    epCombo->setMaximumWidth(180);
    epCombo->setFrame(false);
    epCombo->view()->setItemDelegate(new EpComboItemDelegate(this));

    QPushButton *autoSetEpBtn = new KPushButton(this);
    autoSetEpBtn->setToolTip(tr("Set Episode in Sequence"));
    autoSetEpBtn->setObjectName(QStringLiteral("AutoSetPoolBtn"));
    autoSetEpBtn->setFont(*GlobalObjects::iconfont);
    autoSetEpBtn->setText(QChar(0xe6ed));
    QHBoxLayout *poolHLayout = new QHBoxLayout;
    poolHLayout->setContentsMargins(0, 0, 0, 0);
    poolHLayout->addWidget(epCombo);
    poolHLayout->addWidget(autoSetEpBtn);

    refreshEps();

    QObject::connect(epCombo, &QComboBox::currentIndexChanged, this, [=](int index){
        if (!epCombo->currentData(EpRole).isValid()) return;
        int epIndex = epCombo->currentData(EpRole).toInt();
        if (_eps.empty() || epIndex < 0 || epIndex >= _eps.size()) return;
        _matchList[_index].ep.infoAssign(_eps[epIndex]);
    });
    QObject::connect(autoSetEpBtn, &QPushButton::clicked, this, [=](){
        if (!epCombo->currentData(EpRole).isValid()) return;
        int epIndex = epCombo->currentData(EpRole).toInt();
        emit setIndexFrom(epIndex);
    });

    epCheck = new ElaCheckBox(" ", this);
    epCheck->setChecked(info.checked);
    QObject::connect(epCheck, &QCheckBox::clicked, this, [=](bool checked){
        _matchList[_index].checked = checked;
    });

    QGridLayout *itemGLayout=new QGridLayout(this);
    itemGLayout->addWidget(epCheck, 0, 0);
    itemGLayout->addWidget(titleLabel, 0, 1);
    itemGLayout->addItem(poolHLayout, 0, 2);
    itemGLayout->setColumnStretch(1, 1);
}

void EpItemWidget::setIndex(int index)
{
    for (int i = 0; i < epCombo->count(); ++i)
    {
        if (epCombo->itemData(i, EpRole).isValid())
        {
            int epIndex = epCombo->itemData(i, EpRole).toInt();
            if (epIndex == index && epIndex >= 0 && epIndex < _eps.size())
            {
                _matchList[_index].ep.infoAssign(_eps[epIndex]);
                epCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

int EpItemWidget::getIndex() const
{
    return epCombo->currentData(EpRole).toInt();
}

void EpItemWidget::setChecked(bool on)
{
    epCheck->setChecked(on);
    _matchList[_index].checked = on;
}

void EpItemWidget::refreshEps()
{
    epCombo->clear();
    int lastType = -1;
    for (int i = 0; i < _eps.size(); ++i)
    {
        auto &ep = _eps[i];
        if (static_cast<int>(ep.type) != lastType)
        {
            addParentItem(epCombo, EpTypeName[static_cast<int>(ep.type) - 1]);
        }
        addChildItem(epCombo, ep.toString(), i);
        lastType = static_cast<int>(ep.type);
    }
    if (!_eps.empty())
    {
        setIndex(0);
    }
}

QSize EpItemWidget::sizeHint() const
{
    return layout()->sizeHint();
}

void EpItemWidget::addParentItem(QComboBox *combo, const QString &text) const
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

void EpItemWidget::addChildItem(QComboBox *combo, const QString &text, int epListIndex) const
{
    QStandardItem* item = new QStandardItem(text);
    item->setData("child", Qt::AccessibleDescriptionRole );
    item->setData(epListIndex, EpRole);
    QStandardItemModel* itemModel = (QStandardItemModel*)combo->model();
    itemModel->appendRow(item);
}

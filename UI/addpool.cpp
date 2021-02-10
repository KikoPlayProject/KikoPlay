#include "addpool.h"
#include <QGridLayout>
#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QClipboard>
#include <QApplication>
#include <QAction>
#include <QTreeView>
#include <QComboBox>
#include <QButtonGroup>
#include <QAbstractItemModel>
#include "globalobjects.h"
#include "Common/lrucache.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "MediaLibrary/animeprovider.h"
#define AnimeRole Qt::UserRole+1
#define EpRole Qt::UserRole+2
namespace
{
    QSet<QString> hitWords;
    LRUCache<QString, QList<AnimeLite>> animeCache{64};
    QString lastSearchCacheId;

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
        void fillEpInfo(const QModelIndex &index, const QList<EpInfo> &epList)
        {
            if(!index.isValid()) return;
            animeBaseList[index.row()].epList.reset(new QList<EpInfo>(epList));
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
            if(role==Qt::DisplayRole)
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
    class EpModel : public QAbstractItemModel
    {
    public:
        EpModel(QObject *parent = nullptr) : QAbstractItemModel(parent) {}
        void reset(const AnimeLite &anime)
        {
            beginResetModel();
            this->animeLite = anime;
            endResetModel();
        }
        const AnimeLite &anime() const {return animeLite;}
        enum class Columns
        {
            TYPE, INDEX, EPNAME
        };

    private:
        QStringList headers={QObject::tr("Type"),QObject::tr("Index"),QObject::tr("EpName")};
        AnimeLite animeLite;
    public:
        inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
        inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
        inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()||!animeLite.epList?0:animeLite.epList->count();}
        inline virtual int columnCount(const QModelIndex &) const{return headers.size();}
        virtual QVariant data(const QModelIndex &index, int role) const
        {
            if(!index.isValid() || !animeLite.epList) return QVariant();
            int row = index.row();
            auto &ep = (*animeLite.epList)[row];
            Columns col=static_cast<Columns>(index.column());
            if(role==Qt::DisplayRole)
            {
                switch (col)
                {
                case Columns::TYPE:
                    return EpTypeName[ep.type-1];
                case Columns::INDEX:
                    return ep.index;
                case Columns::EPNAME:
                    return ep.name;
                }
            }
            else if(role==EpRole)
            {
                return QVariant::fromValue<EpInfo>(ep);
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
AddPool::AddPool(QWidget *parent, const QString &srcAnime, const EpInfo &ep) : CFramelessDialog(tr("Add Danmu Pool"),parent,true)
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
    contentStackLayout->addWidget(setupSearchPage(srcAnime, ep));
    contentStackLayout->addWidget(setupCustomPage(srcAnime, ep));
    addPoolVLayout->addLayout(contentStackLayout);


    renamePool=(!srcAnime.isEmpty() && ep.type!=EpType::UNKNOWN);
    if(renamePool) setTitle(tr("Rename Danmu Pool"));
    resize(GlobalObjects::appSetting->value("DialogSize/AddPool",QSize(400*logicalDpiX()/96,400*logicalDpiY()/96)).toSize());
    hitWords.clear();
}

QWidget *AddPool::setupSearchPage(const QString &srcAnime, const EpInfo &)
{
    QFont normalFont(GlobalObjects::normalFont,10);
    QWidget *pageContainer=new QWidget(this);
    pageContainer->setFont(normalFont);

    QWidget *animePage = new QWidget(pageContainer);

    QComboBox *scriptCombo = new QComboBox(animePage);
    for(const auto &s : GlobalObjects::animeProvider->getSearchProviders())
    {
        scriptCombo->addItem(s.first, s.second);
    }
    scriptCombo->addItem(tr("Local DB"), "Kiko.Local");

    QLineEdit *keywordEdit=new QLineEdit(animePage);
    keywordEdit->setText(srcAnime);

    QPushButton *searchButton=new QPushButton(tr("Search"),animePage);

    animeModel = new AnimeModel(this);
    if(!scriptCombo->currentData().isNull())
    {
        QString id(QString("%1/%2").arg(scriptCombo->currentData().toString(), srcAnime));
        if(animeCache.contains(id))
        {
            static_cast<AnimeModel *>(animeModel)->reset(animeCache.get(id));
        }
    }

    QTreeView *animeView=new QTreeView(animePage);
    animeView->setRootIsDecorated(false);
    animeView->setModel(animeModel);
    animeView->setSelectionMode(QAbstractItemView::SingleSelection);
    animeView->setContextMenuPolicy(Qt::ActionsContextMenu);

    QGridLayout *animePageGLayout=new QGridLayout(animePage);
    animePageGLayout->addWidget(scriptCombo,0,0);
    animePageGLayout->addWidget(keywordEdit,0,1);
    animePageGLayout->addWidget(searchButton,0,2);
    animePageGLayout->addWidget(animeView,1,0,1,3);
    animePageGLayout->setRowStretch(1,1);
    animePageGLayout->setColumnStretch(1,1);

    QAction *copy=new QAction(tr("Copy"), this);
    QObject::connect(copy, &QAction::triggered, this, [=](){
        auto selectedRows= animeView->selectionModel()->selectedRows((int)AnimeModel::Columns::ANIME);
        if(selectedRows.count()==0) return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(selectedRows.first().data().toString());
    });
    animeView->addAction(copy);

    QObject::connect(searchButton,&QPushButton::clicked,this,[=](){
        QString keyword=keywordEdit->text().trimmed();
        if(keyword.isEmpty()) return;
        auto cacheId = QString("%1/%2").arg(scriptCombo->currentData().toString(), keyword);
        if(!hitWords.contains(keyword))
        {

            if(animeCache.contains(cacheId))
            {
                lastSearchCacheId = cacheId;
                static_cast<AnimeModel *>(animeModel)->reset(animeCache.get(cacheId));
                hitWords<<keyword;
                return;
            }
        }
        animePage->setEnabled(false);
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
            hitWords.remove(keyword);
            static_cast<AnimeModel *>(animeModel)->reset(results);
            animeCache.put(cacheId, results);
        }
        showBusyState(false);
        animePage->setEnabled(true);
    });
    QObject::connect(keywordEdit,&QLineEdit::returnPressed,searchButton,&QPushButton::click);

    QWidget *epPage = new QWidget(pageContainer);
    QPushButton *backBtn = new QPushButton(tr("Back"), epPage);
    QLabel *animeLabel = new QLabel(epPage);
    epView = new QTreeView(epPage);
    epView->setRootIsDecorated(false);
    epView->setSelectionMode(QAbstractItemView::SingleSelection);
    epView->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *copyEp=new QAction(tr("Copy"), this);
    QObject::connect(copyEp, &QAction::triggered, this, [=](){
        auto selectedRows= epView->selectionModel()->selectedRows((int)EpModel::Columns::EPNAME);
        if(selectedRows.count()==0) return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(selectedRows.first().data().toString());
    });
    epView->addAction(copyEp);

    epModel = new EpModel(this);
    epView->setModel(epModel);

    QGridLayout *epPageGLayout = new QGridLayout(epPage);
    epPageGLayout->addWidget(animeLabel, 0, 0);
    epPageGLayout->addWidget(backBtn, 0, 1);
    epPageGLayout->addWidget(epView, 1, 0, 1, 2);
    epPageGLayout->setRowStretch(1, 1);
    epPageGLayout->setColumnStretch(0, 1);

    QStackedLayout *searchSLayout = new QStackedLayout(pageContainer);
    searchSLayout->addWidget(animePage);
    searchSLayout->addWidget(epPage);

    QObject::connect(backBtn, &QPushButton::clicked, this, [=](){
       searchSLayout->setCurrentIndex(0);
       static_cast<EpModel *>(epModel)->reset(AnimeLite());
    });

    QObject::connect(animeView, &QAbstractItemView::clicked, this, [=](const QModelIndex &index){
        auto animeLite = static_cast<AnimeModel *>(animeModel)->data(index, AnimeRole).value<AnimeLite>();
        if(animeLite.epList)
        {
            static_cast<EpModel *>(epModel)->reset(animeLite);
            searchSLayout->setCurrentIndex(1);
        }
        else
        {
            QScopedPointer<Anime> anime(animeLite.toAnime());
            QList<EpInfo> results;
            animePage->setEnabled(false);
            showBusyState(true);
            ScriptState state = GlobalObjects::animeProvider->getEp(anime.data(), results);
            animePage->setEnabled(true);
            showBusyState(false);
            if(state)
            {
                static_cast<AnimeModel *>(animeModel)->fillEpInfo(index, results);
                auto anime = static_cast<AnimeModel *>(animeModel)->data(index, AnimeRole).value<AnimeLite>();
                static_cast<EpModel *>(epModel)->reset(anime);
                animeLabel->setText(anime.name);
                searchSLayout->setCurrentIndex(1);
            } else {
                showMessage(state.info, 1);
            }
        }
    });

    return pageContainer;
}

QWidget *AddPool::setupCustomPage(const QString &srcAnime, const EpInfo &ep)
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
    epIndexEdit->setValidator(new QRegExpValidator(QRegExp("\\d+.?(\\d+)?"),epIndexEdit));

    QLabel *epTitleTip=new QLabel(tr("Episode Title"),customPage);
    epEdit=new QLineEdit(ep.name, customPage);

    if(ep.type!=EpType::UNKNOWN)
    {
        epTypeCombo->setCurrentIndex(ep.type-1);
        epIndexEdit->setText(QString::number(ep.index));
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

void AddPool::onAccept()
{
    if(searchPage->isChecked())
    {
        auto selectedRows= epView->selectionModel()->selectedRows((int)EpModel::Columns::EPNAME);
        if(selectedRows.count()==0)
        {
            showMessage(tr("You need to choose an episode"), 1);
            return;
        }
        const auto &anime = static_cast<EpModel *>(epModel)->anime();
        EpInfo ep = static_cast<EpModel *>(epModel)->data(selectedRows.first(), EpRole).value<EpInfo>();
        this->anime = anime.name;
        this->ep = ep.name;
        this->epIndex = ep.index;
        this->epType = ep.type;
    }
    else if(customPage->isChecked())
    {
        this->anime = animeEdit->text().trimmed();
        this->ep = epEdit->text().trimmed();
        this->epIndex = epIndexEdit->text().toDouble();
        this->epType = EpType(epTypeCombo->currentIndex()+1);
    }
    if(anime.isEmpty() || epIndexEdit->text().trimmed().isEmpty())
    {
        showMessage(tr("Anime Title and Episode Index should not be empty"), 1);
        return;
    }
    if(!renamePool && GlobalObjects::danmuManager->getPool(anime, epType, epIndex, false))
    {
        showMessage(tr("Pool Already Exists"), 1);
        return;
    }
    if(!lastSearchCacheId.isEmpty())
        animeCache.put(lastSearchCacheId,  static_cast<AnimeModel *>(animeModel)->animeBases());
    GlobalObjects::appSetting->setValue("DialogSize/AddPool",size());
    CFramelessDialog::onAccept();
}

void AddPool::onClose()
{
    if(!lastSearchCacheId.isEmpty())
        animeCache.put(lastSearchCacheId,  static_cast<AnimeModel *>(animeModel)->animeBases());
    GlobalObjects::appSetting->setValue("DialogSize/AddPool",size());
    CFramelessDialog::onClose();
}

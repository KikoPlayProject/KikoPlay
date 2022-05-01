#include "librarywindow.h"
#include <QListView>
#include <QTreeView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QStackedLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMovie>
#include <QToolButton>
#include <QPushButton>
#include <QScrollArea>
#include <QMouseEvent>
#include <QScrollBar>
#include <QButtonGroup>
#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QSplitter>
#include <QApplication>
#include <QClipboard>

#include "globalobjects.h"
#include "Common/notifier.h"
#include "animesearch.h"
#include "episodeeditor.h"
#include "inputdialog.h"
#include "widgets/dialogtip.h"
#include "animedetailinfopage.h"
#include "animebatchaction.h"
#include "Script/scriptmanager.h"
#include "Script/libraryscript.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/animeitemdelegate.h"
#include "MediaLibrary/animeprovider.h"
#include "MediaLibrary/animemodel.h"
#include "MediaLibrary/labelmodel.h"
#include "MediaLibrary/labelitemdelegate.h"
#include "MediaLibrary/animefilterproxymodel.h"
#define TagNodeRole Qt::UserRole+3

namespace
{
    class CListView : public QListView
    {
    public:
        using QListView::QListView;

        // QWidget interface
    protected:
        virtual void resizeEvent(QResizeEvent *event)
        {
            QSize sz(this->itemDelegate()->sizeHint(QStyleOptionViewItem(),QModelIndex()));

            int w = event->size().width() - 2*logicalDpiX()/96 - (this->verticalScrollBar()->isHidden()?
                                                 0:this->verticalScrollBar()->width());
            int c = w / sz.width();
            if (c > 0)
            {
                int wInc = (w % sz.width()) / c;
                setGridSize(QSize(sz.width() + wInc, sz.height()));
            }
            else
            {
                setGridSize(sz);
            }
        }
    };
}

LibraryWindow::LibraryWindow(QWidget *parent) : QWidget(parent), animeViewing(false)
{
    setObjectName(QStringLiteral("LibraryWindow"));
    dialogTip = new DialogTip(this);
    dialogTip->raise();
    dialogTip->hide();
    Notifier::getNotifier()->addNotify(Notifier::LIBRARY_NOTIFY, this);

    AnimeItemDelegate *itemDelegate=new AnimeItemDelegate(this);

    splitter = new QSplitter(this);
    splitter->setObjectName(QStringLiteral("LabelSplitter"));
    QWidget *animeContainer = new QWidget(splitter);

    detailPage = new AnimeDetailInfoPage(animeContainer);
    detailPage->setProperty("cScrollStyle", true);

    animeListView=new CListView(animeContainer);
    animeListView->setObjectName(QStringLiteral("AnimesContent"));
    animeListView->setProperty("cScrollStyle", true);
    animeListView->setViewMode(QListView::IconMode);
    animeListView->setUniformItemSizes(true);
    animeListView->setResizeMode(QListView::Adjust);
    animeListView->setMovement(QListView::Static);
    animeListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    animeListView->setSelectionMode(QAbstractItemView::SingleSelection);
    animeListView->setItemDelegate(itemDelegate);
    animeListView->setMouseTracking(true);
    animeListView->setContextMenuPolicy(Qt::CustomContextMenu);
    animeModel = new AnimeModel(this);
    proxyModel=new AnimeFilterProxyModel(animeModel, this);
    proxyModel->setSourceModel(animeModel);
    animeListView->setModel(proxyModel);

    QTimer *refreshBlockTimer = new QTimer(this);
    QObject::connect(refreshBlockTimer, &QTimer::timeout, this, [=](){
        itemDelegate->setBlockCoverFetch(false);
        animeListView->update();
        refreshBlockTimer->stop();
    });
    connect(animeListView->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](){
        refreshBlockTimer->stop();
        refreshBlockTimer->start(10);
        itemDelegate->setBlockCoverFetch(true);
    });

    QAction *act_delete=new QAction(tr("Delete"),this);
    QObject::connect(act_delete,&QAction::triggered,[this](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        Anime *anime = animeModel->getAnime(selection.indexes().first());
        if(anime)
        {
            if(detailPage->curAnime()==anime) detailPage->setAnime(nullptr);
            LabelModel::instance()->removeTag(anime->name(), anime->airDate(), anime->scriptId());
            animeModel->deleteAnime(selection.indexes().first());
        }
    });
    act_delete->setEnabled(false);

    QAction *act_batch=new QAction(tr("Batch Operation"),this);
    QObject::connect(act_batch,&QAction::triggered,[this](){
        AnimeBatchAction batchAction(animeModel, this);
        batchAction.exec();
    });

    QAction *act_getDetailInfo=new QAction(tr("Search Details"),this);
    QObject::connect(act_getDetailInfo,&QAction::triggered,[this](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        searchAddAnime(animeModel->getAnime(selection.indexes().first()));
    });
    act_getDetailInfo->setEnabled(false);

    QAction *act_updateDetailInfo=new QAction(tr("Update"),this);
    QObject::connect(act_updateDetailInfo,&QAction::triggered,[this](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        Anime *currentAnime = animeModel->getAnime(selection.indexes().first());
        if(currentAnime->scriptId().isEmpty())
        {
            showMessage(tr("No Script ID, Search For Detail First"), NM_HIDE);
            return;
        }
        QSharedPointer<ScriptBase> script = GlobalObjects::scriptManager->getScript(currentAnime->scriptId());
        if(!script)
        {
            showMessage(tr("Script \"%1\" not exist").arg(currentAnime->scriptId()), NM_HIDE);
            return;
        }
        QString scriptName(script->name());
        showMessage(tr("Fetching Info from %1").arg(scriptName),  NM_PROCESS | NM_DARKNESS_BACK);
        Anime *nAnime = new Anime;
        ScriptState state = GlobalObjects::animeProvider->getDetail(currentAnime->toLite(), nAnime);
        if(!state)
        {
            showMessage(state.info, NM_HIDE | NM_ERROR);
            delete nAnime;
        }
        else
        {
            QString animeName(AnimeWorker::instance()->addAnime(currentAnime, nAnime));
            auto &tagAnimes = LabelModel::instance()->customTags();
            bool hasTag = false;
            for(const auto &animes : tagAnimes)
            {
                if(animes.contains(animeName))
                {
                    hasTag = true;
                    break;
                }
            }
            if(!hasTag)
            {
                QStringList tags;
                Anime *tAnime = AnimeWorker::instance()->getAnime(animeName);
                if(tAnime)
                {
                    showMessage(tr("Fetching Tags from %1").arg(scriptName), NM_PROCESS | NM_DARKNESS_BACK);
                    GlobalObjects::animeProvider->getTags(tAnime, tags);
                    if(tags.size()>0)
                    {
                        LabelModel::instance()->addCustomTags(tAnime->name(), tags);
                    }
                }
            }
            showMessage(tr("Fetch Down"), NotifyMessageFlag::NM_HIDE);
        }
    });
    act_updateDetailInfo->setEnabled(false);

    QMenu *addSubMenu = new QMenu(tr("Add Anime"), animeListView);
    QAction *act_searchAdd=new QAction(tr("Search Add"), this);
    QObject::connect(act_searchAdd, &QAction::triggered, this, [this](){
        searchAddAnime();
    });
    QAction *act_directAdd=new QAction(tr("Direct Add"), this);
    QObject::connect(act_directAdd,&QAction::triggered, this, [this](){
        LineInputDialog input(tr("Add Anime"), tr("Anime Name"), "", "DialogSize/DirectAddAnime", false, this);
        if(QDialog::Accepted == input.exec())
        {
            AnimeWorker::instance()->addAnime(input.text);
        }
    });
    addSubMenu->addActions({act_searchAdd, act_directAdd});

    QMenu *orderSubMenu = new QMenu(tr("Sort Order"), animeListView);
    QActionGroup *ascDesc = new QActionGroup(orderSubMenu);
    QAction *actAsc = ascDesc->addAction(tr("Ascending"));
    QAction *actDesc = ascDesc->addAction(tr("Descending"));
    actAsc->setCheckable(true);
    actDesc->setCheckable(true);
    actDesc->setChecked(true);
    QObject::connect(ascDesc, &QActionGroup::triggered, this, [=](QAction *act){
        proxyModel->setAscending(act==actAsc);
    });
    bool orderAsc = GlobalObjects::appSetting->value("Library/SortOrderAscending", false).toBool();
    if(orderAsc)
        actAsc->trigger();


    QActionGroup *orderTypes = new QActionGroup(orderSubMenu);
    QAction *actOrderAddTime = orderTypes->addAction(tr("Add Time"));
    QAction *actOrderName = orderTypes->addAction(tr("Anime Name"));
    QAction *actOrderDate = orderTypes->addAction(tr("Air Date"));
    actOrderAddTime->setCheckable(true);
    actOrderName->setCheckable(true);
    actOrderDate->setCheckable(true);
    actOrderAddTime->setChecked(true);
    static QList<QAction *> acts({actOrderAddTime, actOrderName, actOrderDate});
    QObject::connect(orderTypes, &QActionGroup::triggered, this, [this](QAction *act){
        proxyModel->setOrder((AnimeFilterProxyModel::OrderType)acts.indexOf(act));
    });
    AnimeFilterProxyModel::OrderType orderType =
           AnimeFilterProxyModel::OrderType(GlobalObjects::appSetting->value("Library/SortOrderType", (int)AnimeFilterProxyModel::OrderType::O_AddTime).toInt());
    if(orderType != AnimeFilterProxyModel::OrderType::O_AddTime)
        acts[(int)orderType]->trigger();

    orderSubMenu->addAction(actAsc);
    orderSubMenu->addAction(actDesc);
    orderSubMenu->addSeparator();
    orderSubMenu->addAction(actOrderAddTime);
    orderSubMenu->addAction(actOrderName);
    orderSubMenu->addAction(actOrderDate);

    QMenu *animeListContextMenu=new QMenu(animeListView);
    animeListContextMenu->addMenu(addSubMenu);
    animeListContextMenu->addAction(act_getDetailInfo);
    animeListContextMenu->addAction(act_updateDetailInfo);
    animeListContextMenu->addAction(act_delete);
    animeListContextMenu->addMenu(orderSubMenu);
    animeListContextMenu->addAction(act_batch);

    QAction *menuSep = new QAction(this);
    menuSep->setSeparator(true);
    static QVector<QAction *> scriptActions;
    QObject::connect(animeListView,&QListView::customContextMenuRequested,[=](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        for(QAction *act : scriptActions)
            animeListContextMenu->removeAction(act);
        animeListContextMenu->removeAction(menuSep);
        qDeleteAll(scriptActions);
        scriptActions.clear();
        if(selection.size()>0)
        {
            Anime *currentAnime = animeModel->getAnime(selection.indexes().first());
            auto script = GlobalObjects::scriptManager->getScript(currentAnime->scriptId());
            if(script)
            {
                LibraryScript *libScript = static_cast<LibraryScript *>(script.data());
                const auto &menuItems = libScript->getMenuItems();
                if(menuItems.size()>0)
                {
                    animeListContextMenu->addAction(menuSep);
                    for(auto &p : menuItems)
                    {
                        QAction *scriptAct = new QAction(p.first);
                        scriptAct->setData(p.second);
                        animeListContextMenu->addAction(scriptAct);
                        scriptActions.append(scriptAct);
                    }
                }
            }

        }
        animeListContextMenu->exec(QCursor::pos());
    });
    QObject::connect(animeListContextMenu, &QMenu::triggered, this, [=](QAction *act){
        if(!act->data().isNull())
        {
            QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
            if(selection.size()==0)return;
            Anime *currentAnime = animeModel->getAnime(selection.indexes().first());
            GlobalObjects::animeProvider->menuClick(act->data().toString(), currentAnime);
        }
    });
    QObject::connect(animeListView->selectionModel(), &QItemSelectionModel::selectionChanged,[=](){
        bool hasSelection = !animeListView->selectionModel()->selection().isEmpty();
        act_delete->setEnabled(hasSelection);
        act_updateDetailInfo->setEnabled(hasSelection);
        act_getDetailInfo->setEnabled(hasSelection);
    });

    labelView=new LabelTreeView(splitter);
    labelView->setObjectName(QStringLiteral("LabelView"));
    labelView->setProperty("cScrollStyle", true);
    labelView->setAnimated(true);
    labelView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    labelView->header()->hide();
    labelView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    labelView->setFont(QFont(GlobalObjects::normalFont,12));
    labelView->setIndentation(16*logicalDpiX()/96);
    LabelItemDelegate *labelItemDelegate = new LabelItemDelegate(this);
    labelView->setItemDelegate(labelItemDelegate);
    labelProxyModel = new LabelProxyModel(this);
    labelProxyModel->setSourceModel(LabelModel::instance());
    labelProxyModel->setRecursiveFilteringEnabled(true);
    labelView->setModel(labelProxyModel);
    labelView->setSortingEnabled(true);
    labelProxyModel->sort(0, Qt::AscendingOrder);
    labelProxyModel->setFilterKeyColumn(0);
    labelView->setContextMenuPolicy(Qt::ActionsContextMenu);

    LabelModel::instance()->loadLabels();
    auto expand = [=](){
        labelView->expand(labelProxyModel->index(0,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(1,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(2,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(3,0,QModelIndex()));
    };
    expand();

    auto setBrushColor = [=](LabelModel::BrushType bType, const QColor &color){
        LabelModel::instance()->setBrushColor(bType, color);
        expand();
    };
    QObject::connect(labelView, &LabelTreeView::topLevelColorChanged, [=](const QColor &color){
        setBrushColor(LabelModel::BrushType::TopLevel, color);
    });
    QObject::connect(labelView, &LabelTreeView::childLevelColorChanged, [=](const QColor &color){
        setBrushColor(LabelModel::BrushType::ChildLevel, color);
    });
    QObject::connect(labelView, &LabelTreeView::countFColorChanged, labelItemDelegate, &LabelItemDelegate::setPenColor);
    QObject::connect(labelView, &LabelTreeView::countBColorChanged, labelItemDelegate, &LabelItemDelegate::setBrushColor);


    AnimeFilterBox *filterBox=new AnimeFilterBox(this);
    filterBox->resize(240*logicalDpiX()/96, filterBox->height());
    QObject::connect(filterBox,&AnimeFilterBox::filterChanged,[=](int type,const QString &str){
        if(type==4)
        {
            labelProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
            labelProxyModel->setFilterRegExp(str);
        }
        else
        {
            proxyModel->setFilter(type, str);
        }
    });

    QAction *actDeleteTag=new QAction(tr("Delete"),this);
    QObject::connect(actDeleteTag,&QAction::triggered,[this](){
        QItemSelection selection=labelView->selectionModel()->selection();
        if(selection.size()==0)return;
        LabelModel::instance()->removeTag(labelProxyModel->mapSelectionToSource(selection).indexes().first());
    });
    QAction *actCopyTag=new QAction(tr("Copy"),this);
    QObject::connect(actCopyTag,&QAction::triggered,[this](){
        QItemSelection selection=labelView->selectionModel()->selection();
        if(selection.size()==0)return;
        QModelIndex index = labelProxyModel->mapToSource(selection.indexes().first());
        const TagNode *tag = (const TagNode *)LabelModel::instance()->data(index, TagNodeRole).value<void *>();
        QApplication::clipboard()->setText(tag->tagFilter);
    });
    labelView->addAction(actCopyTag);
    labelView->addAction(actDeleteTag);

    QLabel *totalCountLabel=new QLabel(animeContainer);
    totalCountLabel->setFont(QFont(GlobalObjects::normalFont,12));
    totalCountLabel->setObjectName(QStringLiteral("LibraryCountTip"));
    QPushButton *loadMoreButton=new QPushButton(tr("Continue to load"),this);
    QObject::connect(loadMoreButton,&QPushButton::clicked,[=](){
        animeModel->fetchMore(QModelIndex());
    });
    QObject::connect(proxyModel,&AnimeFilterProxyModel::animeMessage,this, [=](const QString &msg, bool hasMore){
        totalCountLabel->setText(msg);
        !animeViewing && hasMore? loadMoreButton->show():loadMoreButton->hide();
    });

    QPushButton *backButton =  new QPushButton(animeContainer);
    backButton->setObjectName(QStringLiteral("AnimeDetailBack"));
    GlobalObjects::iconfont.setPointSize(14);
    backButton->setFont(GlobalObjects::iconfont);
    backButton->setText(QChar(0xe69b));
    backButton->hide();

    QLabel *selectedLabelTip = new QLabel(animeContainer);
    selectedLabelTip->setFont(QFont(GlobalObjects::normalFont,12));
    selectedLabelTip->setObjectName(QStringLiteral("SelectedLabelTip"));

    QHBoxLayout *toolbuttonHLayout=new QHBoxLayout();
    toolbuttonHLayout->addWidget(backButton);
    toolbuttonHLayout->addWidget(totalCountLabel);
    toolbuttonHLayout->addWidget(loadMoreButton);
    toolbuttonHLayout->addSpacing(5*logicalDpiX()/96);
    toolbuttonHLayout->addWidget(selectedLabelTip);
    toolbuttonHLayout->addStretch(1);
    toolbuttonHLayout->addWidget(filterBox);

    QVBoxLayout *containerVLayout=new QVBoxLayout(animeContainer);
    containerVLayout->setContentsMargins(10*logicalDpiX()/96,10*logicalDpiY()/96,0,10*logicalDpiY()/96);
    containerVLayout->addLayout(toolbuttonHLayout);
    QStackedLayout *viewSLayout = new QStackedLayout;
    viewSLayout->addWidget(animeListView);
    viewSLayout->addWidget(detailPage);
    containerVLayout->addLayout(viewSLayout);

    auto refreshLabelFilter = [=](){
        auto indexes(labelProxyModel->mapSelectionToSource(labelView->selectionModel()->selection()).indexes());
        SelectedLabelInfo selectedInfo;
        LabelModel::instance()->selectedLabelList(indexes, selectedInfo);
        QString tagStr(selectedInfo.customTags.join(','));
        QString elidedLastLine = selectedLabelTip->fontMetrics().
                elidedText(tagStr, Qt::ElideRight, animeListView->width()-filterBox->width()-totalCountLabel->width()-10*logicalDpiX()/96);
        selectedLabelTip->setText(elidedLastLine);
        proxyModel->setTags(std::move(selectedInfo));
    };
    QObject::connect(LabelModel::instance(), &LabelModel::tagCheckedChanged, refreshLabelFilter);
    QObject::connect(labelView->selectionModel(), &QItemSelectionModel::selectionChanged, refreshLabelFilter);

    QObject::connect(detailPage,&AnimeDetailInfoPage::playFile,this,&LibraryWindow::playFile);
    QObject::connect(itemDelegate,&AnimeItemDelegate::ItemClicked,[=](const QModelIndex &index){
        Anime * anime = animeModel->getAnime(static_cast<AnimeFilterProxyModel *>(animeListView->model())->mapToSource(index));
        emit switchBackground(anime->rawCover(), true);
        detailPage->setAnime(anime);
        backButton->show();
        selectedLabelTip->hide();
        totalCountLabel->hide();
        if(animeModel->canFetchMore(QModelIndex())) loadMoreButton->hide();
        filterBox->hide();
        viewSLayout->setCurrentIndex(1);
        animeViewing = true;
    });
    QObject::connect(backButton, &QPushButton::clicked, this, [=](){
        emit switchBackground(QPixmap(), false);
        backButton->hide();
        selectedLabelTip->show();
        totalCountLabel->show();
        filterBox->show();
        if(animeModel->canFetchMore(QModelIndex())) loadMoreButton->show();
        viewSLayout->setCurrentIndex(0);
        animeViewing = false;
    });
    splitter->addWidget(labelView);
    splitter->addWidget(animeContainer);
    splitter->setHandleWidth(1);

    //splitter->setCollapsible(0,false);
    splitter->setCollapsible(1,false);
    splitter->setStretchFactor(0,2);
    splitter->setStretchFactor(1,3);

    QVBoxLayout *libraryVLayout=new QVBoxLayout(this);
    libraryVLayout->setContentsMargins(0,0,10*logicalDpiX()/96,0);
    libraryVLayout->addWidget(splitter);

    QObject::connect(splitter, &QSplitter::splitterMoved, this, [libraryVLayout, this](){
        if(splitter->sizes()[0] == 0) libraryVLayout->setContentsMargins(5*logicalDpiX()/96,0,10*logicalDpiX()/96,0);
        else libraryVLayout->setContentsMargins(0,0,10*logicalDpiX()/96,0);
    });
    QByteArray splitterState = GlobalObjects::appSetting->value("Library/SplitterState").toByteArray();
    if(!splitterState.isNull())
    {
        splitter->restoreState(splitterState);
        if(splitter->sizes()[0] == 0)  libraryVLayout->setContentsMargins(5*logicalDpiX()/96,0,10*logicalDpiX()/96,0);
    }

    animeModel->init();
}

void LibraryWindow::beforeClose()
{
    GlobalObjects::appSetting->setValue("Library/SortOrderAscending", proxyModel->isAscending());
    GlobalObjects::appSetting->setValue("Library/SortOrderType", (int)proxyModel->getOrderType());
    GlobalObjects::appSetting->setValue("Library/SplitterState", splitter->saveState());
}

void LibraryWindow::searchAddAnime(Anime *srcAnime)
{
    AnimeSearch animeSearch(srcAnime,this);
    while(animeSearch.exec()==QDialog::Accepted)
    {
        Anime *nAnime = new Anime;
        QString scriptName(GlobalObjects::scriptManager->getScript(animeSearch.curSelectedAnime.scriptId)->name());
        showMessage(tr("Fetching Info from %1").arg(scriptName), NM_PROCESS | NM_DARKNESS_BACK);
        ScriptState state = GlobalObjects::animeProvider->getDetail(animeSearch.curSelectedAnime, nAnime);
        if(state)
        {
            QStringList tags;
            if(srcAnime)
            {
                QString animeName(AnimeWorker::instance()->addAnime(srcAnime, nAnime));
                Anime *tAnime = AnimeWorker::instance()->getAnime(animeName);
                if(tAnime)
                {
                    showMessage(tr("Fetching Tags from %1").arg(scriptName), NM_PROCESS | NM_DARKNESS_BACK);
                    GlobalObjects::animeProvider->getTags(tAnime, tags);
                    if(tags.size()>0)
                    {
                        LabelModel::instance()->addCustomTags(tAnime->name(), tags);
                    }
                }
            }
            else
            {
                if(AnimeWorker::instance()->addAnime(nAnime))
                {
                    showMessage(tr("Fetching Tags from %1").arg(scriptName), NM_PROCESS | NM_DARKNESS_BACK);
                    GlobalObjects::animeProvider->getTags(nAnime, tags);
                    if(tags.size()>0)
                    {
                        LabelModel::instance()->addCustomTags(nAnime->name(), tags);
                    }
                }
            }
            showMessage(tr("Fetch Down"), NotifyMessageFlag::NM_HIDE);
            break;
        }
        else
        {
            showMessage(state.info, NM_ERROR | NM_HIDE);
            delete nAnime;
        }
    }
}

void LibraryWindow::showEvent(QShowEvent *)
{
    static bool animeModelActive = false;
    if(!animeModelActive)
    {
        QTimer::singleShot(0, [this](){
            animeModelActive = true;
            animeModel->setActive(true);
            animeModelActive = false;
        });
    }
}

void LibraryWindow::hideEvent(QHideEvent *)
{
    animeModel->setActive(false);
}

void LibraryWindow::showMessage(const QString &content, int flag)
{
    dialogTip->showMessage(content, flag);
}

AnimeFilterBox::AnimeFilterBox(QWidget *parent)
    : QLineEdit(parent)
    , filterTypeGroup(new QActionGroup(this))
{
    setClearButtonEnabled(true);
    setFont(QFont(GlobalObjects::normalFont,12));

    QMenu *menu = new QMenu(this);

    filterTypeGroup->setExclusive(true);
    QAction *filterTitle = menu->addAction(tr("Title"));
    filterTitle->setData(QVariant(int(0)));
    filterTitle->setCheckable(true);
    filterTitle->setChecked(true);
    filterTypeGroup->addAction(filterTitle);

    QAction *filterSummary = menu->addAction(tr("Summary"));
    filterSummary->setData(QVariant(int(1)));
    filterSummary->setCheckable(true);
    filterTypeGroup->addAction(filterSummary);

    QAction *filterStaff = menu->addAction(tr("Staff"));
    filterStaff->setData(QVariant(int(2)));
    filterStaff->setCheckable(true);
    filterTypeGroup->addAction(filterStaff);

    QAction *filterCrt = menu->addAction(tr("Character"));
    filterCrt->setData(QVariant(int(3)));
    filterCrt->setCheckable(true);
    filterTypeGroup->addAction(filterCrt);

    QAction *filterTag = menu->addAction(tr("Tag"));
    filterTag->setData(QVariant(int(4)));
    filterTag->setCheckable(true);
    filterTypeGroup->addAction(filterTag);

    connect(filterTypeGroup, &QActionGroup::triggered,[this](QAction *act){
        emit filterChanged(act->data().toInt(),this->text());
    });
    connect(this, &QLineEdit::textChanged, [this](const QString &text){
        emit filterChanged(filterTypeGroup->checkedAction()->data().toInt(),text);
    });


    QToolButton *optionsButton = new QToolButton;

    optionsButton->setCursor(Qt::ArrowCursor);

    optionsButton->setFocusPolicy(Qt::NoFocus);
    optionsButton->setObjectName(QStringLiteral("AnimeFilterOptionButton"));
    GlobalObjects::iconfont.setPointSize(14);
    optionsButton->setFont(GlobalObjects::iconfont);
    optionsButton->setText(QChar(0xe609));
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(optionsButton);
    addAction(optionsAction, QLineEdit::LeadingPosition);
}


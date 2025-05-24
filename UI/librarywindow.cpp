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
#include <QActionGroup>
#include <QWidgetAction>
#include <QSplitter>
#include <QApplication>
#include <QClipboard>

#include "UI/ela/ElaMenu.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/lazycontainer.h"
#include "globalobjects.h"
#include "Common/notifier.h"
#include "animesearch.h"
#include "inputdialog.h"
#include "widgets/dialogtip.h"
#include "widgets/fonticonbutton.h"
#include "animedetailinfopage.h"
#include "animebatchaction.h"
#include "Extension/Script/scriptmanager.h"
#include "Extension/Script/libraryscript.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/animeitemdelegate.h"
#include "MediaLibrary/animeprovider.h"
#include "MediaLibrary/animemodel.h"
#include "MediaLibrary/labelmodel.h"
#include "MediaLibrary/labelitemdelegate.h"
#include "MediaLibrary/animefilterproxymodel.h"
#define TagNodeRole Qt::UserRole+3


LibraryWindow::LibraryWindow(QWidget *parent) : QWidget(parent), animeViewing(false)
{
    setObjectName(QStringLiteral("LibraryWindow"));
    setAttribute(Qt::WA_StyledBackground, true);
    dialogTip = new DialogTip(this);
    dialogTip->raise();
    dialogTip->hide();
    Notifier::getNotifier()->addNotify(Notifier::LIBRARY_NOTIFY, this);

    initUI();
    animeModel->init();
}

void LibraryWindow::initUI()
{
    QWidget *baseContainer = new QWidget(this);
    splitter = new QSplitter(baseContainer);

    initAnimeView();
    initLabelView();
    QLayout *btnHLayout = initLibrarayBtns(baseContainer);

    splitter->setObjectName(QStringLiteral("LabelSplitter"));
    splitter->addWidget(labelView);
    splitter->addWidget(animeListView);
    splitter->setHandleWidth(1);
    splitter->setCollapsible(0, true);
    splitter->setCollapsible(1, false);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    QByteArray splitterState = GlobalObjects::appSetting->value("Library/SplitterState").toByteArray();
    if (!splitterState.isNull())
    {
        splitter->restoreState(splitterState);
    }
    bool filterChecked = GlobalObjects::appSetting->value("Library/FilterChecked", false).toBool();
    if (filterChecked)
    {
        filterCheckBtn->setChecked(true);
        splitter->setContentsMargins(0, 0, 10, 0);
        labelView->show();
    }
    else
    {
        labelView->hide();
        splitter->setContentsMargins(10, 0, 10, 0);
    }

    QGridLayout *baseGLayout = new QGridLayout(baseContainer);
    baseGLayout->setContentsMargins(4, 4, 4, 4);
    baseGLayout->setVerticalSpacing(8);
    baseGLayout->addLayout(btnHLayout, 0, 0);
    baseGLayout->addWidget(splitter, 1, 0);
    baseGLayout->setRowStretch(1, 1);

    QStackedLayout *viewSLayout = new QStackedLayout(this);
    viewSLayout->addWidget(baseContainer);
    viewSLayout->addWidget(new LazyContainer(this, viewSLayout, [this](){
        detailPage = new AnimeDetailInfoPage(this);
        QObject::connect(detailPage, &AnimeDetailInfoPage::playFile, this, &LibraryWindow::playFile);
        QObject::connect(detailPage, &AnimeDetailInfoPage::back, this, [=](){
            emit switchBackground(QPixmap(), false);
            static_cast<QStackedLayout *>(this->layout())->setCurrentIndex(0);
            animeViewing = false;
        });
        return detailPage;
    }));

}

QLayout *LibraryWindow::initLibrarayBtns(QWidget *parent)
{
    const int btnMinWidth = 90;
    FontIconButton *filterBtn = new FontIconButton(QChar(0xe634), tr("Filter"), 12, 10, 4, parent);
    filterBtn->setMinimumWidth(btnMinWidth);
    filterBtn->setObjectName(QStringLiteral("LibraryButton"));
    filterBtn->setContentsMargins(4, 2, 4, 2);
    filterBtn->setCheckable(true);
    this->filterCheckBtn = filterBtn;
    FontIconButton *addBtn = new FontIconButton(QChar(0xec18), tr("Add"), 12, 10, 4, parent);
    addBtn->setMinimumWidth(btnMinWidth);
    addBtn->setObjectName(QStringLiteral("LibraryButton"));
    addBtn->setContentsMargins(4, 2, 4, 2);
    FontIconButton *sortBtn = new FontIconButton(QChar(0xe69d), tr("Sort By Add Time ") + QChar(0x2193), 12, 10, 4, parent);
    sortBtn->setMinimumWidth(btnMinWidth);
    sortBtn->setObjectName(QStringLiteral("LibraryButton"));
    sortBtn->setContentsMargins(4, 2, 4, 2);
    FontIconButton *batchBtn = new FontIconButton(QChar(0xe8ac), tr("Batch Operation"), 12, 10, 4, parent);
    batchBtn->setMinimumWidth(btnMinWidth);
    batchBtn->setObjectName(QStringLiteral("LibraryButton"));
    batchBtn->setContentsMargins(4, 2, 4, 2);

    QObject::connect(filterBtn, &QPushButton::clicked, this, [=](bool checked){
        if (checked && isMovedCollapse)
        {
            splitter->setSizes({width() /4, width()/4*3});
            isMovedCollapse = false;
        }
        labelView->setVisible(checked);
        splitter->setContentsMargins(checked ? 0 : 10, 0, 10, 0);
        GlobalObjects::appSetting->setValue("Library/FilterChecked", checked);
    });
    QObject::connect(this, &LibraryWindow::updateFilterCounter, this, [=](int c){
        filterBtn->setText(c > 0 ? tr("Filter(%1)").arg(c) : tr("Filter"));
    });
    QObject::connect(splitter, &QSplitter::splitterMoved, this, [=](){
        int leftSize = splitter->sizes()[0];
        if (leftSize == 0)
        {
            filterBtn->setChecked(false);
            labelView->setVisible(false);
            splitter->setContentsMargins(10, 0, 10, 0);
            GlobalObjects::appSetting->setValue("Library/FilterChecked", false);
            isMovedCollapse = true;
        }
    });

    QObject::connect(batchBtn, &QPushButton::clicked, this, [=](){
        AnimeBatchAction batchAction(animeModel, this);
        batchAction.exec();
    });

    QMenu *addSubMenu = new ElaMenu(addBtn);
    QAction *actSearchAdd = addSubMenu->addAction(tr("Search Add"));
    QObject::connect(actSearchAdd, &QAction::triggered, this, [=](){
        searchAddAnime();
    });
    QAction *actDirectAdd = addSubMenu->addAction(tr("Direct Add"));
    QObject::connect(actDirectAdd, &QAction::triggered, this, [this](){
        LineInputDialog input(tr("Add Anime"), tr("Anime Name"), "", "DialogSize/DirectAddAnime", false, this);
        if (QDialog::Accepted == input.exec())
        {
            AnimeWorker::instance()->addAnime(input.text);
        }
    });
    addBtn->setMenu(addSubMenu);

    QMenu *orderSubMenu = new ElaMenu(sortBtn);

    QActionGroup *ascDesc = new QActionGroup(orderSubMenu);
    QAction *actAsc = ascDesc->addAction(tr("Ascending"));
    QAction *actDesc = ascDesc->addAction(tr("Descending"));
    actAsc->setCheckable(true);
    actDesc->setCheckable(true);
    actDesc->setChecked(true);

    QActionGroup *orderTypes = new QActionGroup(orderSubMenu);
    QAction *actOrderAddTime = orderTypes->addAction(tr("Add Time"));
    QAction *actOrderName = orderTypes->addAction(tr("Anime Name"));
    QAction *actOrderDate = orderTypes->addAction(tr("Air Date"));
    actOrderAddTime->setCheckable(true);
    actOrderName->setCheckable(true);
    actOrderDate->setCheckable(true);
    actOrderAddTime->setChecked(true);

    QObject::connect(ascDesc, &QActionGroup::triggered, this, [=](QAction *act){
        proxyModel->setAscending(act==actAsc);
        sortBtn->setText(tr("Sort By %1 %2").arg(orderTypes->checkedAction()->text()).arg(act == actAsc? QChar(0x2191) : QChar(0x2193)));
    });
    bool orderAsc = GlobalObjects::appSetting->value("Library/SortOrderAscending", false).toBool();
    (orderAsc ? actAsc : actDesc)->trigger();

    static QList<QAction *> acts({actOrderAddTime, actOrderName, actOrderDate});
    QObject::connect(orderTypes, &QActionGroup::triggered, this, [=](QAction *act){
        proxyModel->setOrder((AnimeFilterProxyModel::OrderType)acts.indexOf(act));
        sortBtn->setText(tr("Sort By %1 %2").arg(act->text()).arg(ascDesc->checkedAction() == actAsc? QChar(0x2191) : QChar(0x2193)));
    });
    AnimeFilterProxyModel::OrderType orderType =
        AnimeFilterProxyModel::OrderType(GlobalObjects::appSetting->value("Library/SortOrderType", (int)AnimeFilterProxyModel::OrderType::O_AddTime).toInt());
    if (orderType != AnimeFilterProxyModel::OrderType::O_AddTime)
        acts[(int)orderType]->trigger();

    orderSubMenu->addAction(actAsc);
    orderSubMenu->addAction(actDesc);
    orderSubMenu->addSeparator();
    orderSubMenu->addAction(actOrderAddTime);
    orderSubMenu->addAction(actOrderName);
    orderSubMenu->addAction(actOrderDate);
    sortBtn->setMenu(orderSubMenu);


    AnimeFilterBox *filterBox = new AnimeFilterBox(parent);
    filterBox->setFixedWidth(130);
    filterBox->setFixedHeight(filterBtn->sizeHint().height());
    QObject::connect(filterBox, &AnimeFilterBox::filterChanged, this, [=](int type, const QString &str){
        proxyModel->setFilter(type, str);
    });

    QLabel *totalCountLabel = new QLabel(parent);
    totalCountLabel->setFont(QFont(GlobalObjects::normalFont, 10));
    totalCountLabel->setObjectName(QStringLiteral("LibraryCountTip"));
    QObject::connect(proxyModel, &AnimeFilterProxyModel::animeMessage, this, [=](const QString &msg, bool hasMore){
        totalCountLabel->setText(msg);
    });

    QHBoxLayout *btnHLayout = new QHBoxLayout;
    btnHLayout->setContentsMargins(8, 4, 8, 4);
    btnHLayout->setSpacing(4);
    btnHLayout->addWidget(filterBtn);
    btnHLayout->addStretch(1);
    btnHLayout->addWidget(totalCountLabel);
    btnHLayout->addWidget(addBtn);
    btnHLayout->addWidget(sortBtn);
    btnHLayout->addWidget(batchBtn);
    btnHLayout->addStretch(1);
    btnHLayout->addWidget(filterBox);

    return btnHLayout;
}

void LibraryWindow::initAnimeView()
{
    AnimeItemDelegate *itemDelegate = new AnimeItemDelegate(this);
    QTimer *refreshBlockTimer = new QTimer(this);
    QObject::connect(refreshBlockTimer, &QTimer::timeout, this, [=](){
        itemDelegate->setBlockCoverFetch(false);
        animeListView->update();
        refreshBlockTimer->stop();
    });
    QObject::connect(itemDelegate, &AnimeItemDelegate::ItemClicked, this, [=](const QModelIndex &index){
        Anime * anime = animeModel->getAnime(static_cast<AnimeFilterProxyModel *>(animeListView->model())->mapToSource(index));
        emit switchBackground(anime->rawCover(), true);
        if (!detailPage)
        {
            static_cast<LazyContainer*>(static_cast<QStackedLayout*>(layout())->widget(1))->init();
        }
        detailPage->setAnime(anime);
        static_cast<QStackedLayout *>(this->layout())->setCurrentIndex(1);
        animeViewing = true;
    });


    animeListView = new AnimeListView(this);
    animeListView->setObjectName(QStringLiteral("AnimeListView"));
    animeListView->setViewMode(QListView::IconMode);
    animeListView->setUniformItemSizes(true);
    animeListView->setResizeMode(QListView::Adjust);
    animeListView->setMovement(QListView::Static);
    animeListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    animeListView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    animeListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    animeListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    animeListView->setItemDelegate(itemDelegate);
    animeListView->setMouseTracking(true);
    animeListView->setContextMenuPolicy(Qt::CustomContextMenu);
    new FloatScrollBar(animeListView->verticalScrollBar(), animeListView);
    animeModel = new AnimeModel(this);
    proxyModel=new AnimeFilterProxyModel(animeModel, this);
    proxyModel->setSourceModel(animeModel);
    animeListView->setModel(proxyModel);

    QObject::connect(animeListView->verticalScrollBar(), &QScrollBar::valueChanged, this, [=]() {
        refreshBlockTimer->stop();
        refreshBlockTimer->start(10);
        itemDelegate->setBlockCoverFetch(true);
    });

    QObject::connect(animeListView, &AnimeListView::itemBorderColorChanged, itemDelegate, &AnimeItemDelegate::setBorderColor);

    QAction *actDelete = new QAction(tr("Delete"), this);
    QObject::connect(actDelete, &QAction::triggered, this, [=](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0) return;
        Anime *anime = animeModel->getAnime(selection.indexes().first());
        if (anime)
        {
            if (detailPage && detailPage->curAnime() == anime) detailPage->setAnime(nullptr);
            LabelModel::instance()->removeTag(anime->name(), anime->airDate(), anime->scriptId());
            animeModel->deleteAnime(selection.indexes().first());
        }
    });
    actDelete->setEnabled(false);

    QAction *actGetDetailInfo = new QAction(tr("Search Details"),this);
    QObject::connect(actGetDetailInfo, &QAction::triggered, this, [=](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if (selection.empty()) return;
        searchAddAnime(animeModel->getAnime(selection.indexes().first()));
    });
    actGetDetailInfo->setEnabled(false);

    QAction *actUpdateDetailInfo = new QAction(tr("Update"),this);
    QObject::connect(actUpdateDetailInfo, &QAction::triggered, this, [=](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if (selection.empty()) return;
        Anime *currentAnime = animeModel->getAnime(selection.indexes().first());
        if (currentAnime->scriptId().isEmpty())
        {
            showMessage(tr("No Script ID, Search For Detail First"), NM_HIDE);
            return;
        }
        QSharedPointer<ScriptBase> script = GlobalObjects::scriptManager->getScript(currentAnime->scriptId());
        if (!script)
        {
            showMessage(tr("Script \"%1\" not exist").arg(currentAnime->scriptId()), NM_HIDE);
            return;
        }
        QString scriptName(script->name());
        showMessage(tr("Fetching Info from %1").arg(scriptName),  NM_PROCESS | NM_DARKNESS_BACK);
        Anime *nAnime = new Anime;
        ScriptState state = GlobalObjects::animeProvider->getDetail(currentAnime->toLite(), nAnime);
        if (!state)
        {
            showMessage(state.info, NM_HIDE | NM_ERROR);
            delete nAnime;
        }
        else
        {
            QString animeName(AnimeWorker::instance()->addAnime(currentAnime, nAnime));
            auto &tagAnimes = LabelModel::instance()->customTags();
            bool hasTag = false;
            for (const auto &animes : tagAnimes)
            {
                if (animes.contains(animeName))
                {
                    hasTag = true;
                    break;
                }
            }
            if (!hasTag)
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
    actUpdateDetailInfo->setEnabled(false);

    QMenu *animeListContextMenu = new ElaMenu(animeListView);
    animeListContextMenu->addAction(actGetDetailInfo);
    animeListContextMenu->addAction(actUpdateDetailInfo);
    animeListContextMenu->addAction(actDelete);

    QAction *menuSep = new QAction(this);
    menuSep->setSeparator(true);
    static QVector<QAction *> scriptActions;
    QObject::connect(animeListView, &QListView::customContextMenuRequested, this, [=](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if (selection.empty()) return;
        for (QAction *act : scriptActions)
        {
            animeListContextMenu->removeAction(act);
        }
        animeListContextMenu->removeAction(menuSep);
        qDeleteAll(scriptActions);
        scriptActions.clear();
        Anime *currentAnime = animeModel->getAnime(selection.indexes().first());
        auto script = GlobalObjects::scriptManager->getScript(currentAnime->scriptId());
        if (script)
        {
            LibraryScript *libScript = static_cast<LibraryScript *>(script.data());
            const auto &menuItems = libScript->getMenuItems();
            if (!menuItems.empty())
            {
                animeListContextMenu->addAction(menuSep);
                for (auto &p : menuItems)
                {
                    QAction *scriptAct = new QAction(p.first);
                    scriptAct->setData(p.second);
                    animeListContextMenu->addAction(scriptAct);
                    scriptActions.append(scriptAct);
                }
            }
        }
        animeListContextMenu->exec(QCursor::pos());
    });
    QObject::connect(animeListContextMenu, &QMenu::triggered, this, [=](QAction *act){
        if (!act->data().isNull())
        {
            QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
            if(selection.size()==0)return;
            Anime *currentAnime = animeModel->getAnime(selection.indexes().first());
            GlobalObjects::animeProvider->menuClick(act->data().toString(), currentAnime);
        }
    });
    QObject::connect(animeListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=](){
        bool hasSelection = !animeListView->selectionModel()->selection().isEmpty();
        actDelete->setEnabled(hasSelection);
        actUpdateDetailInfo->setEnabled(hasSelection);
        actGetDetailInfo->setEnabled(hasSelection);
    });
}

void LibraryWindow::initLabelView()
{
    LabelModel::instance()->waitLabelLoaded();
    labelView = new LabelTreeView(this);
    labelView->setObjectName(QStringLiteral("LabelView"));
    labelView->setAnimated(true);
    labelView->setMouseTracking(true);
    labelView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    labelView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    labelView->header()->hide();
    labelView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    labelView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    labelView->setFont(QFont(GlobalObjects::normalFont, 12));
    labelView->setIndentation(16);
    new FloatScrollBar(labelView->verticalScrollBar(), labelView);
    LabelItemDelegate *labelItemDelegate = new LabelItemDelegate(this);
    labelView->setItemDelegate(labelItemDelegate);
    labelProxyModel = new LabelProxyModel(this);
    labelProxyModel->setSourceModel(LabelModel::instance());
    labelProxyModel->setRecursiveFilteringEnabled(true);
    labelView->setModel(labelProxyModel);
    labelView->setSortingEnabled(true);
    labelProxyModel->sort(0, Qt::AscendingOrder);
    labelProxyModel->setFilterKeyColumn(0);
    labelView->setContextMenuPolicy(Qt::CustomContextMenu);

    QMenu *labelViewContextMenu = new ElaMenu(labelView);
    QAction *actDeleteTag = labelViewContextMenu->addAction(tr("Delete"));
    QObject::connect(actDeleteTag, &QAction::triggered, labelView, [=](){
        QItemSelection selection = labelView->selectionModel()->selection();
        if (selection.empty()) return;
        LabelModel::instance()->removeTag(labelProxyModel->mapSelectionToSource(selection).indexes().first());
    });
    QAction *actCopyTag = labelViewContextMenu->addAction(tr("Copy"));
    QObject::connect(actCopyTag, &QAction::triggered, labelView, [=](){
        QItemSelection selection = labelView->selectionModel()->selection();
        if (selection.empty()) return;
        QModelIndex index = labelProxyModel->mapToSource(selection.indexes().first());
        const TagNode *tag = (const TagNode *)LabelModel::instance()->data(index, TagNodeRole).value<void *>();
        QApplication::clipboard()->setText(tag->tagFilter);
    });
    QObject::connect(labelView, &LabelTreeView::customContextMenuRequested, labelView, [=](){
        QItemSelection selection = labelView->selectionModel()->selection();
        if (selection.empty()) return;
        QModelIndex index = labelProxyModel->mapToSource(selection.indexes().first());
        const TagNode *tag = (const TagNode *)LabelModel::instance()->data(index, TagNodeRole).value<void *>();
        actDeleteTag->setEnabled(tag->tagType == TagNode::TAG_CUSTOM);
        labelViewContextMenu->exec(QCursor::pos());
    });

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
    QObject::connect(labelItemDelegate, &LabelItemDelegate::openTagSearchEditor, [=](const QModelIndex &index){
        labelView->openPersistentEditor(index);
    });
    QObject::connect(labelItemDelegate, &LabelItemDelegate::tagSearchTextChanged, [=](const QString &text, const QModelIndex &index){
        labelProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        labelProxyModel->setFilterRegularExpression(text);
        if (text.isEmpty())
        {
            labelView->closePersistentEditor(index);
        }
    });
    auto refreshLabelFilter = [=](){
        auto indexes(labelProxyModel->mapSelectionToSource(labelView->selectionModel()->selection()).indexes());
        SelectedLabelInfo selectedInfo;
        LabelModel::instance()->selectedLabelList(indexes, selectedInfo);
        emit updateFilterCounter(selectedInfo.scriptTags.size() + selectedInfo.timeTags.size() + selectedInfo.epPathTags.size() + selectedInfo.customTags.size());
        proxyModel->setTags(std::move(selectedInfo));
    };
    QObject::connect(LabelModel::instance(), &LabelModel::tagCheckedChanged, refreshLabelFilter);
    QObject::connect(labelView->selectionModel(), &QItemSelectionModel::selectionChanged, refreshLabelFilter);
}

void LibraryWindow::beforeClose()
{
    GlobalObjects::appSetting->setValue("Library/SortOrderAscending", proxyModel->isAscending());
    GlobalObjects::appSetting->setValue("Library/SortOrderType", (int)proxyModel->getOrderType());
    if (!isMovedCollapse) GlobalObjects::appSetting->setValue("Library/SplitterState", splitter->saveState());
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

void LibraryWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        if(!detailPage->isHidden())
        {
            detailPage->back();
        }
    }
}

void LibraryWindow::showMessage(const QString &content, int flag, const QVariant &)
{
    dialogTip->showMessage(content, flag);
}

AnimeFilterBox::AnimeFilterBox(QWidget *parent)
    : KLineEdit(parent)
    , filterTypeGroup(new QActionGroup(this))
{
    setObjectName(QStringLiteral("FilterEdit"));
    setClearButtonEnabled(true);
    setFont(QFont(GlobalObjects::normalFont, 13));

    QMenu *menu = new ElaMenu(this);

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

    connect(filterTypeGroup, &QActionGroup::triggered, this, [this](QAction *act){
        emit filterChanged(act->data().toInt(),this->text());
    });
    connect(this, &QLineEdit::textChanged, [this](const QString &text){
        emit filterChanged(filterTypeGroup->checkedAction()->data().toInt(),text);
    });


    QToolButton *optionsButton = new QToolButton;

    optionsButton->setCursor(Qt::ArrowCursor);

    optionsButton->setFocusPolicy(Qt::NoFocus);
    optionsButton->setObjectName(QStringLiteral("FilterOptionButton"));
    GlobalObjects::iconfont->setPointSize(14);
    optionsButton->setFont(*GlobalObjects::iconfont);
    optionsButton->setText(QChar(0xea8a));
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(optionsButton);
    addAction(optionsAction, QLineEdit::LeadingPosition);
}


void AnimeListView::resizeEvent(QResizeEvent *event)
{
    QSize sz(this->itemDelegate()->sizeHint(QStyleOptionViewItem(), QModelIndex()));

    int w = event->size().width() - 10;
    int c = w / sz.width();
    if (c > 0)
    {
        int wInc = (w % sz.width()) / c;
        setGridSize(QSize(sz.width() + wInc, sz.height() + 10));
    }
    else
    {
        setGridSize(sz + QSize(10, 10));
    }
}

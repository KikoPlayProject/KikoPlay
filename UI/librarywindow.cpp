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
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QGraphicsBlurEffect>
#include <QMouseEvent>
#include <QScrollBar>
#include <QButtonGroup>
#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QSplitter>

#include "globalobjects.h"
#include "Common/notifier.h"
#include "animesearch.h"
#include "episodeeditor.h"
#include "widgets/dialogtip.h"
#include "animedetailinfopage.h"
#include "Script/scriptmanager.h"
#include "Script/libraryscript.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/animeitemdelegate.h"
#include "MediaLibrary/animeprovider.h"
#include "MediaLibrary/animemodel.h"
#include "MediaLibrary/labelmodel.h"
#include "MediaLibrary/labelitemdelegate.h"
#include "MediaLibrary/animefilterproxymodel.h"

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

LibraryWindow::LibraryWindow(QWidget *parent) : QWidget(parent)
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
    AnimeFilterProxyModel *proxyModel=new AnimeFilterProxyModel(animeModel, this);
    proxyModel->setSourceModel(animeModel);
    animeListView->setModel(proxyModel);

    QAction *act_delete=new QAction(tr("Delete"),this);
    QObject::connect(act_delete,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        Anime *anime = animeModel->getAnime(selection.indexes().first());
        if(anime)
        {
            GlobalObjects::animeLabelModel->removeTag(anime->name(), anime->airDate(), anime->scriptId());
            animeModel->deleteAnime(selection.indexes().first());
        }
    });
    act_delete->setEnabled(false);

    QAction *act_getDetailInfo=new QAction(tr("Search Details"),this);
    QObject::connect(act_getDetailInfo,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        searchAddAnime(animeModel->getAnime(selection.indexes().first()));
    });
    act_getDetailInfo->setEnabled(false);

    QAction *act_updateDetailInfo=new QAction(tr("Update"),this);
    QObject::connect(act_updateDetailInfo,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        Anime *currentAnime = animeModel->getAnime(selection.indexes().first());
        if(currentAnime->scriptId().isEmpty())
        {
            showMessage(tr("No Script ID, Search For Detail First"), NM_HIDE);
            return;
        }
        QString scriptName(GlobalObjects::scriptManager->getScript(currentAnime->scriptId())->name());
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
            auto &tagAnimes = GlobalObjects::animeLabelModel->customTags();
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
                        GlobalObjects::animeLabelModel->addCustomTags(tAnime->name(), tags);
                    }
                }
            }
            showMessage(tr("Fetch Down"), NotifyMessageFlag::NM_HIDE);
        }
    });
    act_updateDetailInfo->setEnabled(false);

    QAction *act_searchAdd=new QAction(tr("Add Anime"), this);
    QObject::connect(act_searchAdd,&QAction::triggered, this, [this](){
        searchAddAnime();
    });

    QMenu *animeListContextMenu=new QMenu(animeListView);
    animeListContextMenu->addAction(act_searchAdd);
    animeListContextMenu->addAction(act_getDetailInfo);
    animeListContextMenu->addAction(act_updateDetailInfo);
    animeListContextMenu->addAction(act_delete);
    QAction *menuSep = new QAction(this);
    menuSep->setSeparator(true);
    static QList<QAction *> scriptActions;
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
            auto scriptInfo = act->data().value<QPair<QString, QString>>();
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
    labelView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    labelView->header()->hide();
    labelView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    labelView->setFont(QFont(GlobalObjects::normalFont,12));
    labelView->setIndentation(16*logicalDpiX()/96);
    LabelItemDelegate *labelItemDelegate = new LabelItemDelegate(this);
    labelView->setItemDelegate(labelItemDelegate);
    labelProxyModel = new LabelProxyModel(this);
    labelProxyModel->setSourceModel(GlobalObjects::animeLabelModel);
    labelProxyModel->setRecursiveFilteringEnabled(true);
    labelView->setModel(labelProxyModel);
    labelView->setSortingEnabled(true);
    labelProxyModel->sort(0, Qt::AscendingOrder);
    labelProxyModel->setFilterKeyColumn(0);
    labelView->setContextMenuPolicy(Qt::ActionsContextMenu);

    auto setBrushColor = [=](LabelModel::BrushType bType, const QColor &color){
        GlobalObjects::animeLabelModel->setBrushColor(bType, color);
        labelView->expand(labelProxyModel->index(0,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(1,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(2,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(3,0,QModelIndex()));
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

    QAction *act_deleteTag=new QAction(tr("Delete"),this);
    QObject::connect(act_deleteTag,&QAction::triggered,[this](){
        QItemSelection selection=labelView->selectionModel()->selection();
        if(selection.size()==0)return;
        GlobalObjects::animeLabelModel->removeTag(labelProxyModel->mapSelectionToSource(selection).indexes().first());
    });
    labelView->addAction(act_deleteTag);

    QLabel *totalCountLabel=new QLabel(animeContainer);
    totalCountLabel->setFont(QFont(GlobalObjects::normalFont,12));
    totalCountLabel->setObjectName(QStringLiteral("LibraryCountTip"));
    QPushButton *loadMore=new QPushButton(tr("Continue to load"),this);
    QObject::connect(loadMore,&QPushButton::clicked,[=](){
        animeModel->fetchMore(QModelIndex());
    });
    QObject::connect(proxyModel,&AnimeFilterProxyModel::animeMessage,this, [=](const QString &msg, bool hasMore){
        totalCountLabel->setText(msg);
        hasMore? loadMore->show():loadMore->hide();
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
    toolbuttonHLayout->addWidget(loadMore);
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
        GlobalObjects::animeLabelModel->selectedLabelList(indexes, selectedInfo);
        QString tagStr(selectedInfo.customTags.join(','));
        QString elidedLastLine = selectedLabelTip->fontMetrics().
                elidedText(tagStr, Qt::ElideRight, animeListView->width()-filterBox->width()-totalCountLabel->width()-10*logicalDpiX()/96);
        selectedLabelTip->setText(elidedLastLine);
        proxyModel->setTags(std::move(selectedInfo));
    };
    QObject::connect(GlobalObjects::animeLabelModel, &LabelModel::tagCheckedChanged, refreshLabelFilter);
    QObject::connect(labelView->selectionModel(), &QItemSelectionModel::selectionChanged, refreshLabelFilter);

    QObject::connect(detailPage,&AnimeDetailInfoPage::playFile,this,&LibraryWindow::playFile);
    QObject::connect(itemDelegate,&AnimeItemDelegate::ItemClicked,[=](const QModelIndex &index){
        Anime * anime = animeModel->getAnime(static_cast<AnimeFilterProxyModel *>(animeListView->model())->mapToSource(index));
        emit switchBackground(anime->cover(), true);
        detailPage->setAnime(anime);
        backButton->show();
        selectedLabelTip->hide();
        totalCountLabel->hide();
        filterBox->hide();
        viewSLayout->setCurrentIndex(1);
    });
    QObject::connect(backButton, &QPushButton::clicked, this, [=](){
        emit switchBackground(QPixmap(), false);
        backButton->hide();
        selectedLabelTip->show();
        totalCountLabel->show();
        filterBox->show();
        viewSLayout->setCurrentIndex(0);
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
}

void LibraryWindow::beforeClose()
{
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
                        GlobalObjects::animeLabelModel->addCustomTags(tAnime->name(), tags);
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
                        GlobalObjects::animeLabelModel->addCustomTags(nAnime->name(), tags);
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
    static bool labelInited = false;
    if(!labelInited)
    {
        QTimer::singleShot(0,[this](){
            GlobalObjects::animeLabelModel->loadLabels();
            labelView->expand(labelProxyModel->index(0,0,QModelIndex()));
            labelView->expand(labelProxyModel->index(1,0,QModelIndex()));
            labelView->expand(labelProxyModel->index(2,0,QModelIndex()));
            labelView->expand(labelProxyModel->index(3,0,QModelIndex()));
        });
        labelInited = true;
    }
    animeModel->setActive(true);
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
    optionsButton->setObjectName(QStringLiteral("FilterOptionButton"));
    GlobalObjects::iconfont.setPointSize(14);
    optionsButton->setFont(GlobalObjects::iconfont);
    optionsButton->setText(QChar(0xe609));
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(optionsButton);
    addAction(optionsAction, QLineEdit::LeadingPosition);
}


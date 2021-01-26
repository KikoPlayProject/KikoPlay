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
#include "bangumisearch.h"
#include "bangumiupdate.h"
#include "episodeeditor.h"
#include "animedetailinfo.h"
#include "animedetailinfopage.h"
#include "MediaLibrary/animeitemdelegate.h"
#include "MediaLibrary/animelibrary.h"
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

LibraryWindow::LibraryWindow(QWidget *parent) : QWidget(parent), bgOn(true)
{
    setObjectName(QStringLiteral("LibraryWindow"));
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
    animeListView->setContextMenuPolicy(Qt::ActionsContextMenu);
    AnimeFilterProxyModel *proxyModel=new AnimeFilterProxyModel(this);
    proxyModel->setSourceModel(GlobalObjects::library->animeModel);
    animeListView->setModel(proxyModel);

    QAction *act_delete=new QAction(tr("Delete"),this);
    QObject::connect(act_delete,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        GlobalObjects::library->animeModel->deleteAnime(selection.indexes().first());
    });
    act_delete->setEnabled(false);

    QAction *act_getDetailInfo=new QAction(tr("Search for details"),this);
    QObject::connect(act_getDetailInfo,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        Anime * currentAnime = GlobalObjects::library->animeModel->getAnime(selection.indexes().first(),true);
        BangumiSearch bgmSearch(currentAnime,this);
        bgmSearch.exec();
    });
    act_getDetailInfo->setEnabled(false);

    QAction *act_updateDetailInfo=new QAction(tr("Update"),this);
    QObject::connect(act_updateDetailInfo,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        Anime * currentAnime = GlobalObjects::library->animeModel->getAnime(selection.indexes().first(),true);
        if(currentAnime->id.isEmpty())
        {
            QMessageBox::information(this,"KikoPlay",tr("No Bangumi ID, Search For Detail First"));
            return;
        }
        BangumiUpdate bgmUpdate(currentAnime,this);
        bgmUpdate.exec();
    });
    act_updateDetailInfo->setEnabled(false);

    animeListView->addAction(act_getDetailInfo);
    animeListView->addAction(act_updateDetailInfo);
    animeListView->addAction(act_delete);

    QObject::connect(animeListView->selectionModel(), &QItemSelectionModel::selectionChanged,[act_delete,act_getDetailInfo,act_updateDetailInfo,this](){
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
    LabelProxyModel *labelProxyModel = new LabelProxyModel(this);
    labelProxyModel->setSourceModel(GlobalObjects::library->labelModel);
    labelView->setModel(labelProxyModel);
    labelView->setSortingEnabled(true);
    labelProxyModel->sort(0, Qt::AscendingOrder);
    labelProxyModel->setFilterKeyColumn(0);
    labelView->setContextMenuPolicy(Qt::ActionsContextMenu);
    QObject::connect(labelView, &LabelTreeView::topLevelColorChanged, [this, labelProxyModel](const QColor &color){
        GlobalObjects::library->labelModel->setBrushColor(LabelModel::BrushType::TopLevel, color);
        labelView->expand(labelProxyModel->index(0,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(1,0,QModelIndex()));
    });
    QObject::connect(labelView, &LabelTreeView::childLevelColorChanged, [this, labelProxyModel](const QColor &color){
        GlobalObjects::library->labelModel->setBrushColor(LabelModel::BrushType::ChildLevel, color);
        labelView->expand(labelProxyModel->index(0,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(1,0,QModelIndex()));
    });
    QObject::connect(labelView, &LabelTreeView::countFColorChanged, labelItemDelegate, &LabelItemDelegate::setPenColor);
    QObject::connect(labelView, &LabelTreeView::countBColorChanged, labelItemDelegate, &LabelItemDelegate::setBrushColor);


    AnimeFilterBox *filterBox=new AnimeFilterBox(this);
    filterBox->resize(240*logicalDpiX()/96, filterBox->height());
    QObject::connect(filterBox,&AnimeFilterBox::filterChanged,[proxyModel,labelProxyModel](int type,const QString &str){
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
    QObject::connect(act_deleteTag,&QAction::triggered,[this,labelProxyModel](){
        QItemSelection selection=labelView->selectionModel()->selection();
        if(selection.size()==0)return;
        GlobalObjects::library->labelModel->removeTag(labelProxyModel->mapSelectionToSource(selection).indexes().first());
    });
    labelView->addAction(act_deleteTag);

    QMovie *loadingIcon=new QMovie(animeContainer);
    QLabel *loadingLabel=new QLabel(animeContainer);
    loadingLabel->setMovie(loadingIcon);
    loadingLabel->setFixedSize(36,36);
    loadingLabel->setScaledContents(true);
    loadingIcon->setFileName(":/res/images/loading-blocks.gif");
    loadingIcon->start();
    loadingLabel->hide();

    QLabel *totalCountLabel=new QLabel(animeContainer);
    totalCountLabel->setFont(QFont(GlobalObjects::normalFont,12));
    totalCountLabel->setObjectName(QStringLiteral("LibraryCountTip"));
    QPushButton *loadMore=new QPushButton(tr("Continue to load"),this);
    QObject::connect(loadMore,&QPushButton::clicked,[](){
        GlobalObjects::library->animeModel->fetchMore(QModelIndex());
    });
    QObject::connect(proxyModel,&AnimeFilterProxyModel::animeMessage,this,
                     [totalCountLabel,loadMore,loadingLabel](const QString &msg, int flag,bool hasMore){
        totalCountLabel->setText(msg);
        hasMore? loadMore->show():loadMore->hide();
        flag&PopMessageFlag::PM_PROCESS? loadingLabel->show():loadingLabel->hide();
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
    toolbuttonHLayout->addWidget(loadingLabel);
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

    QObject::connect(labelView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=](){
        QStringList tags;
        QSet<QString> times;
        GlobalObjects::library->labelModel->selLabelList(labelProxyModel->mapSelectionToSource(labelView->selectionModel()->selection()).indexes(),tags,times);
        proxyModel->setTags(tags);
        proxyModel->setTime(times);
        QStringList totalTag(times.begin(), times.end());
        totalTag.append(tags);
        QString tagStr(totalTag.join(','));
        QString elidedLastLine = selectedLabelTip->fontMetrics().
                elidedText(tagStr, Qt::ElideRight, animeListView->width()-filterBox->width()-totalCountLabel->width()-10*logicalDpiX()/96);
        selectedLabelTip->setText(elidedLastLine);
    });

    QTimer::singleShot(0,[labelProxyModel,this](){
        GlobalObjects::library->labelModel->refreshLabel();
        labelView->expand(labelProxyModel->index(0,0,QModelIndex()));
        labelView->expand(labelProxyModel->index(1,0,QModelIndex()));
    });

    QObject::connect(detailPage,&AnimeDetailInfoPage::playFile,this,&LibraryWindow::playFile);
    QObject::connect(itemDelegate,&AnimeItemDelegate::ItemClicked,[this, viewSLayout, backButton](const QModelIndex &index){
        Anime * anime = GlobalObjects::library->animeModel->getAnime(static_cast<AnimeFilterProxyModel *>(animeListView->model())->mapToSource(index),true);
        if(bgOn)
        {
            emit switchBackground(anime->coverPixmap, true);
            detailPage->setAnime(anime);
            backButton->show();
            viewSLayout->setCurrentIndex(1);
        }
        else
        {
            AnimeDetailInfo infoDialog(anime,this);
            QObject::connect(&infoDialog,&AnimeDetailInfo::playFile,this,&LibraryWindow::playFile);
            QRect geo(0,0,400*logicalDpiX()/96,600*logicalDpiY()/96);
            geo.moveCenter(this->geometry().center());
            infoDialog.move(mapToGlobal(geo.topLeft()));
            infoDialog.exec();
        }
    });
    QObject::connect(detailPage, &AnimeDetailInfoPage::setBackEnable, backButton, &QPushButton::setEnabled);
    QObject::connect(backButton, &QPushButton::clicked, this, [this, viewSLayout, backButton](){
        emit switchBackground(QPixmap(), false);
        backButton->hide();
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


void LibraryWindow::showEvent(QShowEvent *)
{
    GlobalObjects::library->animeModel->setActive(true);
}

void LibraryWindow::hideEvent(QHideEvent *)
{
    GlobalObjects::library->animeModel->setActive(false);
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


#include "librarywindow.h"
#include <QListView>
#include <QTreeView>
#include <QToolButton>
#include <QVBoxLayout>
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

#include "globalobjects.h"
#include "bangumisearch.h"
#include "bangumiupdate.h"
#include "episodeeditor.h"
#include "animedetailinfo.h"
#include "MediaLibrary/animeitemdelegate.h"
#include "MediaLibrary/animelibrary.h"
#include "MediaLibrary/animemodel.h"
#include "MediaLibrary/labelmodel.h"
#include "MediaLibrary/labelitemdelegate.h"
#include "MediaLibrary/animefilterproxymodel.h"

LibraryWindow::LibraryWindow(QWidget *parent) : QWidget(parent)
{
    AnimeItemDelegate *itemDelegate=new AnimeItemDelegate(this);
    QObject::connect(itemDelegate,&AnimeItemDelegate::ItemClicked,[this](const QModelIndex &index){
        Anime * anime = GlobalObjects::library->animeModel->getAnime(static_cast<AnimeFilterProxyModel *>(animeListView->model())->mapToSource(index),true);
        AnimeDetailInfo infoDialog(anime,this);
        QObject::connect(&infoDialog,&AnimeDetailInfo::playFile,this,&LibraryWindow::playFile);
        QRect geo(0,0,400,400);
        geo.moveCenter(this->geometry().center());
        infoDialog.move(geo.topLeft());
        infoDialog.exec();
    });

    animeListView=new QListView(this);
    animeListView->setObjectName(QStringLiteral("AnimesContent"));
    animeListView->setViewMode(QListView::IconMode);
    animeListView->setUniformItemSizes(true);
    animeListView->setResizeMode(QListView::Adjust);
    animeListView->setMovement(QListView::Static);
    animeListView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
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
        if(currentAnime->bangumiID==-1)
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

    tagCollapseButton=new QPushButton(this);
    GlobalObjects::iconfont.setPointSize(8);
    tagCollapseButton->setFont(GlobalObjects::iconfont);
    tagCollapseButton->setText(QChar(0xe6b1));
    tagCollapseButton->setFixedSize(12*logicalDpiX()/96,80*logicalDpiY()/96);
    tagCollapseButton->setObjectName(QStringLiteral("TagPanelCollapse"));
    tagCollapseButton->move(width()-tagCollapseButton->width(),(height()-tagCollapseButton->height())/2);
    QObject::connect(tagCollapseButton,&QPushButton::clicked,[this](){
        bool panelOpen=!labelView->isHidden();
        labelView->resize(width()/4,animeListView->height());
        QPropertyAnimation *moveAnime = new QPropertyAnimation(labelView, "pos");
        QPoint endPos(panelOpen?width():width()/4*3-tagCollapseButton->width(),animeListView->y()),
                startPos(panelOpen?labelView->x():width(),animeListView->y());
        moveAnime->setDuration(500);
        moveAnime->setEasingCurve(QEasingCurve::OutExpo);
        moveAnime->setStartValue(startPos);
        moveAnime->setEndValue(endPos);
        labelView->show();
        moveAnime->start(QAbstractAnimation::DeleteWhenStopped);
        tagCollapseButton->setText(panelOpen?QChar(0xe6b1):QChar(0xe943));
        QObject::connect(moveAnime,&QPropertyAnimation::finished,[this,panelOpen](){
            if(panelOpen)labelView->hide();
        });
    });

    labelView=new QTreeView(this);
    labelView->setObjectName(QStringLiteral("LabelView"));
    labelView->resize(width()/4,animeListView->height());
    labelView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    labelView->header()->hide();
    labelView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    labelView->setFont(QFont("Microsoft YaHei UI",12));
    labelView->setIndentation(16*logicalDpiX()/96);
    labelView->setItemDelegate(new LabelItemDelegate(this));
    labelView->hide();
    LabelProxyModel *labelProxyModel = new LabelProxyModel(this);
    labelProxyModel->setSourceModel(GlobalObjects::library->labelModel);
    labelView->setModel(labelProxyModel);
    labelView->setSortingEnabled(true);
    labelProxyModel->sort(0, Qt::AscendingOrder);
    labelProxyModel->setFilterKeyColumn(0);
    labelView->setContextMenuPolicy(Qt::ActionsContextMenu);

    AnimeFilterBox *filterBox=new AnimeFilterBox(this);
    filterBox->setMinimumWidth(300*logicalDpiX()/96);
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

    QObject::connect(labelView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this,proxyModel,labelProxyModel](){
        QStringList tags;
        QSet<QString> times;
        GlobalObjects::library->labelModel->selLabelList(labelProxyModel->mapSelectionToSource(labelView->selectionModel()->selection()).indexes(),tags,times);
        proxyModel->setTags(tags);
        proxyModel->setTime(times);
    });
    QTimer::singleShot(0,[](){
        GlobalObjects::library->labelModel->refreshLabel();
    });

    QMovie *loadingIcon=new QMovie(this);
    QLabel *loadingLabel=new QLabel(this);
    loadingLabel->setMovie(loadingIcon);
    loadingLabel->setFixedSize(36,36);
    loadingLabel->setScaledContents(true);
    loadingIcon->setFileName(":/res/images/loading-blocks.gif");
    loadingIcon->start();
    loadingLabel->hide();

    QLabel *totalCountLabel=new QLabel(this);
    totalCountLabel->setFont(QFont("Microsoft Yahei",12));
    QPushButton *loadMore=new QPushButton(tr("Continue to load"),this);
    QObject::connect(loadMore,&QPushButton::clicked,[](){
        GlobalObjects::library->animeModel->fetchMore(QModelIndex());
    });
    QObject::connect(proxyModel,&AnimeFilterProxyModel::animeMessage,this,
                     [totalCountLabel,loadMore,loadingLabel](const QString &msg, int flag,bool hasMore){
        totalCountLabel->setText(msg);
        if(hasMore)
        {
            loadMore->show();
        }
        else
        {
            loadMore->hide();
        }
        if(flag&PopMessageFlag::PM_PROCESS)
        {
            loadingLabel->show();
        }
        else
        {
            loadingLabel->hide();
        }

    });


    QHBoxLayout *toolbuttonHLayout=new QHBoxLayout();
    toolbuttonHLayout->addWidget(loadingLabel);
    toolbuttonHLayout->addWidget(totalCountLabel);
    toolbuttonHLayout->addWidget(loadMore);
    toolbuttonHLayout->addStretch(1);
    toolbuttonHLayout->addWidget(filterBox);

    QVBoxLayout *libraryVLayout=new QVBoxLayout(this);
    libraryVLayout->setContentsMargins(10*logicalDpiX()/96,10*logicalDpiY()/96,10*logicalDpiX()/96,10*logicalDpiY()/96);
    libraryVLayout->addLayout(toolbuttonHLayout);
    libraryVLayout->addWidget(animeListView);
}


void LibraryWindow::showEvent(QShowEvent *)
{
    GlobalObjects::library->animeModel->setActive(true);
}

void LibraryWindow::hideEvent(QHideEvent *)
{
    GlobalObjects::library->animeModel->setActive(false);
}

void LibraryWindow::resizeEvent(QResizeEvent *)
{
    tagCollapseButton->move(width()-tagCollapseButton->width(),(height()-tagCollapseButton->height())/2);
    if(!labelView->isHidden())
    {
        labelView->setGeometry(width()/4*3-tagCollapseButton->width(),animeListView->y(),width()/4,animeListView->height());
    }
}

AnimeFilterBox::AnimeFilterBox(QWidget *parent)
    : QLineEdit(parent)
    , filterTypeGroup(new QActionGroup(this))
{
    setClearButtonEnabled(true);
    setFont(QFont("Microsoft YaHei",12));

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


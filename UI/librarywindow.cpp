#include "librarywindow.h"
#include <QListView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>
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
#include "episodeeditor.h"
#include "animedetailinfo.h"
#include "MediaLibrary/animeitemdelegate.h"
#include "MediaLibrary/animelibrary.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/danmurender.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Video/mpvplayer.h"
#include "Common/flowlayout.h"

LibraryWindow::LibraryWindow(QWidget *parent) : QWidget(parent)
{
    AnimeItemDelegate *itemDelegate=new AnimeItemDelegate(this);
    QObject::connect(itemDelegate,&AnimeItemDelegate::ItemClicked,[this](const QModelIndex &index){
        Anime * anime = GlobalObjects::library->getAnime(static_cast<AnimeFilterProxyModel *>(animeListView->model())->mapToSource(index));
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
    proxyModel->setSourceModel(GlobalObjects::library);
    animeListView->setModel(proxyModel);

    QAction *act_delete=new QAction(tr("Delete"),this);
    QObject::connect(act_delete,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        GlobalObjects::library->deleteAnime(selection.indexes().first());
    });
    act_delete->setEnabled(false);

    QAction *act_getDetailInfo=new QAction(tr("Search for details"),this);
    QObject::connect(act_getDetailInfo,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        Anime * currentAnime = GlobalObjects::library->getAnime(selection.indexes().first());
        BangumiSearch bgmSearch(currentAnime,this);
        bgmSearch.exec();
    });
    act_getDetailInfo->setEnabled(false);

    animeListView->addAction(act_getDetailInfo);
    animeListView->addAction(act_delete);

    QObject::connect(animeListView->selectionModel(), &QItemSelectionModel::selectionChanged,[act_delete,act_getDetailInfo,this](){
        bool hasSelection = !animeListView->selectionModel()->selection().isEmpty();
        act_delete->setEnabled(hasSelection);
        act_getDetailInfo->setEnabled(hasSelection);
    });

    AnimeFilterBox *filterBox=new AnimeFilterBox(this);
    filterBox->setMinimumWidth(300*logicalDpiX()/96);
    QObject::connect(filterBox,&AnimeFilterBox::filterChanged,[proxyModel](int type,const QString &str){
       proxyModel->setFilterType(type);
       proxyModel->setFilterRegExp(str);
    });

    tagCollapseButton=new QPushButton(this);
    GlobalObjects::iconfont.setPointSize(8);
    tagCollapseButton->setFont(GlobalObjects::iconfont);
    tagCollapseButton->setText(QChar(0xe6b1));
    tagCollapseButton->setFixedSize(12*logicalDpiX()/96,80*logicalDpiY()/96);
    tagCollapseButton->setObjectName(QStringLiteral("TagPanelCollapse"));
    tagCollapseButton->move(width()-tagCollapseButton->width(),(height()-tagCollapseButton->height())/2);
    QObject::connect(tagCollapseButton,&QPushButton::clicked,[this](){
        bool panelOpen=!filterPage->isHidden();
        filterPage->resize(width()/4,animeListView->height());
        QPropertyAnimation *moveAnime = new QPropertyAnimation(filterPage, "pos");
        QPoint endPos(panelOpen?width():width()/4*3-tagCollapseButton->width(),animeListView->y()),
                startPos(panelOpen?filterPage->x():width(),animeListView->y());
        moveAnime->setDuration(500);
        moveAnime->setEasingCurve(QEasingCurve::OutExpo);
        moveAnime->setStartValue(startPos);
        moveAnime->setEndValue(endPos);
        filterPage->show();
        moveAnime->start(QAbstractAnimation::DeleteWhenStopped);
        tagCollapseButton->setText(panelOpen?QChar(0xe6b1):QChar(0xe943));
        QObject::connect(moveAnime,&QPropertyAnimation::finished,[this,panelOpen](){
            if(panelOpen)filterPage->hide();
        });
    });

    filterPage=new QWidget(this);
    filterPage->setObjectName(QStringLiteral("FilterPage"));
    QWidget *filterPageContent=new QWidget(filterPage);
    filterPageContent->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QScrollArea *filterScrollArea=new QScrollArea(filterPage);
    filterScrollArea->setObjectName(QStringLiteral("FilterContentArea"));
    filterScrollArea->setWidget(filterPageContent);
    filterScrollArea->setWidgetResizable(true);
    filterScrollArea->setAlignment(Qt::AlignCenter);
    filterScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    filterScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QHBoxLayout *filterPageHLayout=new QHBoxLayout(filterPage);
    filterPageHLayout->setContentsMargins(0,0,0,0);
    filterPageHLayout->addWidget(filterScrollArea);

    QLabel *timeLabel=new QLabel(tr("Year"),filterPageContent);
    timeLabel->setObjectName(QStringLiteral("AnimeLabelTip"));
    QLabel *tagLabel=new QLabel(tr("Tag"),filterPageContent);
    tagLabel->setObjectName(QStringLiteral("AnimeLabelTip"));
    timePanel=new LabelPanel(filterPageContent);
    tagPanel=new LabelPanel(filterPageContent,true);

    QVBoxLayout *filterContentVLayout=new QVBoxLayout(filterPageContent);
    filterContentVLayout->addWidget(timeLabel);
    filterContentVLayout->addWidget(timePanel);
    filterContentVLayout->addWidget(tagLabel);
    filterContentVLayout->addWidget(tagPanel);
    filterContentVLayout->addStretch(1);

    filterPage->resize(width()/4,animeListView->height());
    filterPage->hide();

    QObject::connect(GlobalObjects::library,&AnimeLibrary::refreshLabelInfo,[this](){
        auto &tagsMap=GlobalObjects::library->animeTags();
        for(auto iter=tagsMap.begin();iter!=tagsMap.end();iter++)
        {
            tagPanel->addTag(iter.key());
        }
        auto &timeSet=GlobalObjects::library->animeTime();
        for(const QString &time:timeSet)
        {
            timePanel->addTag(time);
        }
    });
    QObject::connect(GlobalObjects::library,&AnimeLibrary::addTags,[this](const QStringList &tags){
        for(const QString &tag:tags)
        {
            tagPanel->addTag(tag);
        }
    });
    QObject::connect(GlobalObjects::library,&AnimeLibrary::addTimeLabel,timePanel,&LabelPanel::addTag);
    QObject::connect(tagPanel,&LabelPanel::tagStateChanged,[proxyModel](const QString &tag, bool checked){
        if(checked)proxyModel->addTag(tag);
        else proxyModel->removeTag(tag);
    });
    QObject::connect(timePanel,&LabelPanel::tagStateChanged,[proxyModel](const QString &time, bool checked){
        if(checked)proxyModel->addTime(time);
        else proxyModel->removeTime(time);
    });
    QObject::connect(tagPanel,&LabelPanel::deleteTag,[proxyModel](const QString &tag){
        proxyModel->removeTag(tag);
        GlobalObjects::library->deleteTag(tag);
    });

    QLabel *totalCountLabel=new QLabel(this);
    totalCountLabel->setFont(QFont("Microsoft Yahei",12));
    QObject::connect(GlobalObjects::library,&AnimeLibrary::animeCountChanged,[totalCountLabel](){
        totalCountLabel->setText(tr("Anime Count: %1").arg(GlobalObjects::library->getCount(0)));
    });

    QHBoxLayout *toolbuttonHLayout=new QHBoxLayout();
    toolbuttonHLayout->addWidget(totalCountLabel);
    toolbuttonHLayout->addStretch(1);
    toolbuttonHLayout->addWidget(filterBox);

    QVBoxLayout *libraryVLayout=new QVBoxLayout(this);
    libraryVLayout->setContentsMargins(10*logicalDpiX()/96,10*logicalDpiY()/96,10*logicalDpiX()/96,10*logicalDpiY()/96);
    libraryVLayout->addLayout(toolbuttonHLayout);
    libraryVLayout->addWidget(animeListView);
}


void LibraryWindow::showEvent(QShowEvent *)
{
    GlobalObjects::library->setActive(true);
}

void LibraryWindow::hideEvent(QHideEvent *)
{
    GlobalObjects::library->setActive(false);
}

void LibraryWindow::resizeEvent(QResizeEvent *)
{
    tagCollapseButton->move(width()-tagCollapseButton->width(),(height()-tagCollapseButton->height())/2);
    if(!filterPage->isHidden())
    {
        filterPage->setGeometry(width()/4*3-tagCollapseButton->width(),animeListView->y(),width()/4,animeListView->height());
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
    QAction *filterTitle = menu->addAction(tr("Anime Title"));
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

LabelPanel::LabelPanel(QWidget *parent, bool allowDelete):QWidget(parent),showDelete(allowDelete)
{
    setLayout(new FlowLayout(this));
    setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
}

void LabelPanel::addTag(const QString &tag)
{
    if(tagList.contains(tag))return;
    QPushButton *tagButton=new QPushButton(tag,this);
    tagButton->setObjectName(QStringLiteral("TagButton"));
    tagButton->setCheckable(true);
    QObject::connect(tagButton,&QPushButton::toggled,[this,tag](bool checked){
        emit tagStateChanged(tag,checked);
    });
    if(showDelete)
    {
        tagButton->setContextMenuPolicy(Qt::ActionsContextMenu);
        QAction *deleteAction=new QAction(tr("Delete"),tagButton);
        QObject::connect(deleteAction,&QAction::triggered,[this,tag,tagButton](){
            emit deleteTag(tag);
            tagButton->deleteLater();
            tagList.remove(tag);
        });
        tagButton->addAction(deleteAction);
    }
    layout()->addWidget(tagButton);
    tagList.insert(tag,tagButton);
}

void LabelPanel::removeTag(const QString &tag)
{
    QPushButton *tagButton=tagList.value(tag,nullptr);
    if(tagButton)
    {
        tagButton->deleteLater();
        tagList.remove(tag);
    }
}

#include "librarywindow.h"
#include <QListView>
#include <QToolButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>
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
#include "MediaLibrary/animeitemdelegate.h"
#include "MediaLibrary/animelibrary.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/danmurender.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Video/mpvplayer.h"
namespace
{
class CharacterItem : public QWidget
{
public:
    explicit CharacterItem(QWidget *parent=nullptr):QWidget(parent)
    {
        iconLabel=new QLabel(this);
        iconLabel->setScaledContents(true);
        iconLabel->setFixedSize(60*logicalDpiX()/96,60*logicalDpiY()/96);
        iconLabel->setAlignment(Qt::AlignCenter);
        nameLabel=new QLabel(this);
        nameLabel->setOpenExternalLinks(true);
        infoLabel=new QLabel(this);
        QHBoxLayout *itemHLayout=new QHBoxLayout(this);
        itemHLayout->addWidget(iconLabel);
        QVBoxLayout *infoVLayout=new QVBoxLayout(this);
        infoVLayout->addWidget(nameLabel);
        infoVLayout->addWidget(infoLabel);
        itemHLayout->addLayout(infoVLayout);
        hide();
    }
    void setCharacter(Character *crt)
    {
        if(crt)
        {
            QPixmap icon;
            icon.loadFromData(crt->image);
            iconLabel->setPixmap(icon);
            nameLabel->setText(QString("<a href = \"http://bgm.tv/character/%1\">%2(%3)</a>").arg(crt->bangumiID).arg(crt->name).arg(crt->name_cn));
            nameLabel->adjustSize();
            infoLabel->setText(crt->actor);
            infoLabel->adjustSize();
            show();
        }
        else
        {
            hide();
        }

    }
private:
    QLabel *iconLabel,*nameLabel,*infoLabel;
};

class EpItem : public QLabel
{
public:
    static LibraryWindow *libraryWindow;
    explicit EpItem(QWidget *parent=nullptr):QLabel(parent)
    {
        setObjectName(QStringLiteral("EpItem"));
        hide();
    }
    void setEp(Episode *ep)
    {
        if(ep)
        {
            show();
            setText(tr("%1  <br/><i style=\"color: #686868; font-size:small;\">Last Play: %2</i>").arg(ep->name).arg(ep->lastPlayTime));
            setToolTip(ep->localFile);
            path=ep->localFile;
        }
        else
        {
            hide();
        }
    }
private:
    QString path;
protected:
    virtual void mousePressEvent(QMouseEvent *event)
    {
        if(event->button()==Qt::LeftButton)
        {
            emit libraryWindow->playFile(path);
        }
    }
};
LibraryWindow *EpItem::libraryWindow=nullptr;
class DetailInfoPage : public QWidget
{
public:
    explicit DetailInfoPage(QWidget *parent=nullptr):QWidget(parent)
    {
        setObjectName(QStringLiteral("AnimeDetailInfo"));
        EpItem::libraryWindow=static_cast<LibraryWindow *>(parent);
        QWidget *contentWidget=new QWidget(this); 
        contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        contentScrollArea=new QScrollArea(this);
        contentScrollArea->setObjectName(QStringLiteral("DetailContentArea"));
        contentScrollArea->setWidget(contentWidget);
        contentScrollArea->setWidgetResizable(true);
        contentScrollArea->setAlignment(Qt::AlignCenter);
        contentScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        QHBoxLayout *cHLayout=new QHBoxLayout(this);
        cHLayout->setContentsMargins(0,0,0,0);
        cHLayout->addWidget(contentScrollArea);

        coverLabel=new QLabel(this);
        coverLabel->setAlignment(Qt::AlignCenter);
        coverLabel->setScaledContents(true);
        coverLabel->setFixedSize(140*logicalDpiX()/96,205*logicalDpiY()/96);
        dateStaffLabel=new QLabel(this);
        dateStaffLabel->setFont(QFont("Microsoft Yahei",12));
        dateStaffLabel->setWordWrap(true);
        titleLabel=new QLabel(this);
        titleLabel->setFont(QFont("Microsoft Yahei",18));
        titleLabel->setWordWrap(true);
        titleLabel->setOpenExternalLinks(true);
        infoLabel=new QLabel(this);
        infoLabel->setWordWrap(true);
        infoLabel->setContentsMargins(0,0,10,0);

        int buttonWidth=24*logicalDpiX()/96,buttonHeight=24*logicalDpiY()/96;
        getDetailInfoButton=new QToolButton(this);
        getDetailInfoButton->setToolTip(QObject::tr("Search for details"));
        getDetailInfoButton->setFixedSize(buttonWidth,buttonHeight);
        GlobalObjects::iconfont.setPointSize(12);
        getDetailInfoButton->setFont(GlobalObjects::iconfont);
        getDetailInfoButton->setText(QChar(0xe609));
        QObject::connect(getDetailInfoButton,&QToolButton::clicked,[this](){
            BangumiSearch bgmSearch(currentAnime,this);
            if(QDialog::Accepted==bgmSearch.exec())
            {
                currentAnime=bgmSearch.currentAnime;
                setAnime(currentAnime);
            }
        });

        QHBoxLayout *titleHLayout=new QHBoxLayout();
        titleHLayout->addWidget(titleLabel);
        titleHLayout->addWidget(getDetailInfoButton);

        episodeLabel=new QLabel(QObject::tr("Episodes"),this);
        episodeLabel->setFont(QFont("Microsoft Yahei",14));
        addToPlaylistButton=new QToolButton(this);
        addToPlaylistButton->setToolTip(QObject::tr("Add to Playlist"));
        addToPlaylistButton->setFixedSize(buttonWidth,buttonHeight);
        addToPlaylistButton->setFont(GlobalObjects::iconfont);
        addToPlaylistButton->setText(QChar(0xe721));
        QObject::connect(addToPlaylistButton,&QToolButton::clicked,[this](){
            if(currentAnime->eps.count()>0)
            {
                QModelIndex collectIndex = GlobalObjects::playlist->addCollection(QModelIndex(),currentAnime->title);
                QStringList items;
                for(auto iter=currentAnime->eps.cbegin();iter!=currentAnime->eps.cend();iter++)
                {
                    items<<(*iter).localFile;
                }
                GlobalObjects::playlist->addItems(items,collectIndex);
            }
        });

        editEpsButton=new QToolButton(this);
        editEpsButton->setToolTip(QObject::tr("Edit Episodes"));
        editEpsButton->setFixedSize(buttonWidth,buttonHeight);
        editEpsButton->setFont(GlobalObjects::iconfont);
        editEpsButton->setText(QChar(0xe60a));
        QObject::connect(editEpsButton,&QToolButton::clicked,[this](){
            EpisodeEditor epEditor(currentAnime,this);
            epEditor.exec();
            if(epEditor.episodeChanged())
            {
                refreshEpList(currentAnime);
            }
        });

        QHBoxLayout *epheadHLayout=new QHBoxLayout();
        epheadHLayout->addWidget(episodeLabel);
        epheadHLayout->addWidget(addToPlaylistButton);
        epheadHLayout->addWidget(editEpsButton);
        epheadHLayout->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::MinimumExpanding));

        epRegion=new QWidget(this);
        QVBoxLayout *epVLayout=new QVBoxLayout(epRegion);
        epRegion->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

        crtLabel=new QLabel(QObject::tr("Characters"),this);
        crtLabel->setFont(QFont("Microsoft Yahei",14));
        crtRegion=new QWidget(this);
        QVBoxLayout *crtVLayout=new QVBoxLayout(crtRegion);
        crtRegion->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

        QGridLayout *infoGLayout=new QGridLayout(contentWidget);
        infoGLayout->setContentsMargins(8*logicalDpiX()/96,30*logicalDpiX()/96,8*logicalDpiX()/96,8*logicalDpiX()/96);
        infoGLayout->addWidget(coverLabel,0,0,2,1);
        infoGLayout->addLayout(titleHLayout,0,1);
        infoGLayout->addWidget(dateStaffLabel,1,1);
        infoGLayout->addWidget(infoLabel,2,0,1,2);
        infoGLayout->addLayout(epheadHLayout,3,0,1,2);
        infoGLayout->addWidget(epRegion,4,0,1,2);
        infoGLayout->addWidget(crtLabel,5,0,1,2);
        infoGLayout->addWidget(crtRegion,6,0,1,2);
        infoGLayout->addItem(new QSpacerItem(1,1,QSizePolicy::Minimum,QSizePolicy::MinimumExpanding),7,0,1,2);
        infoGLayout->setRowStretch(7,1);
        infoGLayout->setColumnStretch(1,1);

    }
    void refreshEpList(Anime *anime)
    {
        int i=0;
        for(auto iter=anime->eps.begin();iter!=anime->eps.end();iter++)
        {
            if(episodes.count()>i)
            {
                episodes.at(i)->setEp(&(*iter));
            }
            else
            {
                EpItem *epItem=new EpItem(epRegion);
                epRegion->layout()->addWidget(epItem);
                episodes.append(epItem);
                epItem->setEp(&(*iter));
            }
            i++;
        }
        while (i<episodes.count())
        {
            episodes.at(i++)->setEp(nullptr);
        }
    }
    void setAnime(Anime *anime)
    {
        static QPixmap nullCover(":/res/images/cover.png");
        if(anime->cover.isEmpty())
        {
            coverLabel->setPixmap(nullCover);
        }
        else
        {
            coverLabel->setPixmap(anime->coverPixmap);
        }
        titleLabel->setText(QString("<a href = \"http://bgm.tv/subject/%1\">%2</a>").arg(anime->bangumiID).arg(anime->title));
        QStringList stafflist;
        for(auto iter=anime->staff.cbegin();iter!=anime->staff.cend();iter++)
        {
            stafflist.append((*iter).first+": "+(*iter).second);
        }
        dateStaffLabel->setMaximumWidth(width()*3/5-20);
        QString addTime(QDateTime::fromSecsSinceEpoch(anime->addTime).toString("yyyy-MM-dd hh:mm:ss"));
        dateStaffLabel->setText(QObject::tr("Add Time: %0\nEps: %1\nDate: %2\n%3").arg(addTime).arg(anime->epCount).arg(anime->date).arg(stafflist.join('\n')));
        infoLabel->setText(anime->summary);
        refreshEpList(anime);
        int i=0;
        for(auto iter=anime->characters.begin();iter!=anime->characters.end();iter++)
        {
            if(crts.count()>i)
            {
                crts.at(i)->setCharacter(&(*iter));
            }
            else
            {
                CharacterItem *crtItem=new CharacterItem(crtRegion);
                crtRegion->layout()->addWidget(crtItem);
                crts.append(crtItem);
                crtItem->setCharacter(&(*iter));
            }
            i++;
        }
        while (i<crts.count())
        {
            crts.at(i++)->setCharacter(nullptr);
        }
        currentAnime=anime;
        contentScrollArea->verticalScrollBar()->setValue(0);
    }
private:
    QLabel *coverLabel,*titleLabel,*infoLabel,*dateStaffLabel, *episodeLabel,*crtLabel;
    QToolButton *addToPlaylistButton,*editEpsButton,*getDetailInfoButton;
    QWidget *epRegion,*crtRegion;
    QScrollArea *contentScrollArea;
    QList<CharacterItem *> crts;
    QList<EpItem *> episodes;
    Anime *currentAnime;
};
}
LibraryWindow::LibraryWindow(QWidget *parent) : QWidget(parent)
{
    contentWidget=new QWidget(this);
    contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    blurEffect=new QGraphicsBlurEffect(this);
    contentWidget->setGraphicsEffect(blurEffect);
    blurEffect->setBlurRadius(30);
    blurEffect->setEnabled(false);
    QHBoxLayout *contentHLayout=new QHBoxLayout(this);
    contentHLayout->addWidget(contentWidget);

    detailInfoPage=new DetailInfoPage(this);
    detailInfoPage->hide();

    QFont btnFont("Microsoft Yahei",12);
    allAnime=new QToolButton(contentWidget);
    //allAnime->setIcon(QIcon(":/res/images/all.png"));
    allAnime->setObjectName(QStringLiteral("LibraryRangeButton"));
    allAnime->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    allAnime->setCheckable(true);
    allAnime->setText(btnText[0]);
    allAnime->setFont(btnFont);

    threeMonth=new QToolButton(contentWidget);
    threeMonth->setObjectName(QStringLiteral("LibraryRangeButton"));
    threeMonth->setCheckable(true);
    threeMonth->setText(btnText[1]);
    threeMonth->setFont(btnFont);


    halfYear=new QToolButton(contentWidget);
    halfYear->setObjectName(QStringLiteral("LibraryRangeButton"));
    halfYear->setCheckable(true);
    halfYear->setText(btnText[2]);
    halfYear->setFont(btnFont);

    year=new QToolButton(contentWidget);
    year->setObjectName(QStringLiteral("LibraryRangeButton"));
    year->setCheckable(true);
    year->setText(btnText[3]);
    year->setFont(btnFont);

    AnimeItemDelegate *itemDelegate=new AnimeItemDelegate(this);
    QObject::connect(itemDelegate,&AnimeItemDelegate::ItemClicked,this,&LibraryWindow::showDetailInfo);

    animeListView=new QListView(contentWidget);
    animeListView->setObjectName(QStringLiteral("AnimesContent"));
    animeListView->setViewMode(QListView::IconMode);
    animeListView->setUniformItemSizes(true);
    animeListView->setResizeMode(QListView::Adjust);
    animeListView->setMovement(QListView::Static);
    animeListView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    animeListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    animeListView->setItemDelegate(itemDelegate);
    animeListView->setMouseTracking(true);
    animeListView->setContextMenuPolicy(Qt::ActionsContextMenu);
    AnimeFilterProxyModel *proxyModel=new AnimeFilterProxyModel(this);
    proxyModel->setSourceModel(GlobalObjects::library);
    animeListView->setModel(proxyModel);

    QAction *act_delete=new QAction(tr("Delete"),this);
    animeListView->addAction(act_delete);
    QObject::connect(act_delete,&QAction::triggered,[this,proxyModel](){
        QItemSelection selection=proxyModel->mapSelectionToSource(animeListView->selectionModel()->selection());
        if(selection.size()==0)return;
        GlobalObjects::library->deleteAnime(selection.indexes());
    });
    act_delete->setEnabled(false);
    QObject::connect(animeListView->selectionModel(), &QItemSelectionModel::selectionChanged,[act_delete,this](){
        bool hasSelection = !animeListView->selectionModel()->selection().isEmpty();
        act_delete->setEnabled(hasSelection);
    });

    btnGroup=new QButtonGroup(contentWidget);
    btnGroup->addButton(allAnime,0);
    btnGroup->addButton(threeMonth,1);
    btnGroup->addButton(halfYear,2);
    btnGroup->addButton(year,3);
    QObject::connect(btnGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[this,proxyModel](int id, bool checked){
        if(checked)
        {
            proxyModel->setTimeRange(id);
            proxyModel->setFilterKeyColumn(0);
        }
    });
    allAnime->setChecked(true);
    updateButtonText();
    QObject::connect(GlobalObjects::library,&AnimeLibrary::animeCountChanged,this,&LibraryWindow::updateButtonText);

    AnimeFilterBox *filterBox=new AnimeFilterBox(this);
    filterBox->setMinimumWidth(200*logicalDpiX()/96);
    QObject::connect(filterBox,&AnimeFilterBox::filterChanged,[proxyModel](int type,const QString &str){
       proxyModel->setFilterType(type);
       proxyModel->setFilterRegExp(str);
    });

    QHBoxLayout *toolbuttonHLayout=new QHBoxLayout();
    toolbuttonHLayout->addWidget(allAnime);
    toolbuttonHLayout->addWidget(threeMonth);
    toolbuttonHLayout->addWidget(halfYear);
    toolbuttonHLayout->addWidget(year);
    toolbuttonHLayout->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::MinimumExpanding));
    toolbuttonHLayout->addWidget(filterBox);

    QVBoxLayout *libraryVLayout=new QVBoxLayout(contentWidget);
    libraryVLayout->addLayout(toolbuttonHLayout);
    libraryVLayout->addWidget(animeListView);
}

void LibraryWindow::updateButtonText()
{
    for(QAbstractButton * btn:btnGroup->buttons())
    {
        int id=btnGroup->id(btn);
        btn->setText(QString("%1(%2)").arg(btnText[id]).arg(GlobalObjects::library->getCount(id)));
    }
}

void LibraryWindow::showDetailInfo(const QModelIndex &index)
{
    Anime * anime = GlobalObjects::library->getAnime(static_cast<AnimeFilterProxyModel *>(animeListView->model())->mapToSource(index));
    detailInfoPage->resize(width()/2,height());
    static_cast<DetailInfoPage *>(detailInfoPage)->setAnime(anime);

    blurEffect->setEnabled(true);

    QPropertyAnimation *moveAnime = new QPropertyAnimation(detailInfoPage, "pos");
    QPoint startPos(width(),0), endPos(width()/2,0);
    moveAnime->setDuration(500);
    moveAnime->setEasingCurve(QEasingCurve::OutExpo);
    moveAnime->setStartValue(startPos);
    moveAnime->setEndValue(endPos);

    detailInfoPage->show();
    contentWidget->setEnabled(false);
    moveAnime->start(QAbstractAnimation::DeleteWhenStopped);
}

void LibraryWindow::showEvent(QShowEvent *)
{
    GlobalObjects::library->setActive(true);
}

void LibraryWindow::hideEvent(QHideEvent *)
{
    GlobalObjects::library->setActive(false);
}

void LibraryWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)
    {
        if(!detailInfoPage->isHidden() && !detailInfoPage->underMouse())
        {

            QPropertyAnimation *moveAnime = new QPropertyAnimation(detailInfoPage, "pos");
            QPoint endPos(width(),0), startPos(width()/2,0);
            moveAnime->setDuration(500);
            moveAnime->setEasingCurve(QEasingCurve::OutExpo);
            moveAnime->setStartValue(startPos);
            moveAnime->setEndValue(endPos);
            moveAnime->start(QAbstractAnimation::DeleteWhenStopped);
            QObject::connect(moveAnime,&QPropertyAnimation::finished,[this](){
                detailInfoPage->hide();
                blurEffect->setEnabled(false);
                contentWidget->setEnabled(true);
            });
        }
    }
}

void LibraryWindow::resizeEvent(QResizeEvent *)
{
    if(!detailInfoPage->isHidden())
    {
        detailInfoPage->setGeometry(width()/2,0,width()/2,height());
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

#include "animedetailinfo.h"

#include <QToolButton>
#include <QStackedLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QTextEdit>
#include <QGraphicsDropShadowEffect>
#include <QTreeView>
#include <QFileDialog>
#include <QPushButton>
#include <QAction>
#include <QHeaderView>
#include <QScrollArea>
#include <QListWidget>
#include <QLineEdit>
#include <QListView>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include "MediaLibrary/animeinfo.h"
#include "MediaLibrary/animelibrary.h"
#include "MediaLibrary/episodesmodel.h"
#include "MediaLibrary/labelmodel.h"
#include "MediaLibrary/capturelistmodel.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Common/network.h"
#include "Common/flowlayout.h"
#include "captureview.h"

AnimeDetailInfo::AnimeDetailInfo(Anime *anime, QWidget *parent) :
    CFramelessDialog(tr("Anime Info"),parent,false,true,false),currentAnime(anime)
{
    QVBoxLayout *dialogVLayout=new QVBoxLayout(this);
    dialogVLayout->setContentsMargins(0,0,0,0);
    dialogVLayout->setSpacing(0);

    QSize pageButtonSize(80 *logicalDpiX()/96,32*logicalDpiY()/96);
    QToolButton *overviewPageButton=new QToolButton(this);
    overviewPageButton->setText(tr("Overview"));
    overviewPageButton->setCheckable(true);
    overviewPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    overviewPageButton->setObjectName(QStringLiteral("DialogPageButton"));
    overviewPageButton->setFixedSize(pageButtonSize);

    QToolButton *episodesPageButton=new QToolButton(this);
    episodesPageButton->setText(tr("Episodes"));
    episodesPageButton->setCheckable(true);
    episodesPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    episodesPageButton->setObjectName(QStringLiteral("DialogPageButton"));
    episodesPageButton->setFixedSize(pageButtonSize);

    QToolButton *characterPageButton=new QToolButton(this);
    characterPageButton->setText(tr("Characters"));
    characterPageButton->setCheckable(true);
    characterPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    characterPageButton->setObjectName(QStringLiteral("DialogPageButton"));
    characterPageButton->setFixedSize(pageButtonSize);

    QToolButton *tagPageButton=new QToolButton(this);
    tagPageButton->setText(tr("Tag"));
    tagPageButton->setCheckable(true);
    tagPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    tagPageButton->setObjectName(QStringLiteral("DialogPageButton"));
    tagPageButton->setFixedSize(pageButtonSize);

    QToolButton *capturePageButton=new QToolButton(this);
    capturePageButton->setText(tr("Capture"));
    capturePageButton->setCheckable(true);
    capturePageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    capturePageButton->setObjectName(QStringLiteral("DialogPageButton"));
    capturePageButton->setFixedSize(pageButtonSize);

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    pageButtonHLayout->addWidget(overviewPageButton);
    pageButtonHLayout->addWidget(episodesPageButton);
    pageButtonHLayout->addWidget(characterPageButton);
    pageButtonHLayout->addWidget(tagPageButton);
    pageButtonHLayout->addWidget(capturePageButton);
    pageButtonHLayout->addStretch(1);
    dialogVLayout->addLayout(pageButtonHLayout);

    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,5*logicalDpiY()/96,0,0);
    contentStackLayout->addWidget(setupOverviewPage());
    contentStackLayout->addWidget(setupEpisodesPage());
    contentStackLayout->addWidget(setupCharacterPage());
    contentStackLayout->addWidget(setupTagPage());
    contentStackLayout->addWidget(setupCapturePage());
    dialogVLayout->addSpacing(6*logicalDpiY()/96);
    dialogVLayout->addLayout(contentStackLayout);

    QButtonGroup *btnGroup = new QButtonGroup(this);
    btnGroup->addButton(overviewPageButton, 0);
    btnGroup->addButton(episodesPageButton, 1);
    btnGroup->addButton(characterPageButton, 2);
    btnGroup->addButton(tagPageButton, 3);
    btnGroup->addButton(capturePageButton,4);
    QObject::connect(btnGroup, (void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled, [contentStackLayout](int id, bool checked) {
        if (checked)
            contentStackLayout->setCurrentIndex(id);
    });
    overviewPageButton->setChecked(true);
}

QWidget *AnimeDetailInfo::setupOverviewPage()
{
    QWidget *pageWidget=new QWidget(this);
    QLabel *coverLabel=new QLabel(pageWidget);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setScaledContents(true);
    coverLabel->setFixedSize(140*logicalDpiX()/96,205*logicalDpiY()/96);
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(pageWidget);
    shadowEffect->setOffset(0, 0);
    shadowEffect->setBlurRadius(14);
    coverLabel->setGraphicsEffect(shadowEffect);
    static QPixmap nullCover(":/res/images/cover.png");
    if(currentAnime->coverPixmap.isNull())
    {
        coverLabel->setPixmap(nullCover);
    }
    else
    {
        coverLabel->setPixmap(currentAnime->coverPixmap);
    }

    QLabel *titleLabel=new QLabel(pageWidget);
    titleLabel->setFont(QFont("Microsoft Yahei",16));
    titleLabel->setWordWrap(true);
    titleLabel->setOpenExternalLinks(true);
    titleLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    titleLabel->setText(QString("<a href = \"http://bgm.tv/subject/%1\">%2</a>").arg(currentAnime->bangumiID).arg(currentAnime->title));

    QLabel *dateStaffLabel=new QLabel(pageWidget);
    dateStaffLabel->setFont(QFont("Microsoft Yahei",12));
    dateStaffLabel->setWordWrap(true);
    dateStaffLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    dateStaffLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QStringList stafflist;
    for(auto iter=currentAnime->staff.cbegin();iter!=currentAnime->staff.cend();++iter)
    {
        stafflist.append((*iter).first+": "+(*iter).second);
    }
    QString addTime(QDateTime::fromSecsSinceEpoch(currentAnime->addTime).toString("yyyy-MM-dd hh:mm:ss"));
    dateStaffLabel->setText(QObject::tr("Add Time: %0\nEps: %1\nDate: %2\n%3").arg(addTime).arg(currentAnime->epCount).arg(currentAnime->date).arg(stafflist.join('\n')));

    QTextEdit *descInfo=new QTextEdit(pageWidget);
    descInfo->setReadOnly(true);
    descInfo->setText(currentAnime->summary);

    QGridLayout *pageGLayout=new QGridLayout(pageWidget);
    pageGLayout->addWidget(coverLabel,0,0,2,1);
    pageGLayout->addItem(new QSpacerItem(10*logicalDpiX()/96,1),0,1);
    pageGLayout->addWidget(titleLabel,0,2);
    pageGLayout->addWidget(dateStaffLabel,1,2);
    pageGLayout->addWidget(descInfo,2,0,1,3);
    pageGLayout->setRowStretch(2,1);
    pageGLayout->setColumnStretch(2,1);
    return pageWidget;
}

QWidget *AnimeDetailInfo::setupEpisodesPage()
{
    QWidget *pageWidget=new QWidget(this);
    QTreeView *episodeView=new QTreeView(pageWidget);
    episodeView->setRootIsDecorated(false);
    episodeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    episodeView->setItemDelegate(new EpItemDelegate(&epNames,this));
    episodeView->setFont(font());
    EpisodesModel *epModel=new EpisodesModel(currentAnime,this);
    episodeView->setModel(epModel);
    episodeView->setAlternatingRowColors(true);
    episodeView->hideColumn(2);
    QPushButton *add=new QPushButton(tr("Add Episode(s)"),pageWidget);
    QObject::connect(add,&QPushButton::clicked,[this,epModel](){
        QStringList files = QFileDialog::getOpenFileNames(this,tr("Select media files"),"",
                                                         QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
        if (files.count())
        {
            QFileInfo fi;
            for(auto &file:files)
            {
                fi.setFile(file);
                epModel->addEpisode(fi.baseName(),file);
            }
        }
    });
    QPushButton *getEpNames=new QPushButton(tr("Get Epsoide Names"),pageWidget);
    QObject::connect(getEpNames,&QPushButton::clicked,[this,getEpNames](){
        if(currentAnime->bangumiID==-1)return;
        QString epUrl(QString("https://api.bgm.tv/subject/%1/ep").arg(currentAnime->bangumiID));
        try
        {
            getEpNames->setText(tr("Getting..."));
            getEpNames->setEnabled(false);
            this->showBusyState(true);
            QString str(Network::httpGet(epUrl,QUrlQuery(),QStringList()<<"Accept"<<"application/json"));
            QJsonDocument document(Network::toJson(str));
            QJsonObject obj = document.object();
            QJsonArray epArray=obj.value("eps").toArray();
            for(auto iter=epArray.begin();iter!=epArray.end();++iter)
            {
                QJsonObject epobj=(*iter).toObject();
                epNames.append(tr("No.%0 %1(%2)").arg(epobj.value("sort").toInt()).arg(epobj.value("name").toString().replace("&amp;","&")).arg(epobj.value("name_cn").toString().replace("&amp;","&")));
            }
            getEpNames->setText(tr("Getting Done"));

        }
        catch(Network::NetworkError &error)
        {
            showMessage(error.errorInfo,1);
            getEpNames->setEnabled(true);
            getEpNames->setText(tr("Get Epsoide Names"));
        }
        this->showBusyState(false);
    });
    QPushButton *addToPlaylist=new QPushButton(tr("Add All to Playlist"),pageWidget);
    QObject::connect(addToPlaylist,&QPushButton::clicked,[this](){
        if(currentAnime->eps.count()>0)
        {
            QModelIndex collectIndex = GlobalObjects::playlist->addCollection(QModelIndex(),currentAnime->title);
            QStringList items;
            for(auto iter=currentAnime->eps.cbegin();iter!=currentAnime->eps.cend();++iter)
            {
                items<<(*iter).localFile;
            }
            showMessage(tr("Add %1 items to Playlist").arg(GlobalObjects::playlist->addItems(items,collectIndex)));
        }
    });
    QAction *deleteAction=new QAction(tr("Delete"),pageWidget);
    QObject::connect(deleteAction,&QAction::triggered,[epModel,episodeView](){
        auto selection = episodeView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        epModel->removeEpisodes(selection);
    });
    QAction *addAction=new QAction(tr("Add to Playlist"),pageWidget);
    QObject::connect(addAction,&QAction::triggered,[episodeView,this](){
        auto selection = episodeView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        QStringList items;
        foreach (const QModelIndex &index, selection)
        {
            if (index.isValid())
            {
                int row=index.row();
                items<<currentAnime->eps.at(row).localFile;
            }
        }
        showMessage(tr("Add %1 items to Playlist").arg(GlobalObjects::playlist->addItems(items,QModelIndex())));
    });
    QAction *playAction=new QAction(tr("Play"),pageWidget);
    QObject::connect(playAction,&QAction::triggered,[episodeView,this](){
        auto selection = episodeView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        QFileInfo info(currentAnime->eps.at(selection.last().row()).localFile);
        if(!info.exists())
        {
            showMessage(tr("File Not Exist"),1);
            return;
        }
        emit playFile(info.absoluteFilePath());
        CFramelessDialog::onAccept();
    });
    QAction *explorerViewAction=new QAction(tr("Browse File"),pageWidget);
    QObject::connect(explorerViewAction,&QAction::triggered,[this,episodeView](){
        auto selection = episodeView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        QFileInfo info(currentAnime->eps.at(selection.last().row()).localFile);
        if(!info.exists())
        {
            showMessage(tr("File not Exist"),1);
            return;
        }
        QString command("Explorer /select," + QDir::toNativeSeparators(info.absoluteFilePath()));
        QProcess::startDetached(command);
    });

    episodeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    episodeView->addAction(playAction);
    episodeView->addAction(addAction);
    episodeView->addAction(explorerViewAction);
    episodeView->addAction(deleteAction);

    QGridLayout *epGLayout=new QGridLayout(pageWidget);
    epGLayout->addWidget(add,0,0);
    epGLayout->addWidget(addToPlaylist,0,1);
    epGLayout->addWidget(getEpNames,0,2);
    epGLayout->addWidget(episodeView,1,0,1,4);
    epGLayout->setRowStretch(1,1);
    epGLayout->setColumnStretch(3,1);
    epGLayout->setContentsMargins(0, 0, 0, 0);
    QHeaderView *epHeader = episodeView->header();
    epHeader->setFont(font());
    epHeader->resizeSection(0, 200*logicalDpiX()/96);
    epHeader->resizeSection(1, 80*logicalDpiX()/96);
    return pageWidget;
}

QWidget *AnimeDetailInfo::setupCharacterPage()
{
    QWidget *pageWidget=new QWidget(this);
    QLabel *tipLabel=new QLabel(tr("Click the icon to download the avatar again"),pageWidget);
    QWidget *contentWidget=new QWidget(pageWidget);
    contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QScrollArea *contentScrollArea=new QScrollArea(pageWidget);
    contentScrollArea->setWidget(contentWidget);
    contentScrollArea->setWidgetResizable(true);
    contentScrollArea->setAlignment(Qt::AlignCenter);
    contentScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QVBoxLayout *cVLayout=new QVBoxLayout(pageWidget);
    cVLayout->setContentsMargins(0,0,0,0);
    cVLayout->addWidget(tipLabel);
    cVLayout->addWidget(contentScrollArea);

    QVBoxLayout *pageVLayout=new QVBoxLayout(contentWidget);
    for(auto iter=currentAnime->characters.begin();iter!=currentAnime->characters.end();++iter)
    {
        CharacterItem *crtItem=new CharacterItem(&(*iter),contentWidget);
        QObject::connect(crtItem,&CharacterItem::updateCharacter,this,[this](CharacterItem *crtItem){
           showBusyState(true);
           crtItem->setEnabled(false);
           GlobalObjects::library->updateCrtImage(currentAnime,crtItem->crt);
           crtItem->refreshIcon();
           crtItem->setEnabled(true);
           showBusyState(false);
        });
        pageVLayout->addWidget(crtItem);
    }
    return pageWidget;
}

QWidget *AnimeDetailInfo::setupTagPage()
{
    QWidget *pageWidget=new QWidget(this);
    QStackedLayout *contentStackLayout=new QStackedLayout(pageWidget);
    contentStackLayout->setContentsMargins(0,0,0,0);

    QWidget *animeTagPage=new QWidget(pageWidget);
    QListWidget *tagList=new QListWidget(animeTagPage);
    tagList->setSelectionMode(QAbstractItemView::SingleSelection);
    tagList->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto &tagMap=GlobalObjects::library->labelModel->getTags();
    for(auto iter=tagMap.begin();iter!=tagMap.end();++iter)
    {
        if(iter.value().contains(currentAnime->title))
            tagList->addItem(iter.key());
    }
    QLineEdit *tagEdit=new QLineEdit(animeTagPage);
    QPushButton *addTagButton=new QPushButton(tr("Add Tag"), animeTagPage);
    QObject::connect(tagEdit,&QLineEdit::returnPressed,addTagButton,&QPushButton::click);
    QObject::connect(addTagButton,&QPushButton::clicked,[tagEdit,tagList,this](){
        QString tag=tagEdit->text().trimmed();
        if(tag.isEmpty())return;
        GlobalObjects::library->addTags(currentAnime,QStringList()<<tag);
        tagEdit->clear();
        tagList->insertItem(0,tag);
    });
    QAction *deleteTagAction=new QAction(tr("Delete"),animeTagPage);
    QObject::connect(deleteTagAction,&QAction::triggered,this,[tagList,this](){
       auto selectedItems=tagList->selectedItems();
       if(selectedItems.count()==0)return;
       GlobalObjects::library->deleteTag(selectedItems.first()->text(),currentAnime->title);
       delete tagList->takeItem(tagList->row(selectedItems.first()));
    });
    tagList->addAction(deleteTagAction);
    QPushButton *tagOnBGMButton=new QPushButton(tr("Tags on Bangumi"), animeTagPage);
    QGridLayout *animeTagGLayout=new QGridLayout(animeTagPage);
    animeTagGLayout->addWidget(tagEdit,0,0);
    animeTagGLayout->addWidget(addTagButton,0,1);
    animeTagGLayout->addWidget(tagOnBGMButton,0,2);
    animeTagGLayout->addWidget(tagList,1,0,1,3);
    animeTagGLayout->setRowStretch(1,1);
    animeTagGLayout->setColumnStretch(0,1);

    QWidget *bgmTagPage=new QWidget(pageWidget);
    LabelPanel *bgmLabelPanel=new LabelPanel(bgmTagPage);
    QPushButton *bgmTagOK=new QPushButton(tr("Add Selected Tags"), animeTagPage);
    QObject::connect(bgmTagOK,&QPushButton::clicked,this,[this,bgmLabelPanel,contentStackLayout,tagList](){
        QStringList tags(bgmLabelPanel->getSelectedTags());
        if(tags.count()==0) return;
        GlobalObjects::library->addTags(currentAnime,tags);
        tagList->clear();
        auto &tagMap=GlobalObjects::library->labelModel->getTags();
        for(auto iter=tagMap.begin();iter!=tagMap.end();++iter)
        {
            if(iter.value().contains(currentAnime->title))
                tagList->addItem(iter.key());
        }
        contentStackLayout->setCurrentIndex(0);
    });
    QPushButton *bgmTagCancel=new QPushButton(tr("Cancel"), animeTagPage);
    QObject::connect(bgmTagCancel,&QPushButton::clicked,this,[contentStackLayout](){
       contentStackLayout->setCurrentIndex(0);
    });

    QObject::connect(tagOnBGMButton,&QPushButton::clicked,this,[contentStackLayout,tagOnBGMButton,bgmLabelPanel,this](){
       showBusyState(true);
       QStringList tags;
       QString errorInfo;
       tagOnBGMButton->setEnabled(false);
       GlobalObjects::library->downloadTags(currentAnime,tags,&errorInfo);
       if(!errorInfo.isEmpty())
       {
           showMessage(errorInfo,1);
       }
       else
       {
           bgmLabelPanel->removeTag();
           bgmLabelPanel->addTag(tags);
           contentStackLayout->setCurrentIndex(1);
       }
       tagOnBGMButton->setEnabled(true);
       showBusyState(false);
    });
    QGridLayout *bgmTagGLayout=new QGridLayout(bgmTagPage);
    bgmTagGLayout->addWidget(bgmTagOK,0,0);
    bgmTagGLayout->addWidget(bgmTagCancel,0,1);
    bgmTagGLayout->addWidget(bgmLabelPanel,1,0,1,3);
    bgmTagGLayout->setRowStretch(1,1);
    bgmTagGLayout->setColumnStretch(2,1);

    contentStackLayout->addWidget(animeTagPage);
    contentStackLayout->addWidget(bgmTagPage);
    return pageWidget;
}

QWidget *AnimeDetailInfo::setupCapturePage()
{
    CaptureListModel *captureModel=new CaptureListModel(currentAnime->title,this);
    QObject::connect(captureModel,&CaptureListModel::fetching,this,&AnimeDetailInfo::showBusyState);
    QListView *captureView=new QListView(this);
    captureView->setObjectName(QStringLiteral("captureView"));
    captureView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    captureView->setIconSize(QSize(200*logicalDpiX()/96,112*logicalDpiY()/96));
    captureView->setViewMode(QListView::IconMode);
    captureView->setUniformItemSizes(true);
    captureView->setResizeMode(QListView::Adjust);
    captureView->setMovement(QListView::Static);
    captureView->setWordWrap(true);

    captureView->setModel(captureModel);
    QAction* actListMode = new QAction(tr("List Mode"),this);
    actListMode->setCheckable(true);
    QObject::connect(actListMode, &QAction::triggered,[captureView](bool)
    {
        captureView->setViewMode(QListView::ListMode);
    });
    QAction* actIconMode = new QAction(tr("Icon Mode"));
    actIconMode->setCheckable(true);
    QObject::connect(actIconMode, &QAction::triggered,[captureView](bool)
    {
        captureView->setViewMode(QListView::IconMode);
    });
    QAction* actCopy = new QAction(tr("Copy"),this);
    QObject::connect(actCopy, &QAction::triggered,[captureView,captureModel](bool)
    {
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        QPixmap img(captureModel->getFullCapture(selection.first().row()));
        QApplication::clipboard()->setPixmap(img);
    });
    QAction* actSave = new QAction(tr("Save"),this);
    QObject::connect(actSave, &QAction::triggered,[this,captureView,captureModel](bool)
    {
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        QPixmap img(captureModel->getFullCapture(selection.first().row()));
        const CaptureItem *item=captureModel->getCaptureItem(selection.first().row());
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Capture"),item->info,
                                    tr("JPEG Images (*.jpg);;PNG Images (*.png)"));
        if(!fileName.isEmpty())
        {
            img.save(fileName);
        }
    });
    QAction* actRemove = new QAction(tr("Remove"),this);
    QObject::connect(actRemove, &QAction::triggered,[captureView,captureModel](bool)
    {
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        captureModel->deleteCaptures(selection);
    });
    QMenu *contexMenu = new QMenu(this);
    QActionGroup *group=new QActionGroup(this);
    group->addAction(actIconMode);
    group->addAction(actListMode);
    actIconMode->setChecked(true);
    contexMenu->addAction(actCopy);
    contexMenu->addAction(actSave);
    contexMenu->addAction(actRemove);
    contexMenu->addSeparator();
    contexMenu->addAction(actIconMode);
    contexMenu->addAction(actListMode);
    captureView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(captureView, &QListView::customContextMenuRequested, [contexMenu]()
    {
        contexMenu->exec(QCursor::pos());
    });
    QObject::connect(captureView,&QListView::doubleClicked,[this,captureModel](const QModelIndex &index){
        CaptureView view(captureModel,index.row(),this);
        view.exec();
    });
    return captureView;
}

LabelPanel::LabelPanel(QWidget *parent, bool allowDelete):QWidget(parent),showDelete(allowDelete)
{
    setLayout(new FlowLayout(this));
}

void LabelPanel::addTag(const QStringList &tags)
{
    for(const QString &tag:tags)
    {
        if(tagList.contains(tag))continue;
        QPushButton *tagButton=new QPushButton(tag,this);
        tagButton->setObjectName(QStringLiteral("TagButton"));
        tagButton->setCheckable(true);
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
}

void LabelPanel::removeTag()
{
    for(auto iter:tagList)
    {
        iter->deleteLater();
    }
    tagList.clear();
}

QStringList LabelPanel::getSelectedTags()
{
    QStringList tags;
    for(auto iter=tagList.begin();iter!=tagList.end();++iter)
    {
        if(iter.value()->isChecked()) tags<<iter.key();
    }
    return tags;
}

CharacterItem::CharacterItem(Character *character, QWidget *parent) : QWidget(parent), crt(character)
{
    iconLabel=new QLabel(this);
    iconLabel->setScaledContents(true);
    iconLabel->setObjectName(QStringLiteral("CrtIcon"));
    iconLabel->setFixedSize(60*logicalDpiX()/96,60*logicalDpiY()/96);
    iconLabel->setAlignment(Qt::AlignCenter);
    refreshIcon();
    QLabel *nameLabel=new QLabel(this);
    nameLabel->setOpenExternalLinks(true);
    nameLabel->setText(QString("<a href = \"http://bgm.tv/character/%1\">%2%3</a>")
                       .arg(character->bangumiID).arg(character->name)
                       .arg(character->name_cn.isEmpty()?"":QString("(%1)").arg(character->name_cn)));

    QLabel *infoLabel=new QLabel(this);
    infoLabel->setText(character->actor);
    infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QHBoxLayout *itemHLayout=new QHBoxLayout(this);
    itemHLayout->addWidget(iconLabel);
    QVBoxLayout *infoVLayout=new QVBoxLayout();
    infoVLayout->addWidget(nameLabel);
    infoVLayout->addWidget(infoLabel);
    itemHLayout->addLayout(infoVLayout);
}

void CharacterItem::refreshIcon()
{
    if(crt->image.isEmpty())
    {
        static QPixmap nullIcon(":/res/images/kikoplay-4.png");
        iconLabel->setPixmap(nullIcon);
    }
    else
    {
        QPixmap icon;
        icon.loadFromData(crt->image);
        iconLabel->setPixmap(icon);
    }
}

void CharacterItem::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)
    {
        if(iconLabel->underMouse())
        {
            if(!crt->imgURL.isEmpty())
            {
                emit updateCharacter(this);
            }
        }
    }
}


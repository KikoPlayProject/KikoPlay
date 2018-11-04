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
#include <QMessageBox>
#include <QAction>
#include <QHeaderView>
#include <QScrollArea>
#include <QListWidget>
#include <QLineEdit>
#include "MediaLibrary/animeinfo.h"
#include "MediaLibrary/animelibrary.h"
#include "MediaLibrary/episodesmodel.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Common/network.h"
namespace
{
class CharacterItem : public QWidget
{
public:
    explicit CharacterItem(Character *crt, QWidget *parent=nullptr):QWidget(parent)
    {
        QLabel *iconLabel=new QLabel(this);
        iconLabel->setScaledContents(true);
        iconLabel->setObjectName(QStringLiteral("CrtIcon"));
        iconLabel->setFixedSize(60*logicalDpiX()/96,60*logicalDpiY()/96);
        iconLabel->setAlignment(Qt::AlignCenter);
        QPixmap icon;
        icon.loadFromData(crt->image);
        iconLabel->setPixmap(icon);

        QLabel *nameLabel=new QLabel(this);
        nameLabel->setOpenExternalLinks(true);
        nameLabel->setText(QString("<a href = \"http://bgm.tv/character/%1\">%2(%3)</a>").arg(crt->bangumiID).arg(crt->name).arg(crt->name_cn));

        QLabel *infoLabel=new QLabel(this);
        infoLabel->setText(crt->actor);

        QHBoxLayout *itemHLayout=new QHBoxLayout(this);
        itemHLayout->addWidget(iconLabel);
        QVBoxLayout *infoVLayout=new QVBoxLayout(this);
        infoVLayout->addWidget(nameLabel);
        infoVLayout->addWidget(infoLabel);
        itemHLayout->addLayout(infoVLayout);
    }
};
}
AnimeDetailInfo::AnimeDetailInfo(Anime *anime, QWidget *parent) :currentAnime(anime),
    CFramelessDialog(tr("Anime Info"),parent,false,true,false)
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

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    pageButtonHLayout->addWidget(overviewPageButton);
    pageButtonHLayout->addWidget(episodesPageButton);
    pageButtonHLayout->addWidget(characterPageButton);
    pageButtonHLayout->addWidget(tagPageButton);
    pageButtonHLayout->addStretch(1);
    dialogVLayout->addLayout(pageButtonHLayout);

    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,5*logicalDpiY()/96,0,0);
    contentStackLayout->addWidget(setupOverviewPage());
    contentStackLayout->addWidget(setupEpisodesPage());
    contentStackLayout->addWidget(setupCharacterPage());
    contentStackLayout->addWidget(setupTagPage());
    dialogVLayout->addSpacing(6*logicalDpiY()/96);
    dialogVLayout->addLayout(contentStackLayout);

    QButtonGroup *btnGroup = new QButtonGroup(this);
    btnGroup->addButton(overviewPageButton, 0);
    btnGroup->addButton(episodesPageButton, 1);
    btnGroup->addButton(characterPageButton, 2);
    btnGroup->addButton(tagPageButton, 3);
    QObject::connect(btnGroup, (void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled, [contentStackLayout](int id, bool checked) {
        if (checked)
        {
            contentStackLayout->setCurrentIndex(id);
        }
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
    if(currentAnime->cover.isEmpty())
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
    episodeView->hideColumn(1);
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
                epNames.append(tr("No.%0 %1(%2)").arg(epobj.value("sort").toInt()).arg(epobj.value("name").toString()).arg(epobj.value("name_cn").toString()));
            }
            getEpNames->setText(tr("Getting Done"));

        }
        catch(Network::NetworkError &error)
        {
            QMessageBox::information(this,tr("Error"),error.errorInfo);
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
            QMessageBox::information(this,"KikoPlay",tr("Add %1 items to Playlist").arg(GlobalObjects::playlist->addItems(items,collectIndex)));
        }
    });
    QAction *deleteAction=new QAction(tr("Delete"),pageWidget);
    QObject::connect(deleteAction,&QAction::triggered,[epModel,episodeView](){
        auto selection = episodeView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        epModel->removeEpisodes(selection);
    });
    QAction *addAction=new QAction(tr("Add to Playlist"),pageWidget);
    QObject::connect(addAction,&QAction::triggered,[epModel,episodeView,this](){
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
        QMessageBox::information(this,"KikoPlay",tr("Add %1 items to Playlist").arg(GlobalObjects::playlist->addItems(items,QModelIndex())));
    });
    QAction *playAction=new QAction(tr("Play"),pageWidget);
    QObject::connect(playAction,&QAction::triggered,[epModel,episodeView,this](){
        auto selection = episodeView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        emit playFile(currentAnime->eps.at(selection.last().row()).localFile);
        CFramelessDialog::onAccept();
    });
    QAction *explorerViewAction=new QAction(tr("Browse File"),pageWidget);
    QObject::connect(explorerViewAction,&QAction::triggered,[this,episodeView](){
        auto selection = episodeView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        QFileInfo info(currentAnime->eps.at(selection.last().row()).localFile);
        if(!info.exists())
        {
            QMessageBox::information(this,"KikoPlay",tr("File not Exist"));
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
    QWidget *contentWidget=new QWidget(pageWidget);
    contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QScrollArea *contentScrollArea=new QScrollArea(pageWidget);
    contentScrollArea->setWidget(contentWidget);
    contentScrollArea->setWidgetResizable(true);
    contentScrollArea->setAlignment(Qt::AlignCenter);
    contentScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QHBoxLayout *cHLayout=new QHBoxLayout(pageWidget);
    cHLayout->setContentsMargins(0,0,0,0);
    cHLayout->addWidget(contentScrollArea);

    QVBoxLayout *pageVLayout=new QVBoxLayout(contentWidget);
    for(auto iter=currentAnime->characters.begin();iter!=currentAnime->characters.end();++iter)
    {
        CharacterItem *crtItem=new CharacterItem(&(*iter),contentWidget);
        pageVLayout->addWidget(crtItem);
    }
    return pageWidget;
}

QWidget *AnimeDetailInfo::setupTagPage()
{
    QWidget *pageWidget=new QWidget(this);
    QListWidget *tagList=new QListWidget(pageWidget);
    tagList->setSelectionMode(QAbstractItemView::SingleSelection);
    tagList->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto &tagMap=GlobalObjects::library->animeTags();
    for(auto iter=tagMap.begin();iter!=tagMap.end();++iter)
    {
        if(iter.value().contains(currentAnime->title))
            tagList->addItem(iter.key());
    }
    QLineEdit *tagEdit=new QLineEdit(pageWidget);
    QPushButton *addTagButton=new QPushButton(tr("Add Tag"), pageWidget);
    QObject::connect(tagEdit,&QLineEdit::returnPressed,addTagButton,&QPushButton::click);
    QObject::connect(addTagButton,&QPushButton::clicked,[tagEdit,tagList,this](){
        QString tag=tagEdit->text().trimmed();
        if(tag.isEmpty())return;
        auto &tagMap=GlobalObjects::library->animeTags();
        if(tagMap.contains(tag) && tagMap[tag].contains(currentAnime->title))return;
        GlobalObjects::library->addTag(currentAnime,tag);
        tagEdit->clear();
        tagList->insertItem(0,tag);
    });
    QAction *deleteTagAction=new QAction(tr("Delete"),pageWidget);
    QObject::connect(deleteTagAction,&QAction::triggered,[tagList,this](){
       auto selectedItems=tagList->selectedItems();
       if(selectedItems.count()==0)return;
       GlobalObjects::library->deleteTag(selectedItems.first()->text(),currentAnime->title);
       delete tagList->takeItem(tagList->row(selectedItems.first()));
    });
    tagList->addAction(deleteTagAction);
    QGridLayout *pageGLayout=new QGridLayout(pageWidget);
    pageGLayout->addWidget(tagEdit,0,0);
    pageGLayout->addWidget(addTagButton,0,1);
    pageGLayout->addWidget(tagList,1,0,1,2);
    pageGLayout->setRowStretch(1,1);
    pageGLayout->setColumnStretch(0,1);
    return pageWidget;
}

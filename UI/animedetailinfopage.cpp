#include "animedetailinfopage.h"
#include <QStackedLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QTextEdit>
#include <QLineEdit>
#include <QGridLayout>
#include <QTreeView>
#include <QAction>
#include <QFileDialog>
#include <QHeaderView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QButtonGroup>
#include <QToolButton>
#include <QMenu>
#include <QApplication>
#include "Common/notifier.h"
#include "MediaLibrary/animeinfo.h"
#include "MediaLibrary/episodesmodel.h"
#include "MediaLibrary/episodeitem.h"
#include "MediaLibrary/labelmodel.h"
#include "MediaLibrary/capturelistmodel.h"
#include "MediaLibrary/animeprovider.h"
#include "MediaLibrary/animeworker.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Common/flowlayout.h"
#include "Common/network.h"
#include "captureview.h"
#include "aliasedit.h"
#include "gifcapture.h"
#include "globalobjects.h"
namespace
{
    class ShadowLabel : public QWidget
    {
    public:
        ShadowLabel(QWidget *parent=nullptr) : QWidget(parent)
        {
            setStyleSheet(QStringLiteral("background:transparent;"));
            coverLabel = new QLabel(this);
            coverLabel->setAlignment(Qt::AlignCenter);
            coverLabel->setScaledContents(true);
            QHBoxLayout *sLayout = new QHBoxLayout(this);
            sLayout->addWidget(coverLabel);
            QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
            shadowEffect->setOffset(0, 0);
            shadowEffect->setBlurRadius(12);
            shadowEffect->setColor(Qt::white);
            coverLabel->setGraphicsEffect(shadowEffect);
            setFixedSize(160*logicalDpiX()/96,220*logicalDpiY()/96);
        }
        QPixmap getShadowPixmap(const QPixmap &cover)
        {
            coverLabel->setPixmap(cover);
            QPixmap pixmap(size());
            pixmap.fill(Qt::transparent);
            render(&pixmap);
            return pixmap;
        }
    private:
        QLabel *coverLabel;
    };
}
AnimeDetailInfoPage::AnimeDetailInfoPage(QWidget *parent) : QWidget(parent), currentAnime(nullptr)
{
    coverLabel=new QLabel(this);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setScaledContents(true);

    coverLabelShadow = new ShadowLabel();

    QAction *actCopyCover = new QAction(tr("Copy Cover"), this);
    QObject::connect(actCopyCover, &QAction::triggered, this, [=](){
        if(currentAnime && !currentAnime->rawCover().isNull())
        {
            QApplication::clipboard()->setPixmap(currentAnime->rawCover());
        }
    });
    QAction *actDownloadCover = new QAction(tr("Re-Download Cover"), this);
    QObject::connect(actDownloadCover, &QAction::triggered, this, [=](){
        if(currentAnime && !currentAnime->coverURL().isEmpty())
        {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Cover Image..."), NM_PROCESS | NM_DARKNESS_BACK);
            Network::Reply reply(Network::httpGet(currentAnime->coverURL(), QUrlQuery()));
            if(reply.hasError)
            {
                Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, reply.errInfo, NM_HIDE | NM_ERROR);
                return;
            }
            currentAnime->setCover(reply.content);
            coverLabel->setPixmap(static_cast<ShadowLabel *>(coverLabelShadow)->getShadowPixmap(currentAnime->rawCover()));
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Down"), NM_HIDE);
        }
    });
    QAction *actLocalCover = new QAction(tr("Select From File"), this);
    QObject::connect(actLocalCover, &QAction::triggered, this, [=](){
        if(currentAnime)
        {
            QString fileName = QFileDialog::getOpenFileName(this, tr("Select Cover"), "", "Image Files(*.jpg *.png);;All Files(*)");
            if(!fileName.isEmpty())
            {
                QImage cover(fileName);
                QByteArray imgBytes;
                QBuffer bufferImage(&imgBytes);
                bufferImage.open(QIODevice::WriteOnly);
                cover.save(&bufferImage, "JPG");
                currentAnime->setCover(imgBytes);
                coverLabel->setPixmap(static_cast<ShadowLabel *>(coverLabelShadow)->getShadowPixmap(currentAnime->rawCover()));
            }
        }
    });
    coverLabel->addAction(actCopyCover);
    coverLabel->addAction(actDownloadCover);
    coverLabel->addAction(actLocalCover);
    coverLabel->setContextMenuPolicy(Qt::ActionsContextMenu);

    QWidget *titleContainer = new QWidget(this);
    titleLabel=new QLabel(titleContainer);
    titleLabel->setObjectName(QStringLiteral("AnimeDetailTitle"));
    titleLabel->setFont(QFont(GlobalObjects::normalFont,20));
    titleLabel->setWordWrap(true);
    titleLabel->setOpenExternalLinks(true);
    titleLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    QAction *actEditAlias = new QAction(tr("Edit Alias"), this);
    QObject::connect(actEditAlias, &QAction::triggered, this, [=](){
        if(currentAnime)
        {
            AliasEdit aliasEdit(currentAnime->name(), this);
            aliasEdit.exec();
        }
    });
    titleLabel->setContextMenuPolicy(Qt::ActionsContextMenu);
    titleLabel->addAction(actEditAlias);
    viewInfoLabel=new QLabel(titleContainer);
    viewInfoLabel->setObjectName(QStringLiteral("AnimeDetailViewInfo"));
    viewInfoLabel->setFont(QFont(GlobalObjects::normalFont,12));
    viewInfoLabel->setWordWrap(true);
    viewInfoLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    viewInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QScrollArea *viewInfoScrollArea = new QScrollArea(titleContainer);
    viewInfoScrollArea->setWidget(viewInfoLabel);
    viewInfoScrollArea->setWidgetResizable(true);

    QHBoxLayout *titleHLayout = new QHBoxLayout(titleContainer);
    titleHLayout->setContentsMargins(0, 0, 0, 0);
    titleHLayout->addWidget(coverLabel);
    QVBoxLayout *textVLayout = new QVBoxLayout();
    textVLayout->addWidget(titleLabel);
    textVLayout->addSpacing(8*logicalDpiY()/96);
    textVLayout->addWidget(viewInfoScrollArea);
    textVLayout->setContentsMargins(0, 0, 0, 0);
    titleHLayout->addItem(textVLayout);

    QWidget *pageContainer = new QWidget(this);
	QFontMetrics fm(font());
	int h = fm.height();
	int w = fm.width(tr("Description"));

    QSize pageButtonSize(80 *logicalDpiX()/96,32*logicalDpiY()/96);
    QToolButton *descPageButton=new QToolButton(pageContainer);
    descPageButton->setText(tr("Description"));
    descPageButton->setCheckable(true);
    descPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    descPageButton->setObjectName(QStringLiteral("AnimeInfoPage"));
    descPageButton->setFixedSize(pageButtonSize);

    QToolButton *episodesPageButton=new QToolButton(pageContainer);
    episodesPageButton->setText(tr("Episodes"));
    episodesPageButton->setCheckable(true);
    episodesPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    episodesPageButton->setObjectName(QStringLiteral("AnimeInfoPage"));
    episodesPageButton->setFixedSize(pageButtonSize);

    QToolButton *characterPageButton=new QToolButton(pageContainer);
    characterPageButton->setText(tr("Characters"));
    characterPageButton->setCheckable(true);
    characterPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    characterPageButton->setObjectName(QStringLiteral("AnimeInfoPage"));
    characterPageButton->setFixedSize(pageButtonSize);

    QToolButton *tagPageButton=new QToolButton(pageContainer);
    tagPageButton->setText(tr("Tag"));
    tagPageButton->setCheckable(true);
    tagPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    tagPageButton->setObjectName(QStringLiteral("AnimeInfoPage"));
    tagPageButton->setFixedSize(pageButtonSize);

    QToolButton *capturePageButton=new QToolButton(pageContainer);
    capturePageButton->setText(tr("Capture"));
    capturePageButton->setCheckable(true);
    capturePageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    capturePageButton->setObjectName(QStringLiteral("AnimeInfoPage"));
    capturePageButton->setFixedSize(pageButtonSize);

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    pageButtonHLayout->addWidget(descPageButton);
    pageButtonHLayout->addWidget(episodesPageButton);
    pageButtonHLayout->addWidget(characterPageButton);
    pageButtonHLayout->addWidget(tagPageButton);
    pageButtonHLayout->addWidget(capturePageButton);
    pageButtonHLayout->addStretch(1);

    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupDescriptionPage());
    contentStackLayout->addWidget(setupEpisodesPage());
    contentStackLayout->addWidget(setupCharacterPage());
    contentStackLayout->addWidget(setupTagPage());
    contentStackLayout->addWidget(setupCapturePage());
    QButtonGroup *btnGroup = new QButtonGroup(this);
    btnGroup->addButton(descPageButton, 0);
    btnGroup->addButton(episodesPageButton, 1);
    btnGroup->addButton(characterPageButton, 2);
    btnGroup->addButton(tagPageButton, 3);
    btnGroup->addButton(capturePageButton,4);
    QObject::connect(btnGroup, (void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled, [contentStackLayout](int id, bool checked) {
        if (checked)
            contentStackLayout->setCurrentIndex(id);
    });
    descPageButton->setChecked(true);

    QVBoxLayout *pageVLayout = new QVBoxLayout(pageContainer);
    pageVLayout->setContentsMargins(0,0,0,0);
    pageVLayout->addLayout(pageButtonHLayout);
    pageVLayout->addSpacing(5*logicalDpiY()/96);
    pageVLayout->addLayout(contentStackLayout);

    QGridLayout *pageGLayout=new QGridLayout(this);
    pageGLayout->setContentsMargins(5*logicalDpiY()/96,5*logicalDpiY()/96,0,0);
    pageGLayout->addWidget(titleContainer, 0, 0);
    pageGLayout->addWidget(pageContainer, 1, 0);
    pageGLayout->setRowStretch(1,1);
}

void AnimeDetailInfoPage::setAnime(Anime *anime)
{
    currentAnime = anime;
    epModel->setAnime(nullptr);
    epDelegate->setAnime(nullptr);
    epNames.clear();
    characterList->clear();
    tagPanel->clear();
    captureModel->setAnimeName("");
    if(!currentAnime)
    {
        return;
    }
    static QPixmap nullCover(":/res/images/cover.png");
    coverLabel->setPixmap(static_cast<ShadowLabel *>(coverLabelShadow)->getShadowPixmap(currentAnime->rawCover().isNull()?nullCover:currentAnime->rawCover()));

    titleLabel->setText(QString("<style> a {text-decoration: none} </style><a style='color: white;' href = \"%1\">%2</a>").arg(currentAnime->url(), currentAnime->name()));
    QStringList stafflist;
    for(const auto &p: currentAnime->staffList())
    {
        stafflist.append(p.first+": "+p.second);
    }
    viewInfoLabel->setToolTip("");
    viewInfoLabel->setText(QObject::tr("Add Time: %1\nDate: %3\nEps: %2\n%4").arg(currentAnime->addTimeStr()).arg(currentAnime->epCount()).
                           arg(currentAnime->airDate()).arg(stafflist.join('\n')));
    viewInfoLabel->adjustSize();

    descInfo->setText(currentAnime->description());
    epModel->setAnime(currentAnime);
    epDelegate->setAnime(currentAnime);
    for(auto &character: currentAnime->crList(true))
    {
        CharacterWidget *crtItem=new CharacterWidget(&character);
        QObject::connect(crtItem,&CharacterWidget::updateCharacter,this,[this](CharacterWidget *crtItem){
            if(crtItem->crt->imgURL.isEmpty()) return;
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Character Image..."), NotifyMessageFlag::NM_PROCESS | NotifyMessageFlag::NM_DARKNESS_BACK);
            crtItem->setEnabled(false);
            Network::Reply reply(Network::httpGet(crtItem->crt->imgURL, QUrlQuery()));
            crtItem->setEnabled(true);
            if(reply.hasError)
            {
                Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, reply.errInfo, NM_HIDE | NM_ERROR);
                return;
            }

            QBuffer bufferImage(&reply.content);
            bufferImage.open(QIODevice::ReadOnly);
            QImageReader reader(&bufferImage);
            QSize s = reader.size();
            int w = qMin(s.width(), s.height());
            reader.setScaledClipRect(QRect(0, 0, w, w));

            QPixmap tmp = QPixmap::fromImageReader(&reader);
            Character::scale(tmp);
            QByteArray imgBytes;
            bufferImage.close();
            bufferImage.setBuffer(&imgBytes);
            bufferImage.open(QIODevice::WriteOnly);
            tmp.save(&bufferImage, "JPG");

            currentAnime->setCrtImage(crtItem->crt->name, imgBytes);
            crtItem->refreshIcon();
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Down"), NotifyMessageFlag::NM_HIDE);
        });
        QObject::connect(crtItem, &CharacterWidget::selectLocalImage, this, [this](CharacterWidget *crtItem){
            QString fileName = QFileDialog::getOpenFileName(this, tr("Select Image"), "", "Image Files(*.jpg *.png);;All Files(*)");
            if(!fileName.isEmpty())
            {
                QImage cover(fileName);
                QByteArray imgBytes;
                QBuffer bufferImage(&imgBytes);
                bufferImage.open(QIODevice::WriteOnly);
                cover.save(&bufferImage, "JPG");
                currentAnime->setCrtImage(crtItem->crt->name, imgBytes);
                crtItem->refreshIcon();
            }
        });
        QListWidgetItem *listItem=new QListWidgetItem(characterList);
        characterList->setItemWidget(listItem, crtItem);
        listItem->setSizeHint(crtItem->sizeHint());
    }
    auto &tagMap=LabelModel::instance()->customTags();
    QStringList tags;
    for(auto iter=tagMap.begin();iter!=tagMap.end();++iter)
    {
        if(iter.value().contains(currentAnime->name()))
            tags<<iter.key();
    }
    tagPanel->addTag(tags);
    tagContainerSLayout->setCurrentIndex(0);
    captureModel->setAnimeName(currentAnime->name());
}


QWidget *AnimeDetailInfoPage::setupDescriptionPage()
{
    descInfo=new QTextEdit(this);
    descInfo->setObjectName(QStringLiteral("AnimeDetailDesc"));
    descInfo->setReadOnly(true);
    return descInfo;
}

QWidget *AnimeDetailInfoPage::setupEpisodesPage()
{
    QTreeView *episodeView=new QTreeView(this);
    episodeView->setObjectName(QStringLiteral("AnimeDetailEpisode"));
    episodeView->setRootIsDecorated(false);
    episodeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    epDelegate = new EpItemDelegate(this);
    episodeView->setItemDelegate(epDelegate);
    episodeView->setFont(font());
    epModel=new EpisodesModel(nullptr,this);
    episodeView->setModel(epModel);
    episodeView->setAlternatingRowColors(true);
    QAction *addEpAction=new QAction(tr("Add Episode(s)"), this);
    QObject::connect(addEpAction,&QAction::triggered,[this](){
        QStringList files = QFileDialog::getOpenFileNames(this,tr("Select media files"),"",
                                                         QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
        if (files.count())
        {
            for(auto &file:files)
            {
                epModel->addEp(file);
            }
        }
    });

    QAction *deleteAction=new QAction(tr("Delete"), this);
    QObject::connect(deleteAction,&QAction::triggered,[this,episodeView](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.size() == 0)return;
        QModelIndex selectedIndex = selection.last();
        if(selectedIndex.isValid())
        {
            int epCount = epModel->rowCount(QModelIndex());
            QHash<QString, int> pathCounter;
            for(int i = 0; i<epCount; ++i)
            {
                QString file(epModel->data(epModel->index(i, (int)EpisodesModel::Columns::LOCALFILE, QModelIndex()), Qt::DisplayRole).toString());
                ++pathCounter[file.mid(0, file.lastIndexOf('/'))];
            }
            QString localFile = epModel->data(selectedIndex, Qt::DisplayRole).toString();
            QString filePath = localFile.mid(0, localFile.lastIndexOf('/'));
            if(pathCounter.value(filePath, 0)<2)
            {
                LabelModel::instance()->removeTag(currentAnime->name(), filePath, TagNode::TAG_FILE);
            }
            epModel->removeEp(selection.last());
        }
    });
    QAction *deleteInvalidAction=new QAction(tr("Delete Invalid Episodes"), this);
    QObject::connect(deleteInvalidAction,&QAction::triggered,[this](){
        Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Remove %1 invalid episodes").arg(epModel->removeInvalidEp()),
                                             NotifyMessageFlag::NM_HIDE);
    });
    QAction *addAction=new QAction(tr("Add to Playlist"), this);
    QObject::connect(addAction,&QAction::triggered,[episodeView,this](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.size() == 0)return;
        QStringList items;
        for(const QModelIndex &index : selection)
        {
            if (index.isValid())
            {
                QString localFile = epModel->data(index, Qt::DisplayRole).toString();
                if(QFile::exists(localFile)) items<<localFile;
            }
        }
        Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Add %1 items to Playlist").arg(GlobalObjects::playlist->addItems(items,QModelIndex())),
                                             NotifyMessageFlag::NM_HIDE);
    });
    QAction *playAction=new QAction(tr("Play"), this);
    QObject::connect(playAction,&QAction::triggered,[episodeView,this](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.size() == 0)return;
        QFileInfo info(epModel->data(selection.last(), Qt::DisplayRole).toString());
        if(!info.exists())
        {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("File Not Exist"), NotifyMessageFlag::NM_ERROR|NotifyMessageFlag::NM_HIDE);
            return;
        }
        emit playFile(info.absoluteFilePath());
    });
    QAction *explorerViewAction=new QAction(tr("Browse File"), this);
    QObject::connect(explorerViewAction,&QAction::triggered,[this,episodeView](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.size() == 0)return;
        QFileInfo info(epModel->data(selection.last(), Qt::DisplayRole).toString());
        if(!info.exists())
        {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("File Not Exist"), NotifyMessageFlag::NM_ERROR|NotifyMessageFlag::NM_HIDE);
            return;
        }
        QProcess::startDetached("Explorer", {"/select,", QDir::toNativeSeparators(info.absoluteFilePath())});
    });

    QAction *autoGetEpNames=new QAction(tr("Auto Get Epsoide Names"),this);
    autoGetEpNames->setCheckable(true);
    autoGetEpNames->setChecked(true);
    QObject::connect(autoGetEpNames, &QAction::toggled, epDelegate, &EpItemDelegate::setAutoGetEpInfo);

    episodeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    episodeView->addAction(playAction);
    episodeView->addAction(addAction);
#ifdef Q_OS_WIN
    episodeView->addAction(explorerViewAction);
#endif
    episodeView->addAction(addEpAction);
    episodeView->addAction(deleteAction);
    episodeView->addAction(deleteInvalidAction);
    episodeView->addAction(autoGetEpNames);

    QHeaderView *epHeader = episodeView->header();
    epHeader->setFont(font());
    epHeader->resizeSection(0, 260*logicalDpiX()/96);
    epHeader->resizeSection(1, 160*logicalDpiX()/96);
    return episodeView;
}

QWidget *AnimeDetailInfoPage::setupCharacterPage()
{
    characterList = new QListWidget(this);
    characterList->setObjectName(QStringLiteral("AnimeDetailCharacterList"));
    characterList->setViewMode(QListView::IconMode);
    characterList->setResizeMode(QListView::Adjust);
    characterList->setMovement(QListView::Static);
    characterList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return characterList;
}

QWidget *AnimeDetailInfoPage::setupTagPage()
{
    QWidget *container = new QWidget(this);
    tagContainerSLayout = new QStackedLayout(container);
    tagContainerSLayout->setContentsMargins(0,0,0,0);

    tagPanel = new TagPanel(container,true,false,true);
    QObject::connect(LabelModel::instance(), &LabelModel::tagRemoved, tagPanel, &TagPanel::removeTag);
    QObject::connect(tagPanel, &TagPanel::deleteTag, [this](const QString &tag){
        LabelModel::instance()->removeTag(currentAnime->name(), tag, TagNode::TAG_CUSTOM);
    });
    QObject::connect(tagPanel,&TagPanel::tagAdded, [this](const QString &tag){
        QStringList tags{tag};
        LabelModel::instance()->addCustomTags(currentAnime->name(), tags);
        tagPanel->addTag(tags);
    });
    TagPanel *webTagPanel = new TagPanel(container,false,true);
    tagContainerSLayout->addWidget(tagPanel);
    tagContainerSLayout->addWidget(webTagPanel);

    QAction *webTagAction=new QAction(tr("Tags on Web"), this);
    tagPanel->addAction(webTagAction);
    QObject::connect(webTagAction,&QAction::triggered,this,[this, webTagAction, webTagPanel](){
        if(currentAnime->scriptId().isEmpty()) return;
        QStringList tags;
        webTagAction->setEnabled(false);
        Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Tags..."), NotifyMessageFlag::NM_DARKNESS_BACK | NotifyMessageFlag::NM_PROCESS);
        ScriptState state = GlobalObjects::animeProvider->getTags(currentAnime, tags);
        if(state)
        {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Add the Selected Tags from the Right-Click Menu"), NotifyMessageFlag::NM_HIDE);
            webTagPanel->clear();
            webTagPanel->addTag(tags);
            webTagPanel->setChecked(tagPanel->tags());
            tagContainerSLayout->setCurrentIndex(1);
        } else {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, state.info, NotifyMessageFlag::NM_ERROR | NotifyMessageFlag::NM_HIDE);
        }
        webTagAction->setEnabled(true);
    });

    QAction *webTagOKAction=new QAction(tr("Add Selected Tags"), this);
    QObject::connect(webTagOKAction,&QAction::triggered,this,[this,webTagPanel](){
        QStringList tags(webTagPanel->getSelectedTags());
        if(tags.count()==0) return;
        LabelModel::instance()->addCustomTags(currentAnime->name(), tags);
        tagPanel->clear();
        auto &tagMap=LabelModel::instance()->customTags();
        tags.clear();
        for(auto iter=tagMap.begin();iter!=tagMap.end();++iter)
        {
            if(iter.value().contains(currentAnime->name()))
                tags.append(iter.key());
        }
        tagPanel->addTag(tags);
        tagContainerSLayout->setCurrentIndex(0);
    });
    QAction *webTagCancelAction=new QAction(tr("Cancel"), this);
    QObject::connect(webTagCancelAction,&QAction::triggered,this,[this](){
       tagContainerSLayout->setCurrentIndex(0);
    });
    webTagPanel->addAction(webTagOKAction);
    webTagPanel->addAction(webTagCancelAction);
    return container;
}

QWidget *AnimeDetailInfoPage::setupCapturePage()
{
    captureModel=new CaptureListModel("",this);
    QObject::connect(captureModel,&CaptureListModel::fetching,this,[](bool on){
        if(on)  Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Loading..."), NotifyMessageFlag::NM_PROCESS);
        else Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Loading..."), NotifyMessageFlag::NM_HIDE);
    });
    QListView *captureView=new QListView(this);
    captureView->setObjectName(QStringLiteral("captureView"));
    captureView->setSelectionMode(QAbstractItemView::SingleSelection);
    captureView->setIconSize(QSize(200*logicalDpiX()/96,112*logicalDpiY()/96));
    captureView->setViewMode(QListView::IconMode);
    captureView->setUniformItemSizes(true);
    captureView->setResizeMode(QListView::Adjust);
    captureView->setMovement(QListView::Static);
    captureView->setWordWrap(true);
    captureView->setEditTriggers(QAbstractItemView::SelectedClicked);

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
    QObject::connect(actCopy, &QAction::triggered,[captureView,this](bool)
    {
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        const AnimeImage *item=captureModel->getCaptureItem(selection.first().row());
        if(item->type == AnimeImage::CAPTURE)
        {
            QPixmap img(captureModel->getFullCapture(selection.first().row()));
            QApplication::clipboard()->setPixmap(img);
        }
    });
    QAction* actSave = new QAction(tr("Save"),this);
    QObject::connect(actSave, &QAction::triggered,[this,captureView](bool)
    {
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        const AnimeImage *item=captureModel->getCaptureItem(selection.first().row());
        if(item->type == AnimeImage::CAPTURE)
        {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save Capture"), item->info, "JPEG Images (*.jpg);;PNG Images (*.png)");
            if(!fileName.isEmpty())
            {
                QPixmap img(captureModel->getFullCapture(selection.first().row()));
                img.save(fileName);
            }
        }
        else if(item->type == AnimeImage::SNIPPET)
        {
            QString snippetFilePath(captureModel->getSnippetFile(selection.first().row()));
            if(snippetFilePath.isEmpty())
            {
                Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Snippet %1 Lost").arg(item->timeId), NM_HIDE | NM_ERROR);
                return;
            }
            else
            {
                QFileInfo snippetFile(snippetFilePath);
                QString fileName = QFileDialog::getSaveFileName(this, tr("Save Snippet"), item->info, QString("(*.%1)").arg(snippetFile.suffix()));
                if(!fileName.isEmpty())
                {
                    QFile::copy(snippetFilePath, fileName);
                }
            }
        }

    });
    QAction* actSaveGIF = new QAction(tr("Save As GIF"),this);
    QObject::connect(actSaveGIF, &QAction::triggered,[this,captureView](bool)
    {
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        const AnimeImage *item=captureModel->getCaptureItem(selection.first().row());
        if(item->type != AnimeImage::SNIPPET) return;


        QString snippetFilePath(captureModel->getSnippetFile(selection.first().row()));
        if(snippetFilePath.isEmpty())
        {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Snippet %1 Lost").arg(item->timeId), NM_HIDE | NM_ERROR);
            return;
        }
        else
        {
            GIFCapture gifCapture(snippetFilePath, false, this);
            gifCapture.exec();
            GlobalObjects::mpvplayer->setMute(GlobalObjects::mpvplayer->getMute());
        }
    });
    QAction* actAdd = new QAction(tr("Add"),this);
    QObject::connect(actAdd, &QAction::triggered,[=](bool)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Image"), "", "JPEG Images (*.jpg);;PNG Images (*.png)");
        if(!fileName.isEmpty())
        {
            AnimeWorker::instance()->saveCapture(currentAnime->name(), QFileInfo(fileName).fileName(), QImage(fileName));
        }
    });
    QAction* actRemove = new QAction(tr("Remove"),this);
    QObject::connect(actRemove, &QAction::triggered,[captureView,this](bool)
    {
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
		captureModel->deleteRow(selection.first().row());
    });
    QAction *explorerViewAction=new QAction(tr("Browse File"), this);
    QObject::connect(explorerViewAction,&QAction::triggered,[=](){
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        QString snippetFilePath(captureModel->getSnippetFile(selection.first().row()));
        if(!snippetFilePath.isEmpty())
        {
            QFileInfo snippetFile(snippetFilePath);
            QProcess::startDetached("Explorer", {"/select,", QDir::toNativeSeparators(snippetFile.absoluteFilePath())});
        }
    });

    QMenu *contexMenu = new QMenu(this);
    QActionGroup *group=new QActionGroup(this);
    group->addAction(actIconMode);
    group->addAction(actListMode);
    actIconMode->setChecked(true);
    contexMenu->addAction(actCopy);
    contexMenu->addAction(actSave);
    contexMenu->addAction(actSaveGIF);
    contexMenu->addAction(actAdd);
    contexMenu->addAction(actRemove);
    contexMenu->addAction(explorerViewAction);
    contexMenu->addSeparator();
    contexMenu->addAction(actIconMode);
    contexMenu->addAction(actListMode);
    captureView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(captureView, &QListView::customContextMenuRequested, [=]()
    {
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.size()==0)
        {
            actCopy->setEnabled(false);
            actSave->setEnabled(false);
            actSaveGIF->setEnabled(false);
            actRemove->setEnabled(false);
            explorerViewAction->setEnabled(false);
        }
        else
        {
            const AnimeImage *item=captureModel->getCaptureItem(selection.first().row());
            actCopy->setEnabled(item->type == AnimeImage::CAPTURE);
            actSave->setEnabled(true);
            actRemove->setEnabled(true);
            explorerViewAction->setEnabled(item->type == AnimeImage::SNIPPET);
            actSaveGIF->setEnabled(item->type == AnimeImage::SNIPPET);
        }
        contexMenu->exec(QCursor::pos());
    });
    QObject::connect(captureView,&QListView::doubleClicked,[this](const QModelIndex &index){
        CaptureView view(captureModel,index.row(),this->parentWidget());
        view.exec();
    });
    return captureView;
}

CharacterWidget::CharacterWidget(const Character *character, QWidget *parent) : QWidget(parent), crt(character)
{
    iconLabel=new QLabel(this);
    iconLabel->setScaledContents(true);
    iconLabel->setObjectName(QStringLiteral("CrtIcon"));
    iconLabel->setFixedSize(60*logicalDpiX()/96,60*logicalDpiY()/96);
    iconLabel->setAlignment(Qt::AlignCenter);

    QAction *actCopyImage = new QAction(tr("Copy Image"), this);
    QObject::connect(actCopyImage, &QAction::triggered, this, [=](){
        if(crt->image.isNull()) return;
        QApplication::clipboard()->setPixmap(crt->image);
    });
    QAction *actDownloadImage = new QAction(tr("Re-Download Image"), this);
    QObject::connect(actDownloadImage, &QAction::triggered, this, [=](){
        if(!crt->imgURL.isEmpty())
            emit updateCharacter(this);
    });
    QAction *actLocalImage = new QAction(tr("Select From File"), this);
    QObject::connect(actLocalImage, &QAction::triggered, this, [=](){
        emit selectLocalImage(this);
    });

    iconLabel->addAction(actCopyImage);
    iconLabel->addAction(actDownloadImage);
    iconLabel->addAction(actLocalImage);
    iconLabel->setContextMenuPolicy(Qt::ActionsContextMenu);

    refreshIcon();
    QLabel *nameLabel=new QLabel(this);
    nameLabel->setOpenExternalLinks(true);
    nameLabel->setText(QString("<a style='color: rgb(96, 208, 252);' href = \"%1\">%2</a>")
                       .arg(character->link).arg(character->name));
    nameLabel->setFixedWidth(200*logicalDpiX()/96);
    nameLabel->setToolTip(character->name);

    QLabel *infoLabel=new QLabel(this);
    infoLabel->setObjectName(QStringLiteral("AnimeDetailCharactorTitle"));
    infoLabel->setText(character->actor);
    infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QHBoxLayout *itemHLayout=new QHBoxLayout(this);
    itemHLayout->addWidget(iconLabel);
    QVBoxLayout *infoVLayout=new QVBoxLayout();
    infoVLayout->addWidget(nameLabel);
    infoVLayout->addWidget(infoLabel);
    itemHLayout->addLayout(infoVLayout);
}

void CharacterWidget::refreshIcon()
{
    if(crt->image.isNull())
    {
        static QPixmap nullIcon(":/res/images/kikoplay-4.png");
        iconLabel->setPixmap(nullIcon);
    }
    else
    {
        iconLabel->setPixmap(crt->image);
    }
}

TagPanel::TagPanel(QWidget *parent, bool allowDelete, bool checkAble, bool allowAdd):QWidget(parent),
    currentTagButton(nullptr), allowCheck(checkAble), adding(false)
{
    setLayout(new FlowLayout(this));

    tagEdit = new QLineEdit(this);
    tagEdit->setFont(QFont(GlobalObjects::normalFont,14));
    tagEdit->hide();
    setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction *actAddTag=new QAction(tr("Add"),this);
    QObject::connect(actAddTag,&QAction::triggered,[this](){
        if(adding) return;
        adding = true;
        tagEdit->clear();
        layout()->addWidget(tagEdit);
        tagEdit->show();
        tagEdit->setFocus();
    });
    QObject::connect(tagEdit, &QLineEdit::editingFinished, this, [this](){
       adding = false;
       layout()->removeWidget(tagEdit);
       QString tag=tagEdit->text().trimmed();
       tagEdit->hide();
       if(tagList.contains(tag) || tag.isEmpty()) return;
       emit tagAdded(tag);
    });

    addAction(actAddTag);
    actAddTag->setEnabled(allowAdd);

    tagContextMenu = new QMenu(this);
    QAction *actCopy=new QAction(tr("Copy"),this);
    tagContextMenu->addAction(actCopy);
    QObject::connect(actCopy,&QAction::triggered,[this](){
        if(!currentTagButton) return;
        QApplication::clipboard()->setText(currentTagButton->text());
    });
    QAction *actRemoveTag=new QAction(tr("Delete"),this);
    tagContextMenu->addAction(actRemoveTag);
    QObject::connect(actRemoveTag,&QAction::triggered,[this](){
        if(!currentTagButton) return;
        emit deleteTag(currentTagButton->text());
        tagList.remove(currentTagButton->text());
        currentTagButton->deleteLater();
        currentTagButton = nullptr;
    });
    actRemoveTag->setEnabled(allowDelete);

}

void TagPanel::addTag(const QStringList &tags)
{
    layout()->removeWidget(tagEdit);
    tagEdit->hide();
    adding = false;
    for(const QString &tag:tags)
    {
        if(tagList.contains(tag))continue;
        QPushButton *tagButton=new QPushButton(tag,this);
        tagButton->setObjectName(QStringLiteral("TagButton"));
        tagButton->setCheckable(allowCheck);
        tagButton->setFont(QFont(GlobalObjects::normalFont,14));
        tagButton->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(tagButton, &QPushButton::customContextMenuRequested,this,[this, tagButton](const QPoint &pos){
            currentTagButton = tagButton;
            tagContextMenu->exec(tagButton->mapToGlobal(pos));
        });
        layout()->addWidget(tagButton);
        tagList.insert(tag,tagButton);
    }
}

void TagPanel::removeTag(const QString &tag)
{
    if(tagList.contains(tag))
    {
        QPushButton *btn = tagList[tag];
        btn->deleteLater();
        tagList.remove(tag);
    }
}

void TagPanel::setChecked(const QStringList &tags, bool checked)
{
    for(auto &tag: tags)
    {
        if(tagList.contains(tag)) tagList[tag]->setChecked(checked);
    }
}


void TagPanel::clear()
{
    for(auto tagButton:tagList)
    {
        tagButton->deleteLater();
    }
    tagList.clear();
}

QStringList TagPanel::getSelectedTags()
{
    QStringList tags;
    for(auto iter=tagList.cbegin();iter!=tagList.cend();++iter)
    {
        if(iter.value()->isChecked()) tags<<iter.key();
    }
    return tags;
}

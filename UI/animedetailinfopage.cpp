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
#include <QDesktopServices>
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
#include "gifcapture.h"
#include "charactereditor.h"
#include "animeinfoeditor.h"
#include "globalobjects.h"

int CharacterWidget::maxCrtItemWidth = 0;
int CharacterWidget::crtItemHeight = 0;

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
    titleLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    QObject::connect(titleLabel, &QLabel::linkActivated, this, [=](const QString &url){
        if(url == editAnimeURL && currentAnime)
        {
            AnimeInfoEditor editor(currentAnime, this);
            if(QDialog::Accepted == editor.exec())
            {
                QVector<std::function<void (QSqlQuery *)>> queries;
                if(editor.airDateChanged)
                {
                    queries.append(AnimeWorker::instance()->updateAirDate(currentAnime->name(), editor.airDate(), currentAnime->airDate()));
                    currentAnime->setAirDate(editor.airDate());
                }
                if(editor.epsChanged)
                {
                    queries.append(AnimeWorker::instance()->updateEpCount(currentAnime->name(), editor.epCount()));
                    currentAnime->setEpCount(editor.epCount());
                }
                if(editor.staffChanged)
                {
                    queries.append(AnimeWorker::instance()->updateStaffInfo(currentAnime->name(), editor.staffs()));
                    currentAnime->setStaffs(editor.staffs());
                }
                if(editor.descChanged)
                {
                    queries.append(AnimeWorker::instance()->updateDescription(currentAnime->name(), editor.desc()));
                    currentAnime->setDesc(editor.desc());
                }
                if(!queries.isEmpty())
                {
                    AnimeWorker::instance()->runQueryGroup(queries);
                    refreshCurInfo();
                }
            }
            return;
        }
        QDesktopServices::openUrl(QUrl(url));
    });

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

    QStringList pageButtonTexts = {
        tr("Description"),
        tr("Episodes"),
        tr("Characters"),
        tr("Tag"),
        tr("Capture")
    };
    QFontMetrics fm = QToolButton().fontMetrics();
    int btnHeight = fm.height() + 10*logicalDpiY()/96;
    int btnWidth = 0;
    for(const QString &t : pageButtonTexts)
    {
        btnWidth = qMax(btnWidth, fm.horizontalAdvance(t));
    }
    btnWidth += 40*logicalDpiX()/96;

    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);

    QButtonGroup *btnGroup = new QButtonGroup(this);

    for(int i = 0; i < pageButtonTexts.size(); ++i)
    {
        QToolButton *btn = new QToolButton(pageContainer);
        btn->setText(pageButtonTexts[i]);
        btn->setCheckable(true);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setObjectName(QStringLiteral("AnimeInfoPage"));
        btn->setFixedSize(QSize(btnWidth, btnHeight));
        pageButtonHLayout->addWidget(btn);
        btnGroup->addButton(btn, i);
    }
    pageButtonHLayout->addStretch(1);
    QStackedLayout *contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupDescriptionPage());
    contentStackLayout->addWidget(setupEpisodesPage());
    contentStackLayout->addWidget(setupCharacterPage());
    contentStackLayout->addWidget(setupTagPage());
    contentStackLayout->addWidget(setupCapturePage());

    QObject::connect(btnGroup, (void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled, [contentStackLayout](int id, bool checked) {
        if (checked)
            contentStackLayout->setCurrentIndex(id);
    });
    btnGroup->button(0)->setChecked(true);

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

    refreshCurInfo();

    epModel->setAnime(currentAnime);
    epDelegate->setAnime(currentAnime);

    captureModel->setAnimeName(currentAnime->name());

    setCharacters();
    setTags();
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
#ifdef Q_OS_WIN
        QProcess::startDetached("Explorer", {"/select,", QDir::toNativeSeparators(info.absoluteFilePath())});
#else
        QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(info.absolutePath())));
#endif
    });

    QAction *autoGetEpNames=new QAction(tr("Auto Get Epsoide Names"),this);
    autoGetEpNames->setCheckable(true);
    autoGetEpNames->setChecked(true);
    QObject::connect(autoGetEpNames, &QAction::toggled, epDelegate, &EpItemDelegate::setAutoGetEpInfo);

    episodeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    episodeView->addAction(playAction);
    episodeView->addAction(addAction);
    episodeView->addAction(explorerViewAction);
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
#ifdef Q_OS_WIN
            QProcess::startDetached("Explorer", {"/select,", QDir::toNativeSeparators(snippetFile.absoluteFilePath())});
#else
            QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(snippetFile.absolutePath())));
#endif
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

void AnimeDetailInfoPage::refreshCurInfo()
{
    titleLabel->setText(QString("<style> a {text-decoration: none} </style><a style='color: white;' href = \"%1\">%2</a> &nbsp;&nbsp;"
                                "<font face=\"%3\"><a style='color: white;' href = \"%4\">%5</a></font>")
                        .arg(currentAnime->url(), currentAnime->name(), GlobalObjects::iconfont->family(), editAnimeURL, "&#xe60a;"));
    QStringList stafflist;
    for(const auto &p: currentAnime->staffList())
    {
        stafflist.append(p.first+": "+p.second);
    }
    viewInfoLabel->setText(QObject::tr("Add Time: %1\nDate: %3\nEps: %2\n%4").arg(currentAnime->addTimeStr()).arg(currentAnime->epCount()).
                           arg(currentAnime->airDate()).arg(stafflist.join('\n')));
    viewInfoLabel->adjustSize();

    descInfo->setText(currentAnime->description());
}

CharacterWidget *AnimeDetailInfoPage::createCharacterWidget(const Character &crt)
{
    CharacterWidget *crtItem=new CharacterWidget(crt);
    QObject::connect(crtItem, &CharacterWidget::updateCharacter, this, &AnimeDetailInfoPage::updateCharacter);
    QObject::connect(crtItem, &CharacterWidget::selectLocalImage, this, &AnimeDetailInfoPage::selectLocalCharacterImage);
    QObject::connect(crtItem, &CharacterWidget::modifyCharacter, this, &AnimeDetailInfoPage::modifyCharacter);
    QObject::connect(crtItem, &CharacterWidget::removeCharacter, this, &AnimeDetailInfoPage::removeCharacter);
    return crtItem;
}

void AnimeDetailInfoPage::setCharacters()
{
    CharacterPlaceholderWidget *addCharacter = new CharacterPlaceholderWidget;
    QObject::connect(addCharacter, &CharacterPlaceholderWidget::addCharacter, this, &AnimeDetailInfoPage::addCharacter);
    CharacterWidget::maxCrtItemWidth = addCharacter->sizeHint().width();
    CharacterWidget::crtItemHeight = 0;
    QVector<CharacterWidget *> crtWidgets;
    for(auto &character: currentAnime->crList(true))
    {
        CharacterWidget *crtItem = createCharacterWidget(character);
        CharacterWidget::maxCrtItemWidth = qMax(CharacterWidget::maxCrtItemWidth, crtItem->sizeHint().width());
        CharacterWidget::crtItemHeight = crtItem->sizeHint().height();
        crtWidgets.append(crtItem);
    }

    CharacterWidget::maxCrtItemWidth = qMin(CharacterWidget::maxCrtItemWidth, 240 * logicalDpiX()/96);
    CharacterWidget::crtItemHeight = qMax(CharacterWidget::crtItemHeight, 70 * logicalDpiY() / 96);

    for(auto crtItem: crtWidgets)
    {
        QListWidgetItem *listItem=new QListWidgetItem(characterList);
        characterList->setItemWidget(listItem, crtItem);
        listItem->setSizeHint(QSize(CharacterWidget::maxCrtItemWidth, CharacterWidget::crtItemHeight));
    }

    QListWidgetItem *listItem=new QListWidgetItem(characterList);
    characterList->setItemWidget(listItem, addCharacter);
    listItem->setSizeHint(QSize(CharacterWidget::maxCrtItemWidth, CharacterWidget::crtItemHeight));
}

void AnimeDetailInfoPage::setTags()
{
    auto &tagMap=LabelModel::instance()->customTags();
    QStringList tags;
    for(auto iter=tagMap.begin();iter!=tagMap.end();++iter)
    {
        if(iter.value().contains(currentAnime->name()))
            tags<<iter.key();
    }
    tagPanel->addTag(tags);
    tagContainerSLayout->setCurrentIndex(0);
}

void AnimeDetailInfoPage::updateCharacter(CharacterWidget *crtItem)
{
    if(crtItem->crt.imgURL.isEmpty()) return;
    Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Character Image..."), NotifyMessageFlag::NM_PROCESS | NotifyMessageFlag::NM_DARKNESS_BACK);
    crtItem->setEnabled(false);
    Network::Reply reply(Network::httpGet(crtItem->crt.imgURL, QUrlQuery()));
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

    currentAnime->setCrtImage(crtItem->crt.name, imgBytes);
    crtItem->refreshIcon(imgBytes);
    Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Down"), NotifyMessageFlag::NM_HIDE);
}

void AnimeDetailInfoPage::selectLocalCharacterImage(CharacterWidget *crtItem)
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Image"), "", "Image Files(*.jpg *.png);;All Files(*)");
    if(!fileName.isEmpty())
    {
        QImage cover(fileName);
        QByteArray imgBytes;
        QBuffer bufferImage(&imgBytes);
        bufferImage.open(QIODevice::WriteOnly);
        cover.save(&bufferImage, "JPG");
        currentAnime->setCrtImage(crtItem->crt.name, imgBytes);
        crtItem->refreshIcon(imgBytes);
    }
}

void AnimeDetailInfoPage::addCharacter()
{
    if(!currentAnime) return;
    CharacterEditor editor(currentAnime, nullptr, this);
    if(QDialog::Accepted == editor.exec())
    {
        delete characterList->item(characterList->count()-1);
        Character newCrt;
        newCrt.name = editor.name;
        newCrt.link = editor.link;
        newCrt.actor = editor.actor;
        currentAnime->addCharacter(newCrt);

        CharacterWidget *crtItem = createCharacterWidget(newCrt);
        QListWidgetItem *listItem=new QListWidgetItem(characterList);
        characterList->setItemWidget(listItem, crtItem);
        listItem->setSizeHint(QSize(CharacterWidget::maxCrtItemWidth, CharacterWidget::crtItemHeight));

        CharacterPlaceholderWidget *addCharacter = new CharacterPlaceholderWidget;
        QObject::connect(addCharacter, &CharacterPlaceholderWidget::addCharacter, this, &AnimeDetailInfoPage::addCharacter);
        QListWidgetItem *placeholderItem=new QListWidgetItem(characterList);
        characterList->setItemWidget(placeholderItem, addCharacter);
        placeholderItem->setSizeHint(QSize(CharacterWidget::maxCrtItemWidth, CharacterWidget::crtItemHeight));
    }
}

void AnimeDetailInfoPage::modifyCharacter(CharacterWidget *crtItem)
{
    if(!currentAnime) return;
    CharacterEditor editor(currentAnime, &crtItem->crt, this);
    if(QDialog::Accepted == editor.exec())
    {
        Character &srcCrt = crtItem->crt;
        bool modified = editor.name != srcCrt.name || editor.link != srcCrt.link ||
            editor.actor != srcCrt.actor;
        if(modified)
        {
            Character newCrt;
            newCrt.name = editor.name;
            newCrt.link = editor.link;
            newCrt.actor = editor.actor;
            currentAnime->modifyCharacterInfo(srcCrt.name, newCrt);

            srcCrt.name = editor.name;
            srcCrt.link = editor.link;
            srcCrt.actor = editor.actor;
            crtItem->refreshText();
        }
    }
}

void AnimeDetailInfoPage::removeCharacter(CharacterWidget *crtItem)
{
    if(!currentAnime) return;
    currentAnime->removeCharacter(crtItem->crt.name);
    delete characterList->currentItem();
}

CharacterWidget::CharacterWidget(const Character &character, QWidget *parent) : QWidget(parent), crt(character)
{
    QAction *actModify = new QAction(tr("Modify"), this);
    QObject::connect(actModify, &QAction::triggered, this, [=](){
        emit modifyCharacter(this);
    });
    QAction *actRemove = new QAction(tr("Remove"), this);
    QObject::connect(actRemove, &QAction::triggered, this, [=](){
        emit removeCharacter(this);
    });
    setContextMenuPolicy(Qt::ActionsContextMenu);
    addAction(actModify);
    addAction(actRemove);

    iconLabel=new QLabel(this);
    iconLabel->setScaledContents(true);
    iconLabel->setObjectName(QStringLiteral("CrtIcon"));
    iconLabel->setFixedSize(iconSize*logicalDpiX()/96, iconSize*logicalDpiY()/96);
    iconLabel->setAlignment(Qt::AlignCenter);

    QAction *actCopyImage = new QAction(tr("Copy Image"), this);
    QObject::connect(actCopyImage, &QAction::triggered, this, [=](){
        if(crt.image.isNull()) return;
        QApplication::clipboard()->setPixmap(crt.image);
    });
    QAction *actDownloadImage = new QAction(tr("Re-Download Image"), this);
    QObject::connect(actDownloadImage, &QAction::triggered, this, [=](){
        if(!crt.imgURL.isEmpty())
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

    nameLabel=new QLabel(this);
    nameLabel->setOpenExternalLinks(true);

    infoLabel=new QLabel(this);
    infoLabel->setObjectName(QStringLiteral("AnimeDetailCharactorTitle"));

    refreshIcon();
    refreshText();

    QHBoxLayout *itemHLayout=new QHBoxLayout(this);
    itemHLayout->addWidget(iconLabel);
    QVBoxLayout *infoVLayout=new QVBoxLayout();
    infoVLayout->addWidget(nameLabel);
    infoVLayout->addWidget(infoLabel);
    itemHLayout->addLayout(infoVLayout);
    itemHLayout->addStretch(1);
}

void CharacterWidget::refreshIcon(const QByteArray &imgBytes)
{
    if(!imgBytes.isEmpty())
    {
        crt.image.loadFromData(imgBytes);
    }
    if(crt.image.isNull())
    {
        static QPixmap nullIcon(":/res/images/kikoplay-4.png");
        iconLabel->setPixmap(nullIcon);
    }
    else
    {
        iconLabel->setPixmap(crt.image);
    }
}

void CharacterWidget::refreshText()
{
    QFontMetrics fm(nameLabel->fontMetrics());
    nameLabel->setText(QString("<a style='color: rgb(96, 208, 252);' href = \"%1\">%2</a>")
                       .arg(crt.link)
                       .arg(fm.elidedText(crt.name, Qt::ElideRight, 160*logicalDpiX()/96)));
    nameLabel->setToolTip(crt.name);

    infoLabel->setText(fm.elidedText(crt.actor, Qt::ElideRight, 160*logicalDpiX()/96));
    infoLabel->setToolTip(crt.actor);
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

CharacterPlaceholderWidget::CharacterPlaceholderWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("CharacterPlaceholder"));
    QLabel *iconLabel=new QLabel(this);
    GlobalObjects::iconfont->setPointSize(24);
    iconLabel->setFont(*GlobalObjects::iconfont);
    iconLabel->setText(QChar(0xe6f3));
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setObjectName(QStringLiteral("CharacterPlaceholderIcon"));

    QLabel *infoLabel=new QLabel(tr("Add Character"), this);
    infoLabel->setFont(QFont(GlobalObjects::normalFont, 10));
    infoLabel->setObjectName(QStringLiteral("AnimeDetailCharactorTitle"));

    QHBoxLayout *itemHLayout=new QHBoxLayout(this);
    itemHLayout->addWidget(iconLabel);
    itemHLayout->addSpacing(4*logicalDpiX()/96);
    itemHLayout->addWidget(infoLabel);
    itemHLayout->addStretch(1);
}

void CharacterPlaceholderWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        QMargins margins = layout()->contentsMargins();
        margins.setTop(margins.top() + 4);
        layout()->setContentsMargins(margins);
    }
}

void CharacterPlaceholderWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        QMargins margins = layout()->contentsMargins();
        margins.setTop(margins.top() - 4);
        layout()->setContentsMargins(margins);
        emit addCharacter();
    }
}

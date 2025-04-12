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
#include "MediaLibrary/animeitemdelegate.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Common/network.h"
#include "UI/animeepisodeeditor.h"
#include "UI/ela/ElaMenu.h"
#include "UI/inputdialog.h"
#include "UI/widgets/component/flowlayout.h"
#include "UI/widgets/fonticonbutton.h"
#include "captureview.h"
#include "gifcapture.h"
#include "charactereditor.h"
#include "animeinfoeditor.h"
#include "widgets/floatscrollbar.h"
#include "globalobjects.h"

int CharacterWidget::maxCrtItemWidth = 0;
int CharacterWidget::crtItemHeight = 0;

AnimeDetailInfoPage::AnimeDetailInfoPage(QWidget *parent) : QWidget(parent), currentAnime(nullptr)
{
    initPageUI();
}

void AnimeDetailInfoPage::setAnime(Anime *anime)
{
    currentAnime = anime;
    curEpLoaded = false;
    curCaptrueLoaded = false;
    epModel->setAnime(nullptr);
    epDelegate->setAnime(nullptr);
    characterList->clear();
    tagPanel->clear();
    captureModel->setAnimeName("");
    pageSLayout->setCurrentIndex(0);
    static_cast<QScrollArea *>(pageSLayout->widget(0))->verticalScrollBar()->setValue(0);
    if (!currentAnime)
    {
        return;
    }
    updateCover();
    refreshCurInfo();
    setCharacters();
    setTags();
}

bool AnimeDetailInfoPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched==coverLabel)
    {
        if (event->type() == QEvent::MouseButtonDblClick)
        {
            if (currentAnime && !currentAnime->rawCover().isNull())
            {
                CaptureView viewer(currentAnime->rawCover(), this);
                viewer.exec();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched,event);
}

void AnimeDetailInfoPage::initPageUI()
{
    QWidget *titleContainer = new QWidget(this);
    titleLabel = new QLabel(titleContainer);
    titleLabel->setObjectName(QStringLiteral("AnimeDetailTitle"));
    titleLabel->setFont(QFont(GlobalObjects::normalFont, 20));
    titleLabel->setOpenExternalLinks(true);

    GlobalObjects::iconfont->setPointSize(16);
    QPushButton *backButton = new QPushButton(titleContainer);
    backButton->setFont(*GlobalObjects::iconfont);
    backButton->setText(QChar(0xe945));
    backButton->setObjectName(QStringLiteral("AnimeDetailBack"));
    QObject::connect(backButton, &QPushButton::clicked, this, &AnimeDetailInfoPage::back);

    QPushButton *editButton =  new QPushButton(titleContainer);
    editButton->setFont(*GlobalObjects::iconfont);
    editButton->setText(QChar(0xe60a));
    editButton->setObjectName(QStringLiteral("LibraryPageButton"));
    QObject::connect(editButton, &QPushButton::clicked, this, [=](){
        AnimeInfoEditor editor(currentAnime, this);
        if (QDialog::Accepted != editor.exec()) return;
        QVector<std::function<void (QSqlQuery *)>> queries;
        if (editor.airDateChanged)
        {
            queries.append(AnimeWorker::instance()->updateAirDate(currentAnime->name(), editor.airDate(), currentAnime->airDate()));
            currentAnime->setAirDate(editor.airDate());
        }
        if (editor.epsChanged)
        {
            queries.append(AnimeWorker::instance()->updateEpCount(currentAnime->name(), editor.epCount()));
            currentAnime->setEpCount(editor.epCount());
        }
        if (editor.staffChanged)
        {
            queries.append(AnimeWorker::instance()->updateStaffInfo(currentAnime->name(), editor.staffs()));
            currentAnime->setStaffs(editor.staffs());
        }
        if (editor.descChanged)
        {
            queries.append(AnimeWorker::instance()->updateDescription(currentAnime->name(), editor.desc()));
            currentAnime->setDesc(editor.desc());
        }
        if (!queries.isEmpty())
        {
            AnimeWorker::instance()->runQueryGroup(queries);
            refreshCurInfo();
        }
    });

    QHBoxLayout *titleHLayout = new QHBoxLayout(titleContainer);
    titleHLayout->addSpacing(16);
    titleHLayout->addWidget(backButton);
    titleHLayout->addSpacing(16);
    titleHLayout->addWidget(titleLabel);
    titleHLayout->addWidget(editButton);
    titleHLayout->addStretch(1);

    QWidget *btnContainer = new QWidget(this);
    const int btnMinWidth = 46;

    QButtonGroup *btnGroup = new QButtonGroup(btnContainer);
    QPushButton *infoPageBtn = new FontIconButton(QChar(0xec19), "", 14, 14, 10, this);
    infoPageBtn->setObjectName(QStringLiteral("TaskTypeToolButton"));
    infoPageBtn->setContentsMargins(8, 4, 4, 4);
    infoPageBtn->setMinimumWidth(btnMinWidth);
    infoPageBtn->setCheckable(true);
    infoPageBtn->setChecked(true);
    btnGroup->addButton(infoPageBtn, 0);

    QPushButton *episodePageBtn = new FontIconButton(QChar(0xe639), "", 14, 14, 10, this);
    episodePageBtn->setObjectName(QStringLiteral("TaskTypeToolButton"));
    episodePageBtn->setContentsMargins(8, 4, 4, 4);
    episodePageBtn->setMinimumWidth(btnMinWidth);
    episodePageBtn->setCheckable(true);
    btnGroup->addButton(episodePageBtn, 1);

    QPushButton *capturePageBtn = new FontIconButton(QChar(0xe6e6), "", 14, 14, 10, this);
    capturePageBtn->setObjectName(QStringLiteral("TaskTypeToolButton"));
    capturePageBtn->setContentsMargins(8, 4, 4, 4);
    capturePageBtn->setMinimumWidth(btnMinWidth);
    capturePageBtn->setCheckable(true);
    btnGroup->addButton(capturePageBtn, 2);

    QVBoxLayout *btnVLayout = new QVBoxLayout(btnContainer);
    btnVLayout->setSpacing(10);
    btnVLayout->addWidget(infoPageBtn);
    btnVLayout->addWidget(episodePageBtn);
    btnVLayout->addWidget(capturePageBtn);
    btnVLayout->addStretch(1);

    QWidget *pageContainer = new QWidget(this);
    pageSLayout = new QStackedLayout(pageContainer);
    pageSLayout->setContentsMargins(0, 0, 0, 0);

    pageSLayout->addWidget(initInfoPage());
    pageSLayout->addWidget(initEpisodePage());
    pageSLayout->addWidget(initCapturePage());
    QObject::connect(btnGroup, &QButtonGroup::idToggled, [=](int id, bool checked) {
        if (checked)
        {
            pageSLayout->setCurrentIndex(id);
        }
    });
    QObject::connect(pageSLayout, &QStackedLayout::currentChanged, this, [=](int index){
        QAbstractButton *btn = btnGroup->button(index);
        if (btn) btn->setChecked(true);
        if (btn == episodePageBtn && !curEpLoaded)
        {
            curEpLoaded = true;
            epModel->setAnime(currentAnime);
            epDelegate->setAnime(currentAnime);
        }
        else if (btn == capturePageBtn && !curCaptrueLoaded)
        {
            curCaptrueLoaded = true;
            captureModel->setAnimeName(currentAnime->name());
        }
    });

    QGridLayout *animeGLayout = new QGridLayout(this);
    animeGLayout->setContentsMargins(0, 4, 4, 4);
    animeGLayout->addWidget(titleContainer, 0, 0, 1, 2);
    animeGLayout->addWidget(btnContainer, 1, 0, 1, 1);
    animeGLayout->addWidget(pageContainer, 1, 1, 1, 1);
    animeGLayout->setRowStretch(1, 1);
    animeGLayout->setColumnStretch(1, 1);
}

QWidget *AnimeDetailInfoPage::initInfoPage()
{
    QWidget *pageContainer = new QWidget(this);

    coverLabel = new QLabel(pageContainer);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setScaledContents(true);
    coverLabel->installEventFilter(this);
    initCoverActions();

    viewInfoLabel = new QLabel(pageContainer);
    viewInfoLabel->setObjectName(QStringLiteral("AnimeDetailViewInfo"));
    viewInfoLabel->setFont(QFont(GlobalObjects::normalFont, 12));
    viewInfoLabel->setWordWrap(true);
    viewInfoLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    viewInfoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    viewInfoLabel->setOpenExternalLinks(true);

    QScrollArea *viewInfoScrollArea = new QScrollArea(pageContainer);
    viewInfoScrollArea->setWidget(viewInfoLabel);
    viewInfoScrollArea->setWidgetResizable(true);
    new FloatScrollBar(viewInfoScrollArea->verticalScrollBar(), viewInfoScrollArea);

    QHBoxLayout *infoHLayout = new QHBoxLayout();
    infoHLayout->setContentsMargins(0, 0, 16, 0);
    infoHLayout->addWidget(coverLabel);
    infoHLayout->addSpacing(16);
    infoHLayout->addWidget(viewInfoScrollArea);

    QVBoxLayout *pageVLayout = new QVBoxLayout(pageContainer);
    pageVLayout->setContentsMargins(0, 0, 32, 4);
    pageVLayout->addLayout(infoHLayout);
    pageVLayout->addSpacing(8);
    pageVLayout->addWidget(initInfoPageCharcters());
    pageVLayout->addSpacing(8);
    pageVLayout->addWidget(initInfoPageTags());
    pageVLayout->addSpacing(8);
    pageVLayout->addWidget(initInfoPageStaffs());
    pageVLayout->addSpacing(32);
    pageVLayout->addStretch(1);

    QScrollArea *pageScrollArea = new QScrollArea(this);
    pageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pageScrollArea->setWidget(pageContainer);
    pageScrollArea->setWidgetResizable(true);
    new FloatScrollBar(pageScrollArea->verticalScrollBar(), pageScrollArea);
    return pageScrollArea;
}

QWidget *AnimeDetailInfoPage::initEpisodePage()
{
    QWidget *epPageContainer = new QWidget(this);
    QVBoxLayout *epVLayout = new QVBoxLayout(epPageContainer);
    epVLayout->setContentsMargins(0, 0, 16, 16);

    FontIconButton *addButton = new FontIconButton(QChar(0xe667), tr("Add Episode(s)"), 12, 10, 2, epPageContainer);
    addButton->setObjectName(QStringLiteral("FontIconToolButton"));
    QObject::connect(addButton, &QPushButton::clicked, this, [=](){
        const QStringList files = QFileDialog::getOpenFileNames(this, tr("Select media files"), "",
                                    QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
        if (!files.isEmpty())
        {
            for (auto &file : files)
            {
                epModel->addEp(file);
            }
        }
    });

    FontIconButton *clearInvalidButton = new FontIconButton(QChar(0xe637), tr("Delete Invalid Episodes"), 12, 10, 2, epPageContainer);
    clearInvalidButton->setObjectName(QStringLiteral("FontIconToolButton"));

    QObject::connect(clearInvalidButton,  &QPushButton::clicked, this, [=](){
        Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Remove %1 invalid episodes").arg(epModel->removeInvalidEp()),
                                             NotifyMessageFlag::NM_HIDE);
    });

    QHBoxLayout *toolBtnHLayout = new QHBoxLayout;
    toolBtnHLayout->setContentsMargins(0, 0, 0, 0);
    toolBtnHLayout->setSpacing(6);
    toolBtnHLayout->addWidget(addButton);
    toolBtnHLayout->addWidget(clearInvalidButton);
    toolBtnHLayout->addStretch(1);
    epVLayout->addLayout(toolBtnHLayout);

    QTreeView *episodeView = new QTreeView(this);
    // episodeView->setObjectName(QStringLiteral("AnimeDetailEpisode"));
    episodeView->setRootIsDecorated(false);
    episodeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    epDelegate = new EpItemDelegate(this);
    episodeView->setItemDelegate(epDelegate);
    episodeView->setFont(QFont(GlobalObjects::normalFont, 10));
    episodeView->header()->setFont(QFont(GlobalObjects::normalFont, 12));
    epModel = new EpisodesModel(nullptr, this);
    episodeView->setModel(epModel);
    episodeView->setAlternatingRowColors(true);
    new FloatScrollBar(episodeView->verticalScrollBar(), episodeView);

    epVLayout->addWidget(episodeView);

    ElaMenu *epMenu = new ElaMenu(episodeView);

    QAction *deleteAction = epMenu->addAction(tr("Delete"));
    QObject::connect(deleteAction, &QAction::triggered, episodeView, [=](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.empty()) return;
        QModelIndex selectedIndex = selection.last();
        if (selectedIndex.isValid())
        {
            int epCount = epModel->rowCount(QModelIndex());
            QHash<QString, int> pathCounter;
            for (int i = 0; i < epCount; ++i)
            {
                QString file(epModel->data(epModel->index(i, (int)EpisodesModel::Columns::LOCALFILE, QModelIndex()), Qt::DisplayRole).toString());
                ++pathCounter[file.mid(0, file.lastIndexOf('/'))];
            }
            QString localFile = epModel->data(selectedIndex, Qt::DisplayRole).toString();
            QString filePath = localFile.mid(0, localFile.lastIndexOf('/'));
            if (pathCounter.value(filePath, 0) < 2)
            {
                LabelModel::instance()->removeTag(currentAnime->name(), filePath, TagNode::TAG_FILE);
            }
            epModel->removeEp(selection.last());
        }
    });
    QAction *addAction = epMenu->addAction(tr("Add to Playlist"));
    QObject::connect(addAction, &QAction::triggered, episodeView, [=](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.empty()) return;
        QStringList items;
        for (const QModelIndex &index : selection)
        {
            if (index.isValid())
            {
                QString localFile = epModel->data(index, Qt::DisplayRole).toString();
                if (QFile::exists(localFile)) items << localFile;
            }
        }
        Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Add %1 items to Playlist").arg(GlobalObjects::playlist->addItems(items,QModelIndex())),
                                             NotifyMessageFlag::NM_HIDE);
    });
    QAction *playAction = epMenu->addAction(tr("Play"));
    QObject::connect(playAction, &QAction::triggered, episodeView, [=](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.empty()) return;
        QFileInfo info(epModel->data(selection.last(), Qt::DisplayRole).toString());
        if (!info.exists())
        {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("File Not Exist"), NotifyMessageFlag::NM_ERROR|NotifyMessageFlag::NM_HIDE);
            return;
        }
        emit playFile(info.absoluteFilePath());
    });
    QAction *explorerViewAction = epMenu->addAction(tr("Browse File"));
    QObject::connect(explorerViewAction, &QAction::triggered, episodeView, [=](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.empty()) return;
        QFileInfo info(epModel->data(selection.last(), Qt::DisplayRole).toString());
        if (!info.exists())
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

    episodeView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(episodeView, &QTreeView::customContextMenuRequested, this, [=](){
        auto selection = episodeView->selectionModel()->selectedRows((int)EpisodesModel::Columns::LOCALFILE);
        if (selection.empty()) return;
        epMenu->exec(QCursor::pos());
    });

    QObject::connect(episodeView, &QTreeView::doubleClicked, this, [=](const QModelIndex &index){
        if (index.column() == static_cast<int>(EpisodesModel::Columns::TITLE) && currentAnime)
        {
            AnimeEpisodeEditor epEditor(currentAnime, index.data(EpisodesModel::EpRole).value<EpInfo>(), this);
            if (QDialog::Accepted == epEditor.exec() && epEditor.epChanged)
            {
                epModel->setData(index, QVariant::fromValue<EpInfo>(epEditor.curEp), Qt::EditRole);
            }
        }
    });

    QHeaderView *epHeader = episodeView->header();
    // epHeader->setObjectName(QStringLiteral("AnimeDetailEpisodeHeader"));
    epHeader->resizeSection(0, 300);
    epHeader->resizeSection(1, 160);
    epHeader->resizeSection(2, 160);

    return epPageContainer;
}

QWidget *AnimeDetailInfoPage::initCapturePage()
{
    QWidget *capturePageContainer = new QWidget(this);
    QVBoxLayout *captureVLayout = new QVBoxLayout(capturePageContainer);

    captureModel = new CaptureListModel("", this);
    QObject::connect(captureModel,&CaptureListModel::fetching,this,[](bool on){
        if (on)  Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Loading..."), NotifyMessageFlag::NM_PROCESS);
        else Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Loading..."), NotifyMessageFlag::NM_HIDE);
    });

    QListView *captureView = new QListView(this);
    captureView->setObjectName(QStringLiteral("captureView"));
    captureView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    captureView->setIconSize(QSize(200, 112));
    captureView->setViewMode(QListView::IconMode);
    captureView->setUniformItemSizes(true);
    captureView->setResizeMode(QListView::Adjust);
    captureView->setMovement(QListView::Static);
    captureView->setWordWrap(true);
    captureView->setEditTriggers(QAbstractItemView::SelectedClicked);
    captureView->setModel(captureModel);
    new FloatScrollBar(captureView->verticalScrollBar(), captureView);

    captureVLayout->addWidget(captureView);

    QAction* actListMode = new QAction(tr("List Mode"),this);
    actListMode->setCheckable(true);
    QObject::connect(actListMode, &QAction::triggered, captureView, [=](bool){
        captureView->setViewMode(QListView::ListMode);
    });
    QAction* actIconMode = new QAction(tr("Icon Mode"));
    actIconMode->setCheckable(true);
    QObject::connect(actIconMode, &QAction::triggered, captureView, [=](bool){
        captureView->setViewMode(QListView::IconMode);
    });
    QAction* actCopy = new QAction(tr("Copy"), this);
    QObject::connect(actCopy, &QAction::triggered, captureView, [=](bool){
        auto selection = captureView->selectionModel()->selectedRows();
        if (selection.empty()) return;
        const AnimeImage *item = captureModel->getCaptureItem(selection.first().row());
        if (item->type == AnimeImage::CAPTURE)
        {
         QPixmap img(captureModel->getFullCapture(selection.first().row()));
         QApplication::clipboard()->setPixmap(img);
        }
    });
    QAction* actSave = new QAction(tr("Save"), this);
    QObject::connect(actSave, &QAction::triggered, captureView, [=](bool){
        auto selection = captureView->selectionModel()->selectedRows();
        if (selection.empty()) return;
        const AnimeImage *item = captureModel->getCaptureItem(selection.first().row());
        if (item->type == AnimeImage::CAPTURE)
        {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save Capture"), item->info, "JPEG Images (*.jpg);;PNG Images (*.png)");
            if(!fileName.isEmpty())
            {
             QPixmap img(captureModel->getFullCapture(selection.first().row()));
             img.save(fileName);
            }
        }
        else if (item->type == AnimeImage::SNIPPET)
        {
            QString snippetFilePath(captureModel->getSnippetFile(selection.first().row()));
            if (snippetFilePath.isEmpty())
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
    QAction *actSaveGIF = new QAction(tr("Save As GIF"), this);
    QObject::connect(actSaveGIF, &QAction::triggered, captureView, [=](bool){
        auto selection = captureView->selectionModel()->selectedRows();
        if (selection.empty()) return;
        const AnimeImage *item=captureModel->getCaptureItem(selection.first().row());
        if (item->type != AnimeImage::SNIPPET) return;


        QString snippetFilePath(captureModel->getSnippetFile(selection.first().row()));
        if (snippetFilePath.isEmpty())
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
    QAction* actAdd = new QAction(tr("Add"), this);
    QObject::connect(actAdd, &QAction::triggered, captureView, [=](bool){
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Image"), "", "JPEG Images (*.jpg);;PNG Images (*.png)");
        if (!fileName.isEmpty())
        {
            AnimeWorker::instance()->saveCapture(currentAnime->name(), QFileInfo(fileName).fileName(), QImage(fileName));
        }
    });
    QAction* actRemove = new QAction(tr("Remove"), this);
    QObject::connect(actRemove, &QAction::triggered, captureView, [=](bool){
        auto selection = captureView->selectionModel()->selectedRows();
        if(selection.empty()) return;
        captureModel->deleteRow(selection.first().row());
    });
    QAction *explorerViewAction = new QAction(tr("Browse File"), this);
    QObject::connect(explorerViewAction, &QAction::triggered, captureView, [=](){
        auto selection = captureView->selectionModel()->selectedRows();
        if (selection.size()==0) return;
        QString snippetFilePath(captureModel->getSnippetFile(selection.first().row()));
        if (!snippetFilePath.isEmpty())
        {
            QFileInfo snippetFile(snippetFilePath);
#ifdef Q_OS_WIN
            QProcess::startDetached("Explorer", {"/select,", QDir::toNativeSeparators(snippetFile.absoluteFilePath())});
#else
            QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(snippetFile.absolutePath())));
#endif
        }
    });

    QMenu *contexMenu = new ElaMenu(captureView);
    QActionGroup *group = new QActionGroup(this);
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
    QObject::connect(captureView, &QListView::customContextMenuRequested, captureView, [=](){
        auto selection = captureView->selectionModel()->selectedRows();
        if (selection.empty())
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

    return capturePageContainer;
}

QWidget *AnimeDetailInfoPage::initInfoPageCharcters()
{
    QWidget *crtContainer = new QWidget(this);
    QVBoxLayout *crtVLayout = new QVBoxLayout(crtContainer);
    crtVLayout->setContentsMargins(0, 0, 16, 0);

    QLabel *crtTip = new QLabel(tr("Charcters"), crtContainer);
    crtTip->setFont(QFont(GlobalObjects::normalFont, 16));
    crtTip->setObjectName(QStringLiteral("AnimeDetailInfoTitle"));

    characterList = new QListWidget(crtContainer);
    characterList->setContentsMargins(12, 8, 0, 8);
    characterList->setObjectName(QStringLiteral("AnimeDetailCharacterList"));
    characterList->setViewMode(QListView::IconMode);
    characterList->setResizeMode(QListView::Adjust);
    characterList->setMovement(QListView::Static);
    characterList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    characterList->setWrapping(false);
    characterList->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    characterList->setMaximumHeight(104);
    new FloatScrollBar(characterList->horizontalScrollBar(), characterList);

    crtVLayout->addWidget(crtTip);
    crtVLayout->addSpacing(4);
    crtVLayout->addWidget(characterList);

    return crtContainer;
}

QWidget *AnimeDetailInfoPage::initInfoPageTags()
{
    QWidget *tagContainer = new QWidget(this);
    QVBoxLayout *infoVLayout = new QVBoxLayout(tagContainer);
    infoVLayout->setContentsMargins(0, 0, 16, 0);

    QLabel *tagTip = new QLabel(tr("Tag"), tagContainer);
    tagTip->setFont(QFont(GlobalObjects::normalFont, 16));
    tagTip->setObjectName(QStringLiteral("AnimeDetailInfoTitle"));

    QWidget *container = new QWidget(tagContainer);
    container->setObjectName(QStringLiteral("AnimeDetailTagList"));
    container->setMinimumHeight(80);
    tagContainerSLayout = new QStackedLayout(container);
    tagContainerSLayout->setContentsMargins(0,0,0,0);

    infoVLayout->addWidget(tagTip);
    infoVLayout->addSpacing(4);
    infoVLayout->addWidget(container);

    tagPanel = new TagPanel(container, true, false, true);
    tagPanel->setSizePolicy(QSizePolicy::Policy::Ignored, QSizePolicy::Ignored);
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
    webTagPanel->setSizePolicy(QSizePolicy::Policy::Ignored, QSizePolicy::Ignored);
    tagContainerSLayout->addWidget(tagPanel);
    tagContainerSLayout->addWidget(webTagPanel);

    QAction *webTagAction=new QAction(tr("Tags on Web"), this);
    tagPanel->addAction(webTagAction);
    QObject::connect(webTagAction, &QAction::triggered, this, [=](){
        if (currentAnime->scriptId().isEmpty()) return;
        QStringList tags;
        webTagAction->setEnabled(false);
        Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Tags..."), NotifyMessageFlag::NM_DARKNESS_BACK | NotifyMessageFlag::NM_PROCESS);
        ScriptState state = GlobalObjects::animeProvider->getTags(currentAnime, tags);
        if (state)
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

    QAction *webTagOKAction = new QAction(tr("Add Selected Tags"), this);
    QObject::connect(webTagOKAction, &QAction::triggered, this, [=](){
        QStringList tags(webTagPanel->getSelectedTags());
        if (tags.empty()) return;
        LabelModel::instance()->addCustomTags(currentAnime->name(), tags);
        tagPanel->clear();
        auto &tagMap=LabelModel::instance()->customTags();
        tags.clear();
        for (auto iter = tagMap.begin(); iter != tagMap.end(); ++iter)
        {
            if (iter.value().contains(currentAnime->name()))
            {
                tags.append(iter.key());
            }
        }
        tagPanel->addTag(tags);
        tagContainerSLayout->setCurrentIndex(0);
    });
    QAction *webTagCancelAction=new QAction(tr("Cancel"), this);
    QObject::connect(webTagCancelAction,&QAction::triggered, this, [=](){
        tagContainerSLayout->setCurrentIndex(0);
    });
    webTagPanel->addAction(webTagOKAction);
    webTagPanel->addAction(webTagCancelAction);

    return tagContainer;
}

QWidget *AnimeDetailInfoPage::initInfoPageStaffs()
{
    QWidget *staffContainer = new QWidget(this);
    QVBoxLayout *infoVLayout = new QVBoxLayout(staffContainer);
    infoVLayout->setContentsMargins(0, 0, 16, 0);

    QLabel *staffTip = new QLabel(tr("Staff"), staffContainer);
    staffTip->setFont(QFont(GlobalObjects::normalFont, 16));
    staffTip->setObjectName(QStringLiteral("AnimeDetailInfoTitle"));

    staffInfoLabel = new QLabel(staffContainer);
    staffInfoLabel->setObjectName(QStringLiteral("AnimeDetailStaffInfo"));
    staffInfoLabel->setFont(QFont(GlobalObjects::normalFont, 12));
    staffInfoLabel->setWordWrap(true);
    staffInfoLabel->setOpenExternalLinks(true);
    staffInfoLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    staffInfoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    staffInfoLabel->setContentsMargins(10, 8, 8, 8);


    infoVLayout->addWidget(staffTip);
    infoVLayout->addSpacing(4);
    infoVLayout->addWidget(staffInfoLabel);

    return staffContainer;
}

void AnimeDetailInfoPage::initCoverActions()
{
    ElaMenu *coverMenu = new ElaMenu(coverLabel);
    QAction *actCopyCover = coverMenu->addAction(tr("Copy Cover"));
    QObject::connect(actCopyCover, &QAction::triggered, this, [=](){
        if(currentAnime && !currentAnime->rawCover().isNull())
        {
            QApplication::clipboard()->setPixmap(currentAnime->rawCover());
        }
    });
    QAction *actPasteCover = coverMenu->addAction(tr("Paste Cover"));
    QObject::connect(actPasteCover, &QAction::triggered, this, [=](){
        QImage img = QApplication::clipboard()->image();
        if (img.isNull()) return;
        CaptureView viewer(QPixmap::fromImage(img), this);
        if (QDialog::Accepted == viewer.exec())
        {
            QByteArray imgBytes;
            QBuffer bufferImage(&imgBytes);
            bufferImage.open(QIODevice::WriteOnly);
            img.save(&bufferImage, "JPG");
            currentAnime->setCover(imgBytes);
            updateCover();
        }
    });
    QAction *actCleanCover = coverMenu->addAction(tr("Clean Cover"));
    QObject::connect(actCleanCover, &QAction::triggered, this, [=](){
        currentAnime->setCover(QByteArray());
        updateCover();
    });
    QAction *actDownloadCover = coverMenu->addAction(tr("Re-Download Cover"));
    QObject::connect(actDownloadCover, &QAction::triggered, this, [=](){
        if (currentAnime && !currentAnime->coverURL().isEmpty())
        {
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Cover Image..."), NM_PROCESS | NM_DARKNESS_BACK);
            Network::Reply reply(Network::httpGet(currentAnime->coverURL(), QUrlQuery()));
            if (reply.hasError)
            {
                Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, reply.errInfo, NM_HIDE | NM_ERROR);
                return;
            }
            currentAnime->setCover(reply.content);
            updateCover();
            Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching Down"), NM_HIDE);
        }
    });
    QAction *actLocalCover = coverMenu->addAction(tr("Select From File"));
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
                updateCover();
            }
        }
    });

    coverLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(coverLabel, &QLabel::customContextMenuRequested, this, [=](const QPoint &pos){
       coverMenu->popup(coverLabel->mapToGlobal(pos));
    });
}

void AnimeDetailInfoPage::updateCover()
{
    static QPixmap nullCover(":/res/images/null-cover.png");

    const int pxR = devicePixelRatio();
    const int w = AnimeItemDelegate::CoverWidth * 1.3;
    const int h = AnimeItemDelegate::CoverHeight * 1.3;
    QPixmap dest(w*pxR, h*pxR);
    dest.setDevicePixelRatio(pxR);
    dest.fill(Qt::transparent);
    QPainter painter(&dest);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
    QPainterPath path;
    path.addRoundedRect(0, 0, w, h, 8, 8);
    painter.setClipPath(path);
    painter.drawPixmap(QRect(0, 0, w, h), currentAnime->rawCover().isNull() ? nullCover : currentAnime->rawCover());

    coverLabel->setPixmap(dest);
}

void AnimeDetailInfoPage::refreshCurInfo()
{
    titleLabel->setText(QString("<a style='color: white; text-decoration: none;' href = \"%1\">%2</a>")
                            .arg(currentAnime->url(), currentAnime->name()));

    const QString itemTpl = "<p><font style='color: #d0d0d0;'>%1</font>%2</p>";
    QString displayDesc = currentAnime->description();
    displayDesc.replace("\n", "<br/>");
    QStringList animeInfoList{
        itemTpl.arg(tr("Adding Time: "), currentAnime->addTimeStr()),
        itemTpl.arg(tr("Air Date: "), currentAnime->airDate()),
        itemTpl.arg(tr("Episode Count: ")).arg(currentAnime->epCount()),
        QString("<div>%1</div>").arg(displayDesc),
    };
    viewInfoLabel->setText(animeInfoList.join("\n"));
    viewInfoLabel->adjustSize();

    QStringList stafflist;
    static QRegularExpression urlRe("^https?://[^\\s/$.?#].[^\\s]*$");
    for (const auto &p: currentAnime->staffList())
    {
        auto match = urlRe.match(p.second);
        bool isURL = match.hasMatch() && match.capturedLength() == p.second.length();
        stafflist.append(QString("<p style='color: rgb(240,240,240);'><font style='color: #d0d0d0;'>%1:</font> %2</p>").
                         arg(p.first, isURL ? QString("<a style='color: rgb(96, 208, 252);' href = \"%1\">%2</a>").arg(p.second, p.second) : p.second));
    }
    staffInfoLabel->clear();
    staffInfoLabel->adjustSize();
    staffInfoLabel->setText(stafflist.join("\n"));
    staffInfoLabel->adjustSize();
}

CharacterWidget *AnimeDetailInfoPage::createCharacterWidget(const Character &crt)
{
    CharacterWidget *crtItem=new CharacterWidget(crt);
    QObject::connect(crtItem, &CharacterWidget::updateCharacter, this, &AnimeDetailInfoPage::updateCharacter);
    QObject::connect(crtItem, &CharacterWidget::selectLocalImage, this, &AnimeDetailInfoPage::selectLocalCharacterImage);
    QObject::connect(crtItem, &CharacterWidget::pasteImage, this, &AnimeDetailInfoPage::pasteCharacterImage);
    QObject::connect(crtItem, &CharacterWidget::cleanImage, this, &AnimeDetailInfoPage::cleanCharacterImage);
    QObject::connect(crtItem, &CharacterWidget::modifyCharacter, this, &AnimeDetailInfoPage::modifyCharacter);
    QObject::connect(crtItem, &CharacterWidget::removeCharacter, this, &AnimeDetailInfoPage::removeCharacter);
    return crtItem;
}

void AnimeDetailInfoPage::setCharacters()
{
    CharacterPlaceholderWidget *addCharacter = new CharacterPlaceholderWidget;
    QObject::connect(addCharacter, &CharacterPlaceholderWidget::addCharacter, this, &AnimeDetailInfoPage::addCharacter);
    CharacterWidget::maxCrtItemWidth = addCharacter->layout()->sizeHint().width();
    CharacterWidget::crtItemHeight = 0;
    QVector<CharacterWidget *> crtWidgets;
    for (auto &character : currentAnime->crList(true))
    {
        CharacterWidget *crtItem = createCharacterWidget(character);
        CharacterWidget::maxCrtItemWidth = qMax(CharacterWidget::maxCrtItemWidth, crtItem->layout()->sizeHint().width());
        CharacterWidget::crtItemHeight = crtItem->layout()->sizeHint().height();
        crtWidgets.append(crtItem);
    }

    CharacterWidget::maxCrtItemWidth = qMin(CharacterWidget::maxCrtItemWidth, CharacterWidget::minCrtItemWidth);
    CharacterWidget::crtItemHeight = qMax(CharacterWidget::crtItemHeight, CharacterWidget::minCrtItemHeight);

    for (auto crtItem : crtWidgets)
    {
        QListWidgetItem *listItem = new QListWidgetItem(characterList);
        characterList->setItemWidget(listItem, crtItem);
        int maxW = qMin(crtItem->layout()->sizeHint().width(), CharacterWidget::minCrtItemWidth);
        listItem->setSizeHint(QSize(maxW, CharacterWidget::crtItemHeight));
    }

    QListWidgetItem *placeholderItem = new QListWidgetItem(characterList);
    characterList->setItemWidget(placeholderItem, addCharacter);
    placeholderItem->setSizeHint(QSize(addCharacter->layout()->sizeHint().width(), CharacterWidget::crtItemHeight));
}

void AnimeDetailInfoPage::setTags()
{
    auto &tagMap=LabelModel::instance()->customTags();
    QStringList tags;
    for (auto iter=tagMap.begin(); iter!=tagMap.end(); ++iter)
    {
        if (iter.value().contains(currentAnime->name()))
            tags<<iter.key();
    }
    int lastLeafTagPos = -1;
    for (int i = 0; i < tags.size(); ++i)
    {
        if (!tags[i].contains('/'))
        {
            ++lastLeafTagPos;
            std::swap(tags[i], tags[lastLeafTagPos]);
        }
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

void AnimeDetailInfoPage::pasteCharacterImage(CharacterWidget *crtItem)
{
    QImage img = QApplication::clipboard()->image();
    if(img.isNull()) return;
    CaptureView viewer(QPixmap::fromImage(img), this);
    if(QDialog::Accepted == viewer.exec())
    {
        QByteArray imgBytes;
        QBuffer bufferImage(&imgBytes);
        bufferImage.open(QIODevice::WriteOnly);
        img.save(&bufferImage, "JPG");
        currentAnime->setCrtImage(crtItem->crt.name, imgBytes);
        crtItem->refreshIcon(imgBytes);
    }
}

void AnimeDetailInfoPage::cleanCharacterImage(CharacterWidget *crtItem)
{
    currentAnime->setCrtImage(crtItem->crt.name, QByteArray());
    crtItem->crt.image = QPixmap();
    crtItem->refreshIcon(QByteArray());
}

void AnimeDetailInfoPage::addCharacter()
{
    if(!currentAnime) return;
    CharacterEditor editor(currentAnime, nullptr, this);
    if (QDialog::Accepted == editor.exec())
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
        int maxW = qMin(crtItem->layout()->sizeHint().width(), CharacterWidget::minCrtItemWidth);
        listItem->setSizeHint(QSize(maxW, CharacterWidget::crtItemHeight));

        CharacterPlaceholderWidget *addCharacter = new CharacterPlaceholderWidget;
        QObject::connect(addCharacter, &CharacterPlaceholderWidget::addCharacter, this, &AnimeDetailInfoPage::addCharacter);
        QListWidgetItem *placeholderItem=new QListWidgetItem(characterList);
        characterList->setItemWidget(placeholderItem, addCharacter);
        placeholderItem->setSizeHint(QSize(addCharacter->layout()->sizeHint().width(), CharacterWidget::crtItemHeight));
    }
}

void AnimeDetailInfoPage::modifyCharacter(CharacterWidget *crtItem)
{
    if (!currentAnime) return;
    CharacterEditor editor(currentAnime, &crtItem->crt, this);
    if (QDialog::Accepted == editor.exec())
    {
        Character &srcCrt = crtItem->crt;
        bool modified = editor.name != srcCrt.name || editor.link != srcCrt.link ||
            editor.actor != srcCrt.actor;
        if (modified)
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
    setContextMenuPolicy(Qt::CustomContextMenu);
    ElaMenu* ctxMenu = new ElaMenu(this);
    ctxMenu->addAction(actModify);
    ctxMenu->addAction(actRemove);
    QObject::connect(this, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos){
        ctxMenu->popup(this->mapToGlobal(pos));
    });

    iconLabel = new QLabel(this);
    iconLabel->setScaledContents(true);
    iconLabel->setObjectName(QStringLiteral("CrtIcon"));
    iconLabel->setFixedSize(iconSize, iconSize);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->installEventFilter(this);

    QAction *actCopyImage = new QAction(tr("Copy Image"), this);
    QObject::connect(actCopyImage, &QAction::triggered, this, [=](){
        if(crt.image.isNull()) return;
        QApplication::clipboard()->setPixmap(crt.image);
    });
    QAction *actPasteImage = new QAction(tr("Paste Image"), this);
    QObject::connect(actPasteImage, &QAction::triggered, this, [=](){
        emit pasteImage(this);
    });
    QAction *actCleanImage = new QAction(tr("Clean Image"), this);
    QObject::connect(actCleanImage, &QAction::triggered, this, [=](){
        if(crt.image.isNull()) return;
        emit cleanImage(this);
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

    ElaMenu* iconMenu = new ElaMenu(iconLabel);
    iconMenu->addAction(actCopyImage);
    iconMenu->addAction(actPasteImage);
    iconMenu->addAction(actCleanImage);
    iconMenu->addAction(actDownloadImage);
    iconMenu->addAction(actLocalImage);
    iconLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(iconLabel, &QLabel::customContextMenuRequested, iconLabel, [=](const QPoint &pos){
        iconMenu->popup(iconLabel->mapToGlobal(pos));
    });

    nameLabel = new QLabel(this);
    nameLabel->setFont(QFont(GlobalObjects::normalFont, 13));
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

void CharacterWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    emit modifyCharacter(this);
}

bool CharacterWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == iconLabel && event->type() == QEvent::MouseButtonDblClick)
    {
        if (!crt.image.isNull())
        {
            CaptureView viewer(crt.image, this);
            viewer.exec();
        }
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

TagPanel::TagPanel(QWidget *parent, bool allowDelete, bool checkAble, bool allowAdd):QWidget(parent),
    currentTagButton(nullptr), allowCheck(checkAble)
{
    setLayout(new FlowLayout(this));
    setContextMenuPolicy(Qt::CustomContextMenu);

    QAction *actAddTag = new QAction(tr("Add"),this);
    QObject::connect(actAddTag, &QAction::triggered, this, [=](){
        LineInputDialog input(tr("Add Tag"), tr("Tag"), "", "DialogSize/AddTag", false, this);
        if (QDialog::Accepted == input.exec())
        {
            const QString tag = input.text.trimmed();
            if (tagList.contains(tag) || tag.isEmpty()) return;
            emit tagAdded(tag);
        }
    });

    addAction(actAddTag);
    actAddTag->setEnabled(allowAdd);

    QObject::connect(this, &TagPanel::customContextMenuRequested, this, [=](const QPoint &pos){
        ElaMenu* menu = new ElaMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->addActions(this->actions());
        if (!allowAdd) menu->removeAction(actAddTag);
        menu->popup(this->mapToGlobal(pos));
    });

    tagContextMenu = new ElaMenu(this);
    QAction *actCopy=new QAction(tr("Copy"),this);
    tagContextMenu->addAction(actCopy);
    QObject::connect(actCopy, &QAction::triggered, this, [=](){
        if (!currentTagButton) return;
        QApplication::clipboard()->setText(currentTagButton->text());
    });
    QAction *actRemoveTag=new QAction(tr("Delete"),this);
    tagContextMenu->addAction(actRemoveTag);
    QObject::connect(actRemoveTag, &QAction::triggered, this, [=](){
        if (!currentTagButton) return;
        emit deleteTag(currentTagButton->text());
        tagList.remove(currentTagButton->text());
        currentTagButton->deleteLater();
        currentTagButton = nullptr;
    });
    actRemoveTag->setEnabled(allowDelete);

}

void TagPanel::addTag(const QStringList &tags)
{
    for (const QString &tag : tags)
    {
        if (tagList.contains(tag)) continue;
        QPushButton *tagButton=new QPushButton(tag,this);
        tagButton->setObjectName(QStringLiteral("TagButton"));
        tagButton->setCheckable(allowCheck);
        tagButton->setFont(QFont(GlobalObjects::normalFont, 12));
        tagButton->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(tagButton, &QPushButton::customContextMenuRequested, this, [=](const QPoint &pos){
            currentTagButton = tagButton;
            tagContextMenu->exec(tagButton->mapToGlobal(pos));
        });
        layout()->addWidget(tagButton);
        tagList.insert(tag,tagButton);
    }
    updateGeometry();
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

QSize TagPanel::sizeHint() const
{
    return layout()->sizeHint();
}

CharacterPlaceholderWidget::CharacterPlaceholderWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("CharacterPlaceholder"));
    QLabel *iconLabel=new QLabel(this);
    GlobalObjects::iconfont->setPointSize(24);
    iconLabel->setFont(*GlobalObjects::iconfont);
    iconLabel->setText(QChar(0xe6f3));
    iconLabel->setAlignment(Qt::AlignCenter);

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

#include "list.h"
#include <QHeaderView>
#include <QMenu>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QApplication>
#include <QMessageBox>

#include "globalobjects.h"
#include "pooleditor.h"
#include "adddanmu.h"
#include "matcheditor.h"
#include "blockeditor.h"
#include "inputdialog.h"
#include "Play/Danmu/Provider/localprovider.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/blocker.h"
#include "Play/Danmu/Render/danmurender.h"
#include "Play/Danmu/providermanager.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Download/downloadmodel.h"
#define BgmCollectionRole Qt::UserRole+1
#define FolderCollectionRole Qt::UserRole+2
namespace
{
    class TextColorDelegate: public QStyledItemDelegate
    {
    public:
        explicit TextColorDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent)
        { }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            QStyleOptionViewItem ViewOption(option);
            QColor itemForegroundColor = index.data(Qt::ForegroundRole).value<QBrush>().color();
            if (itemForegroundColor.isValid())
            {
                if (itemForegroundColor != option.palette.color(QPalette::WindowText))
                    ViewOption.palette.setColor(QPalette::HighlightedText, itemForegroundColor);

            }
            QStyledItemDelegate::paint(painter, ViewOption, index);

            static QIcon bgmCollectionIcon(":/res/images/bgm-collection.svg");
            static QIcon folderIcon(":/res/images/folder.svg");

            QIcon *marker(nullptr);
            if(index.data(BgmCollectionRole).toBool())
            {
                marker = &bgmCollectionIcon;

            }
            else if(index.data(FolderCollectionRole).toBool())
            {
                marker = &folderIcon;
            }
            if(marker)
            {
                QRect decoration = option.rect;
                decoration.setHeight(decoration.height()-10);
                decoration.setWidth(decoration.height());
                decoration.moveCenter(option.rect.center());
                decoration.moveRight(option.rect.width()+option.rect.x()-10);
                marker->paint(painter, decoration);
            }
        }
    };
    class InfoTip : public QWidget
    {
    public:
        explicit InfoTip(QWidget *parent=nullptr):QWidget(parent)
        {
            setObjectName(QStringLiteral("ListInfoBar"));
            infoIcon=new QMovie(this);
            QLabel *movieLabel=new QLabel(this);
            movieLabel->setMovie(infoIcon);
            movieLabel->setFixedSize(32, 32);
            //movieLabel->setScaledContents(true);
            infoText=new QLabel(this);
            infoText->setObjectName(QStringLiteral("labelListInfo"));
            infoText->setFont(QFont("Microsoft YaHei UI",10));
            infoText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
            cancelBtn = new QPushButton(QObject::tr("Cancel"), this);
            QHBoxLayout *infoBarHLayout=new QHBoxLayout(this);
            infoBarHLayout->setContentsMargins(5,0,5,0);
            infoBarHLayout->setSpacing(4);
            infoBarHLayout->addWidget(movieLabel);
            infoBarHLayout->addWidget(infoText);
            infoBarHLayout->addWidget(cancelBtn);
            cancelBtn->hide();
            ListWindow *list = static_cast<ListWindow *>(parent);
            QObject::connect(cancelBtn, &QPushButton::clicked, list, &ListWindow::infoCancelClicked);
            QObject::connect(&hideTimer,&QTimer::timeout,[this](){
                infoText->setText("");
                hide();
            });
        }
        void showMessage(const QString &msg,int flag)
        {
            if(hideTimer.isActive())
                hideTimer.stop();
            if(flag&PopMessageFlag::PM_HIDE)
            {
                hideTimer.setSingleShot(true);
                hideTimer.start(3000);
            }
            QString icon;
            if(flag&PopMessageFlag::PM_INFO)
                icon=":/res/images/info.png";
            else if(flag&PopMessageFlag::PM_OK)
                icon=":/res/images/ok.png";
            else if(flag&PopMessageFlag::PM_PROCESS)
                icon=":/res/images/loading-rolling.gif";
            if(flag&PopMessageFlag::PM_SHOWCANCEL) cancelBtn->show();
            else cancelBtn->hide();
            if(icon!=infoIcon->fileName())
            {
                infoIcon->stop();
                infoIcon->setFileName(icon);
            }
            infoText->setText(msg);
            infoIcon->start();
        }
    private:
        QMovie *infoIcon;
        QLabel *infoText;
        QTimer hideTimer;
        QPushButton *cancelBtn;
    };
    static QCollator comparer;
}
ListWindow::ListWindow(QWidget *parent) : QWidget(parent),actionDisable(false),matchStatus(0)
{
    Notifier::getNotifier()->addNotify(Notifier::LIST_NOTIFY, this);
    comparer.setNumericMode(true);
    initActions();

    QVBoxLayout *listVLayout=new QVBoxLayout(this);
    listVLayout->setContentsMargins(0,0,0,0);
    listVLayout->setSpacing(0);

    QFont normalFont("Microsoft YaHei UI",11);
    QHBoxLayout *pageButtonHLayout=new QHBoxLayout();
    pageButtonHLayout->setContentsMargins(0,0,0,0);
    pageButtonHLayout->setSpacing(0);
    playlistPageButton = new QToolButton(this);
    playlistPageButton->setFont(normalFont);
    playlistPageButton->setText(tr("MediaList"));
    playlistPageButton->setObjectName(QStringLiteral("ListPageButton"));
    playlistPageButton->setCheckable(true);
    playlistPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    playlistPageButton->setFixedHeight(30*logicalDpiY()/96);
    playlistPageButton->setMinimumWidth(80*logicalDpiX()/96);
    playlistPageButton->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed);
    playlistPageButton->setChecked(true);
    QObject::connect(playlistPageButton,&QToolButton::clicked,[this](){
        playlistPageButton->setChecked(true);
        danmulistPageButton->setChecked(false);
        contentStackLayout->setCurrentIndex(0);
        infoTip->raise();
    });

    danmulistPageButton = new QToolButton(this);
    danmulistPageButton->setFont(normalFont);
    danmulistPageButton->setText(tr("DanmuList"));
    danmulistPageButton->setObjectName(QStringLiteral("ListPageButton"));
    danmulistPageButton->setCheckable(true);
    danmulistPageButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    danmulistPageButton->setFixedHeight(30*logicalDpiY()/96);
    danmulistPageButton->setMinimumWidth(80*logicalDpiX()/96);
    danmulistPageButton->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed);
    QObject::connect(danmulistPageButton,&QToolButton::clicked,[this](){
        playlistPageButton->setChecked(false);
        danmulistPageButton->setChecked(true);
        contentStackLayout->setCurrentIndex(1);
        infoTip->raise();
    });

    pageButtonHLayout->addWidget(playlistPageButton);
    pageButtonHLayout->addWidget(danmulistPageButton);
    listVLayout->addLayout(pageButtonHLayout);

    contentStackLayout=new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(setupPlaylistPage());
    contentStackLayout->addWidget(setupDanmulistPage());
    listVLayout->addLayout(contentStackLayout);

    infoTip=new InfoTip(this);
    infoTip->hide();
    infoTip->setWindowFlag(Qt::WindowStaysOnTopHint);
    //QObject::connect(GlobalObjects::playlist,&PlayList::message,this,&ListWindow::showMessage);

    QObject::connect(GlobalObjects::playlist,&PlayList::matchStatusChanged, this, [this](bool on){
        matchStatus+=(on?1:-1);
        if(matchStatus==0)
        {
            actionDisable=false;
            updatePlaylistActions();
            playlistView->setDragEnabled(true);
            act_addCollection->setEnabled(true);
            act_addFolder->setEnabled(true);
            act_addItem->setEnabled(true);
        }
        else
        {
            actionDisable=true;
            updatePlaylistActions();
            act_addCollection->setEnabled(false);
            act_addFolder->setEnabled(false);
            act_addItem->setEnabled(false);
            playlistView->setDragEnabled(false);
        }
    });

    updatePlaylistActions();
	updateDanmuActions();
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
}

void ListWindow::initActions()
{
    act_play = new QAction(tr("Play"),this);
    QObject::connect(act_play,&QAction::triggered,[this](){
        QModelIndexList selection = playlistView->selectionModel()->selectedIndexes();
        QModelIndex index = selection.size() == 0 ? QModelIndex() : selection.last();
        playItem(index);
    });
    act_autoAssociate=new QAction(tr("Associate Danmu Pool"),this);
    QObject::connect(act_autoAssociate,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndexList indexes(selection.indexes());

        if(indexes.size()==1)
        {    
            const PlayListItem *item=GlobalObjects::playlist->getItem(indexes.first());
            if(!item->poolID.isEmpty())
            {
                QList<const PlayListItem *> &&siblings=GlobalObjects::playlist->getSiblings(item);
                MatchEditor matchEditor(GlobalObjects::playlist->getItem(indexes.first()),&siblings,this);
                if(QDialog::Accepted==matchEditor.exec())
                {
                    if(matchEditor.batchAnime.isEmpty())
                    {
                        GlobalObjects::playlist->matchIndex(indexes.first(),matchEditor.matchInfo);
                    }
                    else
                    {
                         QList<const PlayListItem *> items;
                         QStringList eps;
                         for(int i=0;i<siblings.size();++i)
                         {
                             if(matchEditor.epCheckedList[i])
                             {
                                 items.append(siblings[i]);
                                 eps.append(matchEditor.batchEp[i]);
                             }
                         }
                         GlobalObjects::playlist->matchItems(items, matchEditor.batchAnime, eps);
                    }
                }
            }
            else if(!item->children)
            {
                showMessage(tr("Match Start"),PopMessageFlag::PM_PROCESS);
                MatchInfo *matchInfo=GlobalObjects::danmuManager->matchFrom(DanmuManager::DanDan,item->path);
                bool matchSuccess=(matchInfo && !matchInfo->error && matchInfo->success && matchInfo->matches.count()>0);
                if(matchSuccess)
                {
                    GlobalObjects::playlist->matchIndex(indexes.first(),matchInfo);
                }
                else
                {
                    MatchEditor matchEditor(GlobalObjects::playlist->getItem(indexes.first()),nullptr,this);
                    if(QDialog::Accepted==matchEditor.exec())
                        GlobalObjects::playlist->matchIndex(indexes.first(),matchEditor.matchInfo);
                    else
                        showMessage(tr("Match Done"),PopMessageFlag::PM_HIDE|PopMessageFlag::PM_OK);
                }
            }
            else
            {
                GlobalObjects::playlist->matchItems(indexes);
            }
            return;
        }
        GlobalObjects::playlist->matchItems(indexes);
    });
    act_removeMatch=new QAction(tr("Remove Match"),this);
    QObject::connect(act_removeMatch,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndexList indexes(selection.indexes());
        GlobalObjects::playlist->removeMatch(indexes);
    });
    act_autoMatchMode=new QAction(tr("Auto Match Mode"),this);
    act_autoMatchMode->setCheckable(true);
    act_autoMatchMode->setChecked(true);
    QObject::connect(act_autoMatchMode,&QAction::toggled, this, [](bool checked){
        GlobalObjects::playlist->setAutoMatch(checked);
        GlobalObjects::appSetting->setValue("List/AutoMatch",checked);
    });
    act_autoMatchMode->setChecked(GlobalObjects::appSetting->value("List/AutoMatch", true).toBool());

    act_markBgmCollection=new QAction(tr("Mark/Unmark Bangumi Collecion"),this);
    QObject::connect(act_markBgmCollection,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        GlobalObjects::playlist->switchBgmCollection(selIndex);
    });

    act_updateFolder=new QAction(tr("Scan Folder Changes"),this);
    QObject::connect(act_updateFolder,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        GlobalObjects::playlist->refreshFolder(selIndex);
    });

    act_addWebDanmuSource=new QAction(tr("Add Danmu Source"),this);
    QObject::connect(act_addWebDanmuSource,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        const PlayListItem *item=GlobalObjects::playlist->getItem(selIndex);
        if(item->poolID.isEmpty() || !GlobalObjects::danmuManager->getPool(item->poolID, false))
        {
            showMessage(tr("No pool associated"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
            return;
        }
        const QList<const PlayListItem *> &siblings=GlobalObjects::playlist->getSiblings(item);
        QMap<QString, QString> poolIdMap;
        QStringList poolTitles;
        for(auto sItem:siblings)
        {
            Pool *pool=GlobalObjects::danmuManager->getPool(sItem->poolID,false);
            if(pool)
            {
                poolIdMap.insert(pool->epTitle(),pool->id());
                poolTitles<<pool->epTitle();
            }
        }
        std::sort(poolTitles.begin(),poolTitles.end(), [&](const QString &s1, const QString &s2){
            return comparer.compare(s1, s2)>=0?false:true;
        });
        AddDanmu addDanmuDialog(item, this,true,poolTitles);
        if(QDialog::Accepted==addDanmuDialog.exec())
        {
            int i = 0;
            for(auto iter=addDanmuDialog.selectedDanmuList.begin();iter!=addDanmuDialog.selectedDanmuList.end();++iter)
            {
                Pool *pool=GlobalObjects::danmuManager->getPool(poolIdMap.value(addDanmuDialog.danmuToPoolList.at(i++)));
                DanmuSourceInfo &sourceInfo=(*iter).first;
                QList<DanmuComment *> &danmuList=(*iter).second;
                if(pool)
                {
                    showMessage(tr("Adding: %1").arg(pool->epTitle()),PopMessageFlag::PM_PROCESS);
                    if(pool->addSource(sourceInfo,danmuList,true)==-1)
                    {
                        qDeleteAll(danmuList);
                    }
                }
                else
                {
                    qDeleteAll(danmuList);
                }
            }
            showMessage(tr("Done adding"),PopMessageFlag::PM_OK|PopMessageFlag::PM_HIDE);
        }

    });
    act_updateDanmu=new QAction(tr("Update Danmu"),this);
    QObject::connect(act_updateDanmu,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndexList indexes(selection.indexes());
        actionDisable=true;
        updatePlaylistActions();
        act_addCollection->setEnabled(false);
        act_addFolder->setEnabled(false);
        act_addItem->setEnabled(false);
        playlistView->setDragEnabled(false);
        GlobalObjects::playlist->updateItemsDanmu(indexes);
        actionDisable=false;
        updatePlaylistActions();
        playlistView->setDragEnabled(true);
        act_addCollection->setEnabled(true);
        act_addFolder->setEnabled(true);
        act_addItem->setEnabled(true);
    });
    act_exportDanmu=new QAction(tr("Export Danmu"),this);
    QObject::connect(act_exportDanmu,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndexList indexes(selection.indexes());
        actionDisable=true;
        updatePlaylistActions();
        act_addCollection->setEnabled(false);
        act_addFolder->setEnabled(false);
        act_addItem->setEnabled(false);
        playlistView->setDragEnabled(false);
        GlobalObjects::playlist->exportDanmuItems(indexes);
        actionDisable=false;
        updatePlaylistActions();
        playlistView->setDragEnabled(true);
        act_addCollection->setEnabled(true);
        act_addFolder->setEnabled(true);
        act_addItem->setEnabled(true);
    });

    act_shareResourceCode=new QAction(tr("Resource Code"),this);
    QObject::connect(act_shareResourceCode,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        const PlayListItem *item=GlobalObjects::playlist->getItem(selIndex);
        Pool *pool = nullptr;
        if(item->poolID.isEmpty() || !(pool=GlobalObjects::danmuManager->getPool(item->poolID, false)))
        {
            showMessage(tr("No pool associated"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
            return;
        }
        InputDialog inputDialog(tr("Resource URI"),tr("Set Resource URI(eg. magnet)\n"
                                                      "The KikoPlay Resource Code would contain the uri and the danmu pool info associated with the anime(only for single file)"),
                                GlobalObjects::downloadModel->findFileUri(item->path),false,this);
        if(QDialog::Accepted!=inputDialog.exec()) return;
        QString uri = inputDialog.text;
        QString file16MD5(GlobalObjects::danmuManager->getFileHash(item->path));
        QString code(pool->getPoolCode(QStringList({uri,pool->animeTitle(),pool->epTitle(),file16MD5})));
        if(code.isEmpty())
        {
            showMessage(tr("No Danmu Source to Share"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
        }
        else
        {
            QClipboard *cb = QApplication::clipboard();
            cb->setText("kikoplay:anime="+code);
            showMessage(tr("Resource Code has been Copied to Clipboard"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
        }
    });
    act_sharePoolCode=new QAction(tr("Danmu Pool Code"),this);
    QObject::connect(act_sharePoolCode,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        const PlayListItem *item=GlobalObjects::playlist->getItem(selIndex);
        Pool *pool = nullptr;
        if(item->poolID.isEmpty() || !(pool=GlobalObjects::danmuManager->getPool(item->poolID, false)))
        {
            showMessage(tr("No pool associated"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
            return;
        }
        QString code(pool->getPoolCode());
        if(code.isEmpty())
        {
            showMessage(tr("No Danmu Source to Share"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
        }
        else
        {
            QClipboard *cb = QApplication::clipboard();
            cb->setText("kikoplay:pool="+code);
            showMessage(tr("Pool Code has been Copied to Clipboard"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
        }
    });

    act_addCollection=new QAction(tr("Add Collection"),this);
    QObject::connect(act_addCollection,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QModelIndex newIndex = model->mapFromSource(GlobalObjects::playlist->addCollection(getPSParentIndex(),"new collection"));
        playlistView->scrollTo(newIndex, QAbstractItemView::PositionAtCenter);
        playlistView->edit(newIndex);
    });
    act_addItem = new QAction(tr("Add Item"),this);
    QObject::connect(act_addItem,&QAction::triggered,[this](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        QStringList files = QFileDialog::getOpenFileNames(this,tr("Select one or more media files"),"",
                                                         QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
        if (files.count())
        {
            GlobalObjects::playlist->addItems(files, getPSParentIndex());
        }
        if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });
    act_addFolder = new QAction(tr("Add Folder"),this);
    QObject::connect(act_addFolder,&QAction::triggered,[this](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        QString directory = QFileDialog::getExistingDirectory(this,
                                    tr("Select folder"), "",
                                    QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly);
        if (!directory.isEmpty())
        {
            GlobalObjects::playlist->addFolder(directory, getPSParentIndex());
        }
        if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });

    act_cut=new QAction(tr("Cut"),this);
    act_cut->setShortcut(QString("Ctrl+X"));
    QObject::connect(act_cut,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        GlobalObjects::playlist->cutItems(selection.indexes());
    });
    act_paste=new QAction(tr("Paste"),this);
    act_paste->setShortcut(QString("Ctrl+V"));
    QObject::connect(act_paste,&QAction::triggered,[this](){
        GlobalObjects::playlist->pasteItems(getPSParentIndex());
    });
    act_moveUp=new QAction(tr("Move Up"),this);
    QObject::connect(act_moveUp,&QAction::triggered,[this](){
        QModelIndexList selection = playlistView->selectionModel()->selectedIndexes();
        if(selection.size()==0)return;
        QModelIndex index(selection.last());
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        GlobalObjects::playlist->moveUpDown(model->mapToSource(index),true);
    });
    act_moveDown=new QAction(tr("Move Down"),this);
    QObject::connect(act_moveDown,&QAction::triggered,[this](){
        QModelIndexList selection = playlistView->selectionModel()->selectedIndexes();
        if(selection.size()==0)return;
        QModelIndex index(selection.last());
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        GlobalObjects::playlist->moveUpDown(model->mapToSource(index),false);
    });
    act_merge=new QAction(tr("Merge"),this);
    QObject::connect(act_merge,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if(selection.size()==0)return;
        QModelIndex newIndex = model->mapFromSource(GlobalObjects::playlist->mergeItems(selection.indexes()));
        playlistView->scrollTo(newIndex, QAbstractItemView::PositionAtCenter);
        playlistView->edit(newIndex);
    });

    act_remove=new QAction(tr("Remove"),this);
    act_remove->setShortcut(QKeySequence::Delete);
    QObject::connect(act_remove,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        GlobalObjects::playlist->deleteItems(selection.indexes());
    });
    act_clear=new QAction(tr("Clear"),this);
    QObject::connect(act_clear,&QAction::triggered,[this](){
       QMessageBox::StandardButton btn =QMessageBox::question(this,tr("Clear"),tr("Are you sure to clear the list ?"),QMessageBox::Yes|QMessageBox::No);
       if(btn==QMessageBox::Yes)
       {
           GlobalObjects::playlist->clear();
       }
    });

    act_browseFile=new QAction(tr("Browse File"),this);
    QObject::connect(act_browseFile,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndexList indexes(selection.indexes());
        const PlayListItem *item=GlobalObjects::playlist->getItem(indexes.last());
        if(item->path.isEmpty() && item->folderPath.isEmpty())return;
        QString command("Explorer /select," + QDir::toNativeSeparators(item->path.isEmpty()?item->folderPath:item->path));
        QProcess::startDetached(command);
    });

    act_sortSelectionAscending = new QAction(tr("Sort Ascending"),this);
    QObject::connect(act_sortSelectionAscending,&QAction::triggered,[this](){
        sortSelection(false,true);
    });
    act_sortSelectionDescending = new QAction(tr("Sort Descending"),this);
    QObject::connect(act_sortSelectionDescending,&QAction::triggered,[this](){
        sortSelection(false,false);
    });
    act_sortAllAscending = new QAction(tr("Sort All Ascending"),this);
    QObject::connect(act_sortAllAscending,&QAction::triggered,[this](){
        sortSelection(true,true);
    });
    act_sortAllDescending = new QAction(tr("Sort All Descending"),this);
    QObject::connect(act_sortAllDescending,&QAction::triggered,[this](){
        sortSelection(true,false);
    });

    loopModeGroup=new QActionGroup(this);
    act_noLoopOne = new QAction(tr("No Loop One"),this);
    act_noLoopOne->setCheckable(true);
    act_noLoopOne->setData(QVariant(int(PlayList::NO_Loop_One)));
    loopModeGroup->addAction(act_noLoopOne);
    act_noLoopAll = new QAction(tr("No Loop All"),this);
    act_noLoopAll->setCheckable(true);
    act_noLoopAll->setData(QVariant(int(PlayList::NO_Loop_All)));
    loopModeGroup->addAction(act_noLoopAll);
    act_loopOne = new QAction(tr("Loop One"),this);
    act_loopOne->setCheckable(true);
    act_loopOne->setData(QVariant(int(PlayList::Loop_One)));
    loopModeGroup->addAction(act_loopOne);
    act_loopAll = new QAction(tr("Loop All"),this);
    act_loopAll->setCheckable(true);
    act_loopAll->setData(QVariant(int(PlayList::Loop_All)));
    loopModeGroup->addAction(act_loopAll);
    act_random = new QAction(tr("Random"),this);
    act_random->setCheckable(true);
    act_random->setData(QVariant(int(PlayList::Random)));
    loopModeGroup->addAction(act_random);
    QObject::connect(loopModeGroup,&QActionGroup::triggered,[this](QAction *action){
        GlobalObjects::playlist->setLoopMode(static_cast<PlayList::LoopMode>(action->data().toInt()));
        GlobalObjects::appSetting->setValue("List/LoopMode",loopModeGroup->actions().indexOf(loopModeGroup->checkedAction()));
    });
    loopModeGroup->actions().at(GlobalObjects::appSetting->value("List/LoopMode",1).toInt())->trigger();

    act_addOnlineDanmu=new QAction(tr("Add Online Danmu"),this);
    QObject::connect(act_addOnlineDanmu,&QAction::triggered,[this](){
        const PlayListItem *currentItem=GlobalObjects::playlist->getCurrentItem();
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        AddDanmu addDanmuDialog(currentItem, this);
        if(QDialog::Accepted==addDanmuDialog.exec())
        {
            Pool *pool=GlobalObjects::danmuPool->getPool();
            for(auto iter=addDanmuDialog.selectedDanmuList.begin();iter!=addDanmuDialog.selectedDanmuList.end();++iter)
            {
                DanmuSourceInfo &sourceInfo=(*iter).first;
                QList<DanmuComment *> &danmuList=(*iter).second;
                if(pool->addSource(sourceInfo,danmuList,iter==addDanmuDialog.selectedDanmuList.end()-1)<0)
                {
                    qDeleteAll(danmuList);
                }
            }
        }
        if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });
    act_addLocalDanmu=new QAction(tr("Add Local Danmu"),this);
    QObject::connect(act_addLocalDanmu,&QAction::triggered,[this](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        QStringList files = QFileDialog::getOpenFileNames(this,tr("Select Xml File"),"","Xml File(*.xml) ");
        if(!files.isEmpty())
        {
            for(auto &file: files)
            {
                QList<DanmuComment *> tmplist;
                LocalProvider::LoadXmlDanmuFile(file,tmplist);
                DanmuSourceInfo sourceInfo;
                sourceInfo.delay=0;
                sourceInfo.name=file.mid(file.lastIndexOf('/')+1);
                sourceInfo.show=true;
                sourceInfo.url=file;
                sourceInfo.count=tmplist.count();
                if(GlobalObjects::danmuPool->getPool()->addSource(sourceInfo,tmplist,true)==-1)
                {
                    qDeleteAll(tmplist);
                    showMessage(tr("Add Failed: Pool is busy"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
                }
            }
        }
        if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });
    act_editBlock=new QAction(tr("Edit Block Rules"),this);
    QObject::connect(act_editBlock,&QAction::triggered,[this](){
        BlockEditor blockEditor(this);
        blockEditor.exec();
    });
    act_editPool=new QAction(tr("Edit Danmu Pool"),this);
    QObject::connect(act_editPool,&QAction::triggered,[this](){
        PoolEditor poolEditor(this);
        poolEditor.exec();
    });
    act_copyDanmuText=new QAction(tr("Copy Danmu Text"),this);
    QObject::connect(act_copyDanmuText,&QAction::triggered,[this](){
        QClipboard *cb = QApplication::clipboard();
        cb->setText(getSelectedDanmu()->text);
    });
    act_copyDanmuColor=new QAction(tr("Copy Danmu Color"),this);
    QObject::connect(act_copyDanmuColor,&QAction::triggered,[this](){
        QClipboard *cb = QApplication::clipboard();
        cb->setText(QString::number(getSelectedDanmu()->color,16));
    });
    act_copyDanmuSender=new QAction(tr("Copy Danmu Sender"),this);
    QObject::connect(act_copyDanmuSender,&QAction::triggered,[this](){
        QClipboard *cb = QApplication::clipboard();
        cb->setText(getSelectedDanmu()->sender);
    });
    act_blockText=new QAction(tr("Block Text"),this);
    QObject::connect(act_blockText,&QAction::triggered,[this](){
        BlockRule *rule=new BlockRule;
        rule->blockField=BlockRule::Field::DanmuText;
        rule->relation=BlockRule::Relation::Contain;
        rule->enable=true;
        rule->isRegExp=false;
        rule->usePreFilter=false;
        rule->content=getSelectedDanmu()->text;
        GlobalObjects::blocker->addBlockRule(rule);
        showMessage(tr("Blocked"),PopMessageFlag::PM_OK|PopMessageFlag::PM_HIDE);
    });
    act_blockColor=new QAction(tr("Block Color"),this);
    QObject::connect(act_blockColor,&QAction::triggered,[this](){
        BlockRule *rule=new BlockRule;
        rule->blockField=BlockRule::Field::DanmuColor;
        rule->relation=BlockRule::Relation::Equal;
        rule->enable=true;
        rule->isRegExp=false;
        rule->usePreFilter=false;
        rule->content=QString::number(getSelectedDanmu()->color,16);
        GlobalObjects::blocker->addBlockRule(rule);
        showMessage(tr("Blocked"),PopMessageFlag::PM_OK|PopMessageFlag::PM_HIDE);
    });
    act_blockSender=new QAction(tr("Block Sender"),this);
    QObject::connect(act_blockSender,&QAction::triggered,[this](){
        BlockRule *rule=new BlockRule;
        rule->blockField=BlockRule::Field::DanmuSender;
        rule->relation=BlockRule::Relation::Equal;
        rule->enable=true;
        rule->isRegExp=false;
        rule->usePreFilter=false;
        rule->content=getSelectedDanmu()->sender;
        GlobalObjects::blocker->addBlockRule(rule);
        showMessage(tr("Blocked"),PopMessageFlag::PM_OK|PopMessageFlag::PM_HIDE);
    });
    act_deleteDanmu=new QAction(tr("Delete"),this);
    QObject::connect(act_deleteDanmu,&QAction::triggered,[this](){
        GlobalObjects::danmuPool->deleteDanmu(getSelectedDanmu());
    });
    act_jumpToTime=new QAction(tr("Jump to"),this);
    QObject::connect(act_jumpToTime,&QAction::triggered,[this](){
        MPVPlayer::PlayState state=GlobalObjects::mpvplayer->getState();
        if(state==MPVPlayer::PlayState::Play || state==MPVPlayer::PlayState::Pause)
            GlobalObjects::mpvplayer->seek(getSelectedDanmu()->time);
    });

    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::durationChanged,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        playlistView->scrollTo(model->mapFromSource(GlobalObjects::playlist->getCurrentIndex()), QAbstractItemView::EnsureVisible);
    });
}

QModelIndex ListWindow::getPSParentIndex()
{
    QModelIndexList selection = playlistView->selectionModel()->selectedIndexes();
    QModelIndex parentIndex = selection.size() == 0 ? QModelIndex() : selection.last();
    QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
    return model->mapToSource(parentIndex);
}

QSharedPointer<DanmuComment> ListWindow::getSelectedDanmu()
{
    QModelIndexList selection =danmulistView->selectionModel()->selectedRows();
    //QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(danmulistView->model());
    return GlobalObjects::danmuPool->getDanmu(selection.last());
}

void ListWindow::updatePlaylistActions()
{
    if(actionDisable)
    {
        act_autoAssociate->setEnabled(false);
        act_removeMatch->setEnabled(false);
        act_exportDanmu->setEnabled(false);
        act_updateDanmu->setEnabled(false);
        act_cut->setEnabled(false);
        act_remove->setEnabled(false);
        act_moveUp->setEnabled(false);
        act_moveDown->setEnabled(false);
        act_paste->setEnabled(false);
        act_merge->setEnabled(false);
        return;
    }
    bool hasPlaylistSelection = !playlistView->selectionModel()->selection().isEmpty();
    act_autoAssociate->setEnabled(hasPlaylistSelection);
    act_removeMatch->setEnabled(hasPlaylistSelection);
    act_updateDanmu->setEnabled(hasPlaylistSelection);
    act_addWebDanmuSource->setEnabled(hasPlaylistSelection);
    act_cut->setEnabled(hasPlaylistSelection);
    act_remove->setEnabled(hasPlaylistSelection);
    act_moveUp->setEnabled(hasPlaylistSelection);
    act_moveDown->setEnabled(hasPlaylistSelection);
    act_merge->setEnabled(hasPlaylistSelection);
    act_browseFile->setEnabled(hasPlaylistSelection);
    act_exportDanmu->setEnabled(hasPlaylistSelection);
    act_updateFolder->setEnabled(hasPlaylistSelection);
    act_paste->setEnabled(GlobalObjects::playlist->canPaste());  
}

void ListWindow::updateDanmuActions()
{
    bool hasDanmuSelection = !danmulistView->selectionModel()->selection().isEmpty();
    act_copyDanmuText->setEnabled(hasDanmuSelection);
    act_copyDanmuColor->setEnabled(hasDanmuSelection);
    act_copyDanmuSender->setEnabled(hasDanmuSelection);
    act_blockText->setEnabled(hasDanmuSelection);
    act_blockColor->setEnabled(hasDanmuSelection);
    act_blockSender->setEnabled(hasDanmuSelection);
    act_jumpToTime->setEnabled(hasDanmuSelection);
    act_deleteDanmu->setEnabled(hasDanmuSelection);
}

QWidget *ListWindow::setupPlaylistPage()
{
    QWidget *playlistPage=new QWidget(this);
    playlistPage->setObjectName(QStringLiteral("playlistPage"));

    QVBoxLayout *playlistPageVLayout=new QVBoxLayout(playlistPage);
    playlistPageVLayout->setContentsMargins(0,0,0,0);
    playlistPageVLayout->setSpacing(0);
    FilterBox *filter=new FilterBox(playlistPage);
    filter->setObjectName(QStringLiteral("filterBox"));
    filter->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
    playlistPageVLayout->addWidget(filter);

    playlistView=new QTreeView(playlistPage);
    playlistView->setObjectName(QStringLiteral("playlist"));
    playlistView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    playlistView->setEditTriggers(QAbstractItemView::SelectedClicked);
    playlistView->setDragEnabled(true);
    playlistView->setAcceptDrops(true);
    playlistView->setDragDropMode(QAbstractItemView::InternalMove);
    playlistView->setDropIndicatorShown(true);
    playlistView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    playlistView->header()->hide();
    playlistView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    playlistView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    playlistView->setFont(QFont("Microsoft YaHei UI",12));
    playlistView->setContextMenuPolicy(Qt::CustomContextMenu);
    playlistView->setIndentation(12*logicalDpiX()/96);
    playlistView->setItemDelegate(new TextColorDelegate(this));

    QSortFilterProxyModel *proxyModel=new QSortFilterProxyModel(this);
    proxyModel->setRecursiveFilteringEnabled(true);
    proxyModel->setSourceModel(GlobalObjects::playlist);
    playlistView->setModel(proxyModel);

    QObject::connect(filter,&FilterBox::filterChanged,[proxyModel,filter](){
        QRegExp regExp(filter->text(),filter->caseSensitivity(),filter->patternSyntax());
        proxyModel->setFilterRegExp(regExp);
    });

    QObject::connect(playlistView, &QTreeView::doubleClicked, [this](const QModelIndex &index) {
        playItem(index, false);
    });
    QObject::connect(playlistView->selectionModel(), &QItemSelectionModel::selectionChanged,this,&ListWindow::updatePlaylistActions);

    QMenu *playlistContextMenu=new QMenu(playlistView);
    playlistContextMenu->addAction(act_play);
    QMenu *matchSubMenu=new QMenu(tr("Match"),playlistContextMenu);
    matchSubMenu->addAction(act_autoAssociate);
    matchSubMenu->addAction(act_removeMatch);
    playlistContextMenu->addMenu(matchSubMenu);
    QMenu *danmuSubMenu=new QMenu(tr("Danmu"),playlistContextMenu);
    danmuSubMenu->addAction(act_addWebDanmuSource);
    danmuSubMenu->addAction(act_updateDanmu);
    danmuSubMenu->addAction(act_exportDanmu);
    playlistContextMenu->addMenu(danmuSubMenu);
    QMenu *addSubMenu=new QMenu(tr("Add"),playlistContextMenu);
    addSubMenu->addAction(act_addCollection);
    addSubMenu->addAction(act_addItem);
    addSubMenu->addAction(act_addFolder);
    playlistContextMenu->addMenu(addSubMenu);
    playlistContextMenu->addSeparator();
    QMenu *editSubMenu=new QMenu(tr("Edit"),playlistContextMenu);
    editSubMenu->addAction(act_merge);
    editSubMenu->addAction(act_cut);
    editSubMenu->addAction(act_paste);
    playlistView->addAction(act_cut);
    playlistView->addAction(act_paste);
    playlistContextMenu->addMenu(editSubMenu);
    QMenu *markSubMenu=new QMenu(tr("Mark"),playlistContextMenu);
    markSubMenu->addAction(act_markBgmCollection);
    playlistContextMenu->addMenu(markSubMenu);
    playlistContextMenu->addAction(act_remove);
    playlistContextMenu->addSeparator();
    QMenu *moveSubMenu=new QMenu(tr("Move"),playlistContextMenu);
    moveSubMenu->addAction(act_moveUp);
    moveSubMenu->addAction(act_moveDown);
    playlistContextMenu->addMenu(moveSubMenu);
    QMenu *sortSubMenu=new QMenu(tr("Sort"),playlistContextMenu);
    sortSubMenu->addAction(act_sortSelectionAscending);
    sortSubMenu->addAction(act_sortSelectionDescending);
    playlistContextMenu->addMenu(sortSubMenu);
    QMenu *shareSubMenu=new QMenu(tr("Share"),playlistContextMenu);
    shareSubMenu->addAction(act_sharePoolCode);
    shareSubMenu->addAction(act_shareResourceCode);
    playlistContextMenu->addMenu(shareSubMenu);
    playlistContextMenu->addAction(act_updateFolder);
    playlistContextMenu->addAction(act_browseFile);
    playlistContextMenu->addSeparator();
    playlistContextMenu->addAction(act_autoMatchMode);

    QObject::connect(playlistView,&QTreeView::customContextMenuRequested,[playlistContextMenu](){
        playlistContextMenu->exec(QCursor::pos());
    });



    playlistPageVLayout->addWidget(playlistView);

    GlobalObjects::iconfont.setPointSize(14);
    QToolButton *tb_add=new QToolButton(playlistPage);
    tb_add->setFont(GlobalObjects::iconfont);
    tb_add->setText(QChar(0xe6f3));
    tb_add->setObjectName(QStringLiteral("ListEditButton"));
    tb_add->setToolButtonStyle(Qt::ToolButtonTextOnly);
    tb_add->addAction(act_addCollection);
    tb_add->addAction(act_addItem);
    tb_add->addAction(act_addFolder);
    tb_add->setPopupMode(QToolButton::InstantPopup);
    tb_add->setToolTip(tr("Add"));

    QToolButton *tb_remove=new QToolButton(playlistPage);
    tb_remove->setFont(GlobalObjects::iconfont);
    tb_remove->setText(QChar(0xe68e));
    tb_remove->setObjectName(QStringLiteral("ListEditButton"));
    tb_remove->setToolButtonStyle(Qt::ToolButtonTextOnly);
    tb_remove->addAction(act_remove);
    tb_remove->addAction(act_clear);
    tb_remove->setPopupMode(QToolButton::InstantPopup);
    tb_remove->setToolTip(tr("Remove"));

    QToolButton *tb_sort=new QToolButton(playlistPage);
    tb_sort->setFont(GlobalObjects::iconfont);
    tb_sort->setText(QChar(0xe60e));
    tb_sort->setObjectName(QStringLiteral("ListEditButton"));
    tb_sort->setToolButtonStyle(Qt::ToolButtonTextOnly);
    tb_sort->addAction(act_sortSelectionAscending);
    tb_sort->addAction(act_sortSelectionDescending);
    tb_sort->addAction(act_sortAllAscending);
    tb_sort->addAction(act_sortAllDescending);
    tb_sort->setPopupMode(QToolButton::InstantPopup);
    tb_sort->setToolTip(tr("Sort"));

    QToolButton *tb_playmode=new QToolButton(playlistPage);
    tb_playmode->setFont(GlobalObjects::iconfont);
    tb_playmode->setText(QChar(0xe608));
    tb_playmode->setObjectName(QStringLiteral("ListEditButton"));
    tb_playmode->setToolButtonStyle(Qt::ToolButtonTextOnly);
    tb_playmode->setPopupMode(QToolButton::InstantPopup);
    tb_playmode->addActions(loopModeGroup->actions());
    tb_playmode->setToolTip(tr("Loop Mode"));

    QHBoxLayout *listEditHLayout=new QHBoxLayout();
    listEditHLayout->setContentsMargins(0,0,0,0);
    listEditHLayout->addWidget(tb_add);
    listEditHLayout->addWidget(tb_remove);
    listEditHLayout->addWidget(tb_sort);
    listEditHLayout->addWidget(tb_playmode);
    QSpacerItem *listHSpacer = new QSpacerItem(0, 10, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    listEditHLayout->addItem(listHSpacer);

    playlistPageVLayout->addLayout(listEditHLayout);

    return playlistPage;
}

QWidget *ListWindow::setupDanmulistPage()
{
    QWidget *danmulistPage=new QWidget(this);
    danmulistPage->setObjectName(QStringLiteral("danmulistPage"));
    QVBoxLayout *danmulistPageVLayout=new QVBoxLayout(danmulistPage);
    danmulistPageVLayout->setContentsMargins(0,0,0,0);
    danmulistPageVLayout->setSpacing(0);

    QFont normalFont("Microsoft YaHei UI",11);

    danmulistView=new QTreeView(danmulistPage);
    danmulistView->setObjectName(QStringLiteral("danmulist"));
    danmulistView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    danmulistView->setRootIsDecorated(false);
    danmulistView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    danmulistView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    danmulistView->setFont(normalFont);
    danmulistView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    danmulistView->setItemDelegate(new TextColorDelegate(this));

    danmulistView->setContextMenuPolicy(Qt::ActionsContextMenu);
    danmulistView->addAction(act_copyDanmuText);
    danmulistView->addAction(act_copyDanmuColor);
    danmulistView->addAction(act_copyDanmuSender);
    QAction *act_separator0=new QAction(this);
    act_separator0->setSeparator(true);
    danmulistView->addAction(act_separator0);
    danmulistView->addAction(act_blockText);
    danmulistView->addAction(act_blockColor);
    danmulistView->addAction(act_blockSender);
    QAction *act_separator1=new QAction(this);
    act_separator1->setSeparator(true);
    danmulistView->addAction(act_separator1);
    danmulistView->addAction(act_deleteDanmu);
    danmulistView->addAction(act_jumpToTime);

    //QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    //proxyModel->setSourceModel(GlobalObjects::danmuPool);
    danmulistView->setModel(GlobalObjects::danmuPool);
	QObject::connect(danmulistView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ListWindow::updateDanmuActions);
    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::modelReset, this, &ListWindow::updateDanmuActions);


    QHeaderView *danmulistHeader=danmulistView->header();
    danmulistHeader->setFont(normalFont);
    QFontMetrics fontMetrics(normalFont);
    danmulistHeader->resizeSection(0,fontMetrics.width("00:00")+10);
    danmulistPageVLayout->addWidget(danmulistView);

    GlobalObjects::iconfont.setPointSize(14);
    QToolButton *updatePool=new QToolButton(danmulistPage);
    updatePool->setFont(GlobalObjects::iconfont);
    updatePool->setText(QChar(0xe636));
    updatePool->setObjectName(QStringLiteral("ListEditButton"));
    updatePool->setToolButtonStyle(Qt::ToolButtonTextOnly);
    updatePool->setToolTip(tr("Update Danmu Pool"));
    QObject::connect(updatePool,&QToolButton::clicked, this, &ListWindow::updateCurrentPool);

    QToolButton *addDanmu=new QToolButton(danmulistPage);
    addDanmu->setFont(GlobalObjects::iconfont);
    addDanmu->setText(QChar(0xe667));
    addDanmu->setObjectName(QStringLiteral("ListEditButton"));
    addDanmu->setToolButtonStyle(Qt::ToolButtonTextOnly);
    addDanmu->addAction(act_addOnlineDanmu);
    addDanmu->addAction(act_addLocalDanmu);
    addDanmu->setPopupMode(QToolButton::InstantPopup);
    addDanmu->setToolTip(tr("Add Danmu"));

    QToolButton *edit=new QToolButton(danmulistPage);
    edit->setFont(GlobalObjects::iconfont);
    edit->setText(QChar(0xe60a));
    edit->setObjectName(QStringLiteral("ListEditButton"));
    edit->setToolButtonStyle(Qt::ToolButtonTextOnly);
    edit->addAction(act_editPool);
    edit->addAction(act_editBlock);
    edit->setPopupMode(QToolButton::InstantPopup);
    edit->setToolTip(tr("Edit"));

    QToolButton *locatePosition=new QToolButton(danmulistPage);
    locatePosition->setFont(GlobalObjects::iconfont);
    locatePosition->setText(QChar(0xe602));
    locatePosition->setObjectName(QStringLiteral("ListEditButton"));
    locatePosition->setToolButtonStyle(Qt::ToolButtonTextOnly);
    locatePosition->setToolTip(tr("Position"));
	QObject::connect(locatePosition, &QToolButton::clicked, [this]() {
        danmulistView->scrollTo(GlobalObjects::danmuPool->getCurrentIndex(), QAbstractItemView::PositionAtTop);
	});


    QHBoxLayout *poolEditHLayout=new QHBoxLayout();
    //poolEditHLayout->setSpacing(2);
    poolEditHLayout->addWidget(updatePool);
    poolEditHLayout->addWidget(addDanmu);
    poolEditHLayout->addWidget(edit);
    poolEditHLayout->addWidget(locatePosition);
    QSpacerItem *listHSpacer = new QSpacerItem(0, 10, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    poolEditHLayout->addItem(listHSpacer);
    danmulistPageVLayout->addLayout(poolEditHLayout);

    return danmulistPage;
}

int ListWindow::updateCurrentPool()
{
    act_autoAssociate->setEnabled(false);
    act_addOnlineDanmu->setEnabled(false);
    act_addLocalDanmu->setEnabled(false);
    const auto &sources =  GlobalObjects::danmuPool->getPool()->sources();
    int count=0;
    for(auto iter=sources.cbegin();iter!=sources.cend();++iter)
    {
        QList<DanmuComment *> tmpList;
        showMessage(tr("Updating: %1").arg(iter.value().url),PopMessageFlag::PM_PROCESS);
        count+=GlobalObjects::danmuPool->getPool()->update(iter.key());
    }
    showMessage(tr("Add %1 Danmu").arg(count),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
    act_autoAssociate->setEnabled(true);
    act_addOnlineDanmu->setEnabled(true);
    act_addLocalDanmu->setEnabled(true);
    return count;
}

void ListWindow::infoCancelClicked()
{
    if(ccb)
    {
        ccb(this);
    }
}

void ListWindow::resizeEvent(QResizeEvent *)
{
    infoTip->setGeometry(0,height()-90,width(),50);
    infoTip->raise();
}

void ListWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void ListWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls(event->mimeData()->urls());
    if(playlistPageButton->isChecked())
    {
        QStringList fileList,dirList;
        for(QUrl &url:urls)
        {
            if(url.isLocalFile())
            {
                QFileInfo fi(url.toLocalFile());
                if(fi.isFile())
                {
                    if(GlobalObjects::mpvplayer->videoFileFormats.contains("*."+fi.suffix().toLower()))
                    {
                        fileList.append(fi.filePath());
                    }
                }
                else
                {
                    dirList.append(fi.filePath());
                }
            }
        }
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QModelIndex pointIndex(model->mapToSource(playlistView->indexAt(event->pos())));
        if(!fileList.isEmpty())
        {
            GlobalObjects::playlist->addItems(fileList,pointIndex);

        }
        for(QString dir:dirList)
        {
            GlobalObjects::playlist->addFolder(dir,pointIndex);
        }
    }
    else if(danmulistPageButton->isChecked())
    {
        for(QUrl &url:urls)
        {
            if(url.isLocalFile())
            {
                QFileInfo fi(url.toLocalFile());
                if(fi.isFile())
                {
                    if("xml"==fi.suffix())
                    {
                        QList<DanmuComment *> tmplist;
                        LocalProvider::LoadXmlDanmuFile(fi.filePath(),tmplist);
                        DanmuSourceInfo sourceInfo;
                        sourceInfo.delay=0;
                        sourceInfo.name=fi.fileName();
                        sourceInfo.show=true;
                        sourceInfo.url=fi.filePath();
                        sourceInfo.count=tmplist.count();
                        if(GlobalObjects::danmuPool->getPool()->addSource(sourceInfo,tmplist,true)==-1)
                        {
                            qDeleteAll(tmplist);
                            showMessage(tr("Add Failed: Pool is busy"),PopMessageFlag::PM_INFO|PopMessageFlag::PM_HIDE);
                        }
                    }
                }
            }
        }
    }
}

void ListWindow::enterEvent(QEvent *)
{
    playlistView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    danmulistView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void ListWindow::leaveEvent(QEvent *)
{
    playlistView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    danmulistView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void ListWindow::sortSelection(bool allItem, bool ascending)
{
    if(allItem)
    {
        GlobalObjects::playlist->sortAllItems(ascending);
    }
    else
    {
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)
        {
            GlobalObjects::playlist->sortItems(QModelIndex(),ascending);
        }
        else
        {
            for(QModelIndex index : selection.indexes())
            {
                GlobalObjects::playlist->sortItems(index,ascending);
            }
        }
    }
}

void ListWindow::playItem(const QModelIndex &index, bool playChild)
{
    int playTime=GlobalObjects::mpvplayer->getTime();
    GlobalObjects::playlist->setCurrentPlayTime(playTime);
    QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
    const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(model->mapToSource(index),playChild);
    if (curItem)
    {
        GlobalObjects::danmuPool->cleanUp();
        GlobalObjects::danmuRender->cleanup();
        GlobalObjects::mpvplayer->setMedia(curItem->path);
    }
}

void ListWindow::showMessage(const QString &msg,int flag)
{
    infoTip->show();
    infoTip->setGeometry(0, height() - 60*logicalDpiY()/96, width(), 30*logicalDpiX()/96);
    static_cast<InfoTip *>(infoTip)->showMessage(msg,flag);
}

FilterBox::FilterBox(QWidget *parent)
    : QLineEdit(parent)
    , m_patternGroup(new QActionGroup(this))
{
    setClearButtonEnabled(true);
    setFont(QFont("Microsoft YaHei",12));
    connect(this, &QLineEdit::textChanged, this, &FilterBox::filterChanged);

    QMenu *menu = new QMenu(this);
    m_caseSensitivityAction = menu->addAction(tr("Case Sensitive"));
    m_caseSensitivityAction->setCheckable(true);
    connect(m_caseSensitivityAction, &QAction::toggled, this, &FilterBox::filterChanged);

    menu->addSeparator();
    m_patternGroup->setExclusive(true);
    QAction *patternAction = menu->addAction(tr("Fixed String"));
    patternAction->setData(QVariant(int(QRegExp::FixedString)));
    patternAction->setCheckable(true);
    patternAction->setChecked(true);
    m_patternGroup->addAction(patternAction);
    patternAction = menu->addAction(tr("Regular Expression"));
    patternAction->setCheckable(true);
    patternAction->setData(QVariant(int(QRegExp::RegExp2)));
    m_patternGroup->addAction(patternAction);
    patternAction = menu->addAction(tr("Wildcard"));
    patternAction->setCheckable(true);
    patternAction->setData(QVariant(int(QRegExp::Wildcard)));
    m_patternGroup->addAction(patternAction);
    connect(m_patternGroup, &QActionGroup::triggered, this, &FilterBox::filterChanged);

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

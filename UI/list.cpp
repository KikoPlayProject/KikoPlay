#include "list.h"
#include <QHeaderView>
#include <QMenu>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QApplication>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>

#include "globalobjects.h"
#include "pooleditor.h"
#include "adddanmu.h"
#include "matcheditor.h"
#include "blockeditor.h"
#include "inputdialog.h"
#include "dlnadiscover.h"
#include "widgets/loadingicon.h"
#include "Play/Danmu/Provider/localprovider.h"
#include "MediaLibrary/animeprovider.h"
#include "Play/playcontext.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/blocker.h"
#include "Play/Danmu/Render/danmurender.h"
#include "Play/Danmu/danmuprovider.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Download/downloadmodel.h"

namespace
{
    class PlayListItemDelegate: public QStyledItemDelegate
    {
    public:
        explicit PlayListItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent)
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
            static QIcon colorMarkerIcon[] =
            {
                QIcon(":/res/images/mark_red.svg"),
                QIcon(":/res/images/mark_blue.svg"),
                QIcon(":/res/images/mark_green.svg"),
                QIcon(":/res/images/mark_orange.svg"),
                QIcon(":/res/images/mark_pink.svg"),
                QIcon(":/res/images/mark_yellow.svg"),
                QIcon()
            };
            QIcon *marker(nullptr);
            if(index.data(PlayList::ItemRole::BgmCollectionRole).toBool())
            {
                marker = &bgmCollectionIcon;

            }
            else if(index.data(PlayList::ItemRole::FolderCollectionRole).toBool())
            {
                marker = &folderIcon;
            }
            PlayListItem::Marker colorMarker = (PlayListItem::Marker)index.data(PlayList::ItemRole::ColorMarkerRole).toInt();
            if(marker || colorMarker != PlayListItem::Marker::M_NONE)
            {
                QRect decoration = option.rect;
                decoration.setHeight(decoration.height()-10);
                decoration.setWidth(decoration.height());
                decoration.moveCenter(option.rect.center());
                decoration.moveRight(option.rect.width()+option.rect.x()-10);
                if(marker)
                {
                    marker->paint(painter, decoration);
                }

                if(colorMarker != PlayListItem::Marker::M_NONE)
                {
                    if(marker) decoration.moveLeft(decoration.left() - decoration.width() - 4);
                    colorMarkerIcon[colorMarker].paint(painter, decoration);
                }
            }
        }
    };
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
        }
    };
    class InfoTip : public QWidget
    {
    public:
        explicit InfoTip(QWidget *parent=nullptr):QWidget(parent)
        {
            setObjectName(QStringLiteral("ListInfoBar"));
            loadingIcon = new LoadingIcon(Qt::white, this);
            loadingIcon->setMargin(4 * logicalDpiX() / 96);
            loadingIcon->hide();

            infoText=new QLabel(this);
            infoText->setObjectName(QStringLiteral("labelListInfo"));
            infoText->setFont(QFont(GlobalObjects::normalFont,10));
            infoText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
            cancelBtn = new QPushButton(QObject::tr("Cancel"), this);
            QHBoxLayout *infoBarHLayout=new QHBoxLayout(this);
            infoBarHLayout->setContentsMargins(5,0,5,0);
            infoBarHLayout->setSpacing(4);
            infoBarHLayout->addWidget(loadingIcon);
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
            if(flag&NotifyMessageFlag::NM_HIDE)
            {
                hideTimer.setSingleShot(true);
                hideTimer.start(3000);
            }
            QString icon;
            if(flag&NotifyMessageFlag::NM_PROCESS)
                loadingIcon->show();
            else
                loadingIcon->hide();
            if(flag&NotifyMessageFlag::NM_SHOWCANCEL)
                cancelBtn->show();
            else
                cancelBtn->hide();
            infoText->setText(msg);
        }
    private:
        QLabel *infoText;
        LoadingIcon *loadingIcon;
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

    QFont normalFont(GlobalObjects::normalFont,11);
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
            act_addURL->setEnabled(true);
        }
        else
        {
            actionDisable=true;
            updatePlaylistActions();
            act_addCollection->setEnabled(false);
            act_addFolder->setEnabled(false);
            act_addItem->setEnabled(false);
            act_addURL->setEnabled(false);
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
    act_autoMatch=new QAction(tr("Associate Danmu Pool"),this);
	QObject::connect(act_autoMatch, &QAction::triggered, this, [=]() {matchPool(); });
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

    act_addWebDanmuSource=new QAction(tr("Add Web Danmu Source"),this);
    QObject::connect(act_addWebDanmuSource,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        const PlayListItem *item=GlobalObjects::playlist->getItem(selIndex);
        if(!item->hasPool())
        {
            showMessage(tr("No pool associated"), NotifyMessageFlag::NM_HIDE);
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
                EpInfo ep(pool->toEp());
                poolIdMap.insert(ep.toString(),pool->id());
                poolTitles<<ep.toString();
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
                DanmuSource &sourceInfo=(*iter).first;
				QVector<DanmuComment *> &danmuList=(*iter).second;
                if(pool)
                {
                    showMessage(tr("Adding: %1").arg(pool->epTitle()),NotifyMessageFlag::NM_PROCESS);
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
            showMessage(tr("Done adding"), NotifyMessageFlag::NM_HIDE);
        }

    });
    act_addLocalDanmuSource=new QAction(tr("Add Local Danmu Source"),this);
    QObject::connect(act_addLocalDanmuSource,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        const PlayListItem *item=GlobalObjects::playlist->getItem(selIndex);
        if(!item->hasPool())
        {
            showMessage(tr("No pool associated"), NotifyMessageFlag::NM_HIDE);
            return;
        }
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
				QVector<DanmuComment *> tmplist;
                LocalProvider::LoadXmlDanmuFile(file,tmplist);
                DanmuSource sourceInfo;
                sourceInfo.scriptData = file;
                sourceInfo.title=file.mid(file.lastIndexOf('/')+1);
                sourceInfo.count=tmplist.count();
                if(GlobalObjects::danmuManager->getPool(item->poolID)->addSource(sourceInfo,tmplist,true)==-1)
                {
                    qDeleteAll(tmplist);
                    showMessage(tr("Add Failed: Pool is busy"), NotifyMessageFlag::NM_HIDE);
                }
            }
        }
        if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
        showMessage(tr("Done adding"), NotifyMessageFlag::NM_HIDE);
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
        act_addURL->setEnabled(false);
        playlistView->setDragEnabled(false);
        GlobalObjects::playlist->updateItemsDanmu(indexes);
        actionDisable=false;
        updatePlaylistActions();
        playlistView->setDragEnabled(true);
        act_addCollection->setEnabled(true);
        act_addFolder->setEnabled(true);
        act_addItem->setEnabled(true);
        act_addURL->setEnabled(true);
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
        act_addURL->setEnabled(false);
        playlistView->setDragEnabled(false);
        GlobalObjects::playlist->exportDanmuItems(indexes);
        actionDisable=false;
        updatePlaylistActions();
        playlistView->setDragEnabled(true);
        act_addCollection->setEnabled(true);
        act_addFolder->setEnabled(true);
        act_addItem->setEnabled(true);
        act_addURL->setEnabled(true);
    });

    act_shareResourceCode=new QAction(tr("Resource Code"),this);
    QObject::connect(act_shareResourceCode,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        const PlayListItem *item=GlobalObjects::playlist->getItem(selIndex);
        Pool *pool = nullptr;
        if(!item->hasPool() || !(pool = GlobalObjects::danmuManager->getPool(item->poolID, false)))
        {
            showMessage(tr("No pool associated"), NotifyMessageFlag::NM_HIDE);
            return;
        }
        InputDialog inputDialog(tr("Resource URI"),tr("Set Resource URI(eg. magnet)\n"
                                                      "The KikoPlay Resource Code would contain the uri and the danmu pool info associated with the anime(only for single file)"),
                                GlobalObjects::downloadModel->findFileUri(item->path),false,this);
        if(QDialog::Accepted!=inputDialog.exec()) return;
        QString uri = inputDialog.text;
        QString file16MD5(GlobalObjects::danmuManager->getFileHash(item->path));
        EpInfo ep(pool->toEp());
        QString code(pool->getPoolCode(QStringList({uri,pool->animeTitle(),ep.name,QString::number(ep.type),QString::number(ep.index),file16MD5})));
        if(code.isEmpty())
        {
            showMessage(tr("No Danmu Source to Share"), NotifyMessageFlag::NM_HIDE);
        }
        else
        {
            QClipboard *cb = QApplication::clipboard();
            cb->setText("kikoplay:anime="+code);
            showMessage(tr("Resource Code has been Copied to Clipboard"), NotifyMessageFlag::NM_HIDE);
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
        if(!item->hasPool() || !(pool = GlobalObjects::danmuManager->getPool(item->poolID, false)))
        {
            showMessage(tr("No pool associated"), NotifyMessageFlag::NM_HIDE);
            return;
        }
        QString code(pool->getPoolCode());
        if(code.isEmpty())
        {
            showMessage(tr("No Danmu Source to Share"), NotifyMessageFlag::NM_HIDE);
        }
        else
        {
            QClipboard *cb = QApplication::clipboard();
            cb->setText("kikoplay:pool="+code);
            showMessage(tr("Pool Code has been Copied to Clipboard"), NotifyMessageFlag::NM_HIDE);
        }
    });

    act_addCollection=new QAction(tr("Add Collection"),this);
    QObject::connect(act_addCollection,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QModelIndex newIndex = model->mapFromSource(GlobalObjects::playlist->addCollection(getPSParentIndex(),QObject::tr("new collection")));
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
    act_addURL = new QAction(tr("Add URL"), this);
    QObject::connect(act_addURL, &QAction::triggered, [this](){
        AddUrlDialog dialog(this);
        if(QDialog::Accepted != dialog.exec()) return;
        QModelIndex parent = getPSParentIndex();
        if (!dialog.collectionTitle.isEmpty())
        {
            parent = GlobalObjects::playlist->addCollection(parent, dialog.collectionTitle);
        }
        GlobalObjects::playlist->addURL(dialog.urls, parent, dialog.decodeTitle);
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
    act_PlayOnOtherDevices = new QAction(tr("Play on other devices"), this);
    QObject::connect(act_PlayOnOtherDevices, &QAction::triggered, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        const PlayListItem *item = GlobalObjects::playlist->getItem(selIndex);
        if(item->children) return;
        QFileInfo fileInfo(item->path);
        if(!fileInfo.exists())
        {
            showMessage(tr("File Not Exist"), NM_HIDE);
            return;
        }
        DLNADiscover dlnaDiscover(item, this);
        if(QDialog::Accepted == dlnaDiscover.exec())
        {
            showMessage(tr("Play on %1: %2").arg(dlnaDiscover.deviceName, item->title), NM_HIDE);
        }
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

    act_removeInvalid=new QAction(tr("Remove Invalid Items"),this);
    QObject::connect(act_removeInvalid,&QAction::triggered,[this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        showMessage(tr("Remove %1 Invalid Item(s)").arg(GlobalObjects::playlist->deleteInvalidItems(selection.indexes())), NM_HIDE);
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
        if(item->path.isEmpty())return;
#ifdef Q_OS_WIN
        QProcess::startDetached("Explorer", {"/select,",  QDir::toNativeSeparators(item->path)});
#else
        QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(QFileInfo(item->path).absolutePath())));
#endif

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
                DanmuSource &sourceInfo=(*iter).first;
				QVector<DanmuComment *> &danmuList=(*iter).second;
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
				QVector<DanmuComment *> tmplist;
                LocalProvider::LoadXmlDanmuFile(file,tmplist);
                DanmuSource sourceInfo;
                sourceInfo.title=file.mid(file.lastIndexOf('/')+1);
                sourceInfo.show=true;
                sourceInfo.count=tmplist.count();
                if(GlobalObjects::danmuPool->getPool()->addSource(sourceInfo,tmplist,true)==-1)
                {
                    qDeleteAll(tmplist);
                    showMessage(tr("Add Failed: Pool is busy"), NotifyMessageFlag::NM_HIDE);
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
        showMessage(tr("Blocked"), NotifyMessageFlag::NM_HIDE);
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
        showMessage(tr("Blocked"), NotifyMessageFlag::NM_HIDE);
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
        showMessage(tr("Blocked"), NotifyMessageFlag::NM_HIDE);
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

    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::fileChanged,[this](){
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
    return GlobalObjects::danmuPool->getDanmu(selection.last());
}

void ListWindow::matchPool(const QString &scriptId)
{
    QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
    QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
    if (selection.size() == 0)return;
    QModelIndexList indexes(selection.indexes());
    const PlayListItem *item=GlobalObjects::playlist->getItem(indexes.first());
    if(indexes.size()==1 && !item->children)
    {
        if(!item->children)
        {
            bool matchSuccess = false;
            if(!item->hasPool() && item->type == PlayListItem::ItemType::LOCAL_FILE)
            {
                showMessage(tr("Match Start"),NotifyMessageFlag::NM_PROCESS);
                MatchResult match;
                GlobalObjects::danmuManager->localMatch(item->path, match);
                if(!match.success) GlobalObjects::animeProvider->match(scriptId.isEmpty()?GlobalObjects::animeProvider->defaultMatchScript():scriptId, item->path, match);
                if(match.success)
                {
                    GlobalObjects::playlist->matchIndex(indexes.first(), match);
                    matchSuccess = true;
                }
                showMessage(tr("Match Done"), NotifyMessageFlag::NM_HIDE);
            }
            if(!matchSuccess)
            {
                QList<const PlayListItem *> &&siblings=GlobalObjects::playlist->getSiblings(item, false);
                MatchEditor matchEditor(GlobalObjects::playlist->getItem(indexes.first()),&siblings,this);
                if(QDialog::Accepted==matchEditor.exec())
                {
                    if(matchEditor.singleEp.type!=EpType::UNKNOWN)
                    {
                        MatchResult match;
                        match.success = true;
                        match.name = matchEditor.anime;
                        match.ep = matchEditor.singleEp;
                        GlobalObjects::playlist->matchIndex(indexes.first(), match);
                    }
                    else
                    {
                         QList<const PlayListItem *> items;
                         QList<EpInfo> eps;
                         for(int i=0;i<siblings.size();++i)
                         {
                             if(matchEditor.epCheckedList[i])
                             {
                                 items.append(siblings[i]);
                                 eps.append(matchEditor.epList[i]);
                             }
                         }
                         GlobalObjects::playlist->matchItems(items, matchEditor.anime, eps);
                    }
                }
            }
        }
    } else {
        GlobalObjects::playlist->matchItems(indexes);
    }
}

void ListWindow::updatePlaylistActions()
{
    if(actionDisable)
    {
        matchSubMenu->setEnabled(false);
        markSubMenu->setEnabled(false);
        act_removeMatch->setEnabled(false);
        act_exportDanmu->setEnabled(false);
        act_updateDanmu->setEnabled(false);
        act_cut->setEnabled(false);
        act_remove->setEnabled(false);
        act_removeInvalid->setEnabled(false);
        act_moveUp->setEnabled(false);
        act_moveDown->setEnabled(false);
        act_paste->setEnabled(false);
        act_merge->setEnabled(false);
        return;
    }
    bool hasPlaylistSelection = !playlistView->selectionModel()->selection().isEmpty();
    matchSubMenu->setEnabled(hasPlaylistSelection);
    markSubMenu->setEnabled(hasPlaylistSelection);
    act_removeMatch->setEnabled(hasPlaylistSelection);
    act_updateDanmu->setEnabled(hasPlaylistSelection);
    act_addWebDanmuSource->setEnabled(hasPlaylistSelection);
    act_addLocalDanmuSource->setEnabled(hasPlaylistSelection);
    act_cut->setEnabled(hasPlaylistSelection);
    act_remove->setEnabled(hasPlaylistSelection);
    act_removeInvalid->setEnabled(hasPlaylistSelection);
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
    playlistView->setAnimated(true);
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
    playlistView->setFont(QFont(GlobalObjects::normalFont,12));
    playlistView->setContextMenuPolicy(Qt::CustomContextMenu);
    playlistView->setIndentation(12*logicalDpiX()/96);
    playlistView->setItemDelegate(new PlayListItemDelegate(this));

    QSortFilterProxyModel *proxyModel=new QSortFilterProxyModel(this);
    proxyModel->setRecursiveFilteringEnabled(true);
    proxyModel->setSourceModel(GlobalObjects::playlist);
    playlistView->setModel(proxyModel);

    QObject::connect(filter,&FilterBox::filterChanged,[proxyModel,filter](){
        QRegExp regExp(filter->text(),filter->caseSensitivity(), filter->patternSyntax());
        proxyModel->setFilterRole(filter->searchType() == FilterBox::FilePath? PlayList::FilePathRole : Qt::DisplayRole);
        proxyModel->setFilterRegExp(regExp);
    });

    QObject::connect(playlistView, &QTreeView::doubleClicked, [this](const QModelIndex &index) {
        playItem(index, false);
    });
    QObject::connect(playlistView->selectionModel(), &QItemSelectionModel::selectionChanged,this,&ListWindow::updatePlaylistActions);

    QMenu *playlistContextMenu=new QMenu(playlistView);
    playlistContextMenu->addAction(act_play);
    matchSubMenu=new QMenu(tr("Match"),playlistContextMenu);
    QMenu *defaultMatchScriptMenu=new QMenu(tr("Default Match Script"), matchSubMenu);
    matchSubMenu->addAction(act_autoMatch);
    matchSubMenu->addAction(act_removeMatch);
    matchSubMenu->addAction(act_autoMatchMode);
    matchSubMenu->addMenu(defaultMatchScriptMenu);
    defaultMatchScriptMenu->hide();
    QAction *matchSep = new QAction(this);
    matchSep->setSeparator(true);
    playlistContextMenu->addMenu(matchSubMenu);
    QActionGroup *matchCheckGroup = new QActionGroup(this);
    auto matchProviders = GlobalObjects::animeProvider->getMatchProviders();
    static QList<QAction *> matchActions;
    if(matchProviders.count()>0)
    {
        defaultMatchScriptMenu->show();
        matchSubMenu->addAction(matchSep);
        QString defaultSctiptId = GlobalObjects::animeProvider->defaultMatchScript();
        for(const auto &p : matchProviders)
        {
            QAction *mAct = new QAction(p.first);
            mAct->setData(p.second); //id
            matchActions.append(mAct);
            matchSubMenu->addAction(mAct);
            QAction *mCheckAct = defaultMatchScriptMenu->addAction(p.first);
            matchCheckGroup->addAction(mCheckAct);
            mCheckAct->setCheckable(true);
            mCheckAct->setData(p.second);
            if(p.second == defaultSctiptId)
            {
                mCheckAct->setChecked(true);
            }
        }
    }
    QObject::connect(matchSubMenu, &QMenu::triggered, this, [this, matchCheckGroup](QAction *act){
        if(matchCheckGroup->actions().contains(act)) return;
        QString scriptId = act->data().toString();
        if(!scriptId.isEmpty())
        {
            matchPool(scriptId);
        }
    });
    QObject::connect(matchCheckGroup,&QActionGroup::triggered,[](QAction *act){
        QString scriptId = act->data().toString();
        if(!scriptId.isEmpty())
        {
            GlobalObjects::animeProvider->setDefaultMatchScript(scriptId);
        }
    });
    QObject::connect(GlobalObjects::animeProvider, &AnimeProvider::matchProviderChanged, this, [=](){
         auto matchProviders = GlobalObjects::animeProvider->getMatchProviders();
         for(QAction *mAct : matchActions)
         {
             matchSubMenu->removeAction(mAct);
         }
         qDeleteAll(matchActions);
		 matchActions.clear();
         matchSubMenu->removeAction(matchSep);
         for(QAction *act : defaultMatchScriptMenu->actions())
         {
            matchCheckGroup->removeAction(act);
         }
         defaultMatchScriptMenu->clear();
         if(matchProviders.count()>0)
         {
             QString defaultSctiptId = GlobalObjects::animeProvider->defaultMatchScript();
             matchSubMenu->addAction(matchSep);
             for(const auto &p : matchProviders)
             {
                 QAction *mAct = new QAction(p.first);
                 mAct->setData(p.second); //id
                 matchActions.append(mAct);
                 matchSubMenu->addAction(mAct);
                 QAction *mCheckAct = defaultMatchScriptMenu->addAction(p.first);
                 matchCheckGroup->addAction(mCheckAct);
                 mCheckAct->setCheckable(true);
                 mCheckAct->setData(p.second);
                 mCheckAct->setChecked(p.second == defaultSctiptId);
             }
         }
    });

    QMenu *danmuSubMenu=new QMenu(tr("Danmu"),playlistContextMenu);
    danmuSubMenu->addAction(act_addWebDanmuSource);
    danmuSubMenu->addAction(act_addLocalDanmuSource);
    danmuSubMenu->addAction(act_updateDanmu);
    danmuSubMenu->addAction(act_exportDanmu);
    playlistContextMenu->addMenu(danmuSubMenu);
    QMenu *addSubMenu=new QMenu(tr("Add"),playlistContextMenu);
    addSubMenu->addAction(act_addCollection);
    addSubMenu->addAction(act_addItem);
    addSubMenu->addAction(act_addURL);
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

    markSubMenu=new QMenu(tr("Mark"),playlistContextMenu);
    markSubMenu->addAction(act_markBgmCollection);
    markSubMenu->addSeparator();

    QStringList markerName{tr("Red"), tr("Blue"), tr("Green"), tr("Orange"), tr("Pink"), tr("Yellow"), tr("None")};
    QStringList colorMarkerIcon
    {
        (":/res/images/mark_red.svg"),
        (":/res/images/mark_blue.svg"),
        (":/res/images/mark_green.svg"),
        (":/res/images/mark_orange.svg"),
        (":/res/images/mark_pink.svg"),
        (":/res/images/mark_yellow.svg"),
        ("")
    };
    QActionGroup *markerGroup = new QActionGroup(this);
    for(int i = 0; i<=PlayListItem::Marker::M_NONE; ++i)
    {
        QAction *act = markSubMenu->addAction(QIcon(colorMarkerIcon[i]), markerName[i]);
        markerGroup->addAction(act);
        act->setData(i);
    }
    QObject::connect(markerGroup, &QActionGroup::triggered, this, [this](QAction *act){
        int marker = act->data().toInt();
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        GlobalObjects::playlist->setMarker(selIndex, (PlayListItem::Marker)marker);
    });
    playlistContextMenu->addMenu(markSubMenu);
    playlistContextMenu->addAction(act_remove);

    QMenu *moveSubMenu=new QMenu(tr("Move"),playlistContextMenu);
    moveSubMenu->addAction(act_moveUp);
    moveSubMenu->addAction(act_moveDown);
    playlistContextMenu->addMenu(moveSubMenu);
    QMenu *sortSubMenu=new QMenu(tr("Sort"),playlistContextMenu);
    sortSubMenu->addAction(act_sortSelectionAscending);
    sortSubMenu->addAction(act_sortSelectionDescending);
    playlistContextMenu->addMenu(sortSubMenu);
    playlistContextMenu->addSeparator();

    QMenu *shareSubMenu=new QMenu(tr("Share"),playlistContextMenu);
    shareSubMenu->addAction(act_sharePoolCode);
    shareSubMenu->addAction(act_shareResourceCode);
    playlistContextMenu->addMenu(shareSubMenu);
    playlistContextMenu->addAction(act_PlayOnOtherDevices);
    playlistContextMenu->addSeparator();
    playlistContextMenu->addAction(act_updateFolder);
    playlistContextMenu->addAction(act_removeInvalid);
    playlistContextMenu->addAction(act_browseFile);

    QObject::connect(playlistView,&QTreeView::customContextMenuRequested,[=](){
        playlistContextMenu->exec(QCursor::pos());
    });

    playlistPageVLayout->addWidget(playlistView);

    constexpr const int tbBtnCount = 4;
    QPair<QChar, QString> tbButtonTexts[tbBtnCount] = {
        {QChar(0xe6f3), tr("Add")},
        {QChar(0xe68e), tr("Remove")},
        {QChar(0xe60e), tr("Sort")},
        {QChar(0xe608), tr("Loop Mode")}
    };
    QList<QAction *> tbActions[tbBtnCount] = {
        {act_addCollection, act_addItem, act_addURL, act_addFolder},
        {act_remove, act_clear},
        {act_sortSelectionAscending, act_sortSelectionDescending, act_sortAllAscending, act_sortAllDescending},
        {loopModeGroup->actions()}
    };

    GlobalObjects::iconfont->setPointSize(14);
    QHBoxLayout *listEditHLayout=new QHBoxLayout();
    listEditHLayout->setContentsMargins(0,0,0,0);

    for(int i = 0; i < tbBtnCount; ++i)
    {
        QToolButton *tb = new QToolButton(playlistPage);
        tb->setFont(*GlobalObjects::iconfont);
        tb->setText(tbButtonTexts[i].first);
        tb->setToolTip(tbButtonTexts[i].second);
        tb->addActions(tbActions[i]);
        tb->setObjectName(QStringLiteral("ListEditButton"));
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setPopupMode(QToolButton::InstantPopup);
        listEditHLayout->addWidget(tb);
#ifdef Q_OS_MACOS
        tb->setProperty("hideMenuIndicator", true);
#endif
    }
    listEditHLayout->addStretch(1);
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

    QFont normalFont(GlobalObjects::normalFont,11);

    danmulistView=new QTreeView(danmulistPage);
    danmulistView->setObjectName(QStringLiteral("danmulist"));
    danmulistView->setAnimated(true);
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
    danmulistHeader->resizeSection(0,fontMetrics.horizontalAdvance("00:00")+10);
    danmulistPageVLayout->addWidget(danmulistView);

    constexpr const int tbBtnCount = 4;
    QPair<QChar, QString> tbButtonTexts[tbBtnCount] = {
        {QChar(0xe636), tr("Update Danmu Pool")},
        {QChar(0xe667), tr("Add Danmu")},
        {QChar(0xe60a), tr("Edit")},
        {QChar(0xe602), tr("Position")}
    };
    QList<QAction *> tbActions[tbBtnCount] = {
        {},
        {act_addOnlineDanmu, act_addLocalDanmu},
        {act_editPool, act_editBlock},
        {}
    };

    QVector<QToolButton *> btns(tbBtnCount);
    GlobalObjects::iconfont->setPointSize(14);
    QHBoxLayout *poolEditHLayout=new QHBoxLayout();
    for(int i = 0; i < tbBtnCount; ++i)
    {
        QToolButton *tb = new QToolButton(danmulistPage);
        btns[i] = tb;
        tb->setFont(*GlobalObjects::iconfont);
        tb->setText(tbButtonTexts[i].first);
        tb->setToolTip(tbButtonTexts[i].second);
        tb->addActions(tbActions[i]);
        tb->setObjectName(QStringLiteral("ListEditButton"));
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setPopupMode(QToolButton::InstantPopup);
        poolEditHLayout->addWidget(tb);
#ifdef Q_OS_MACOS
        tb->setProperty("hideMenuIndicator", true);
#endif
    }
    poolEditHLayout->addStretch(1);
    danmulistPageVLayout->addLayout(poolEditHLayout);

    constexpr const int Index_UpdatePool = 0;
    constexpr const int Index_LocatePosition = 3;
    QObject::connect(btns[Index_UpdatePool], &QToolButton::clicked, this, &ListWindow::updateCurrentPool);
    QObject::connect(btns[Index_LocatePosition], &QToolButton::clicked, [this]() {
        danmulistView->scrollTo(GlobalObjects::danmuPool->getCurrentIndex(), QAbstractItemView::PositionAtTop);
    });

    return danmulistPage;
}

int ListWindow::updateCurrentPool()
{
    act_autoMatch->setEnabled(false);
    act_addOnlineDanmu->setEnabled(false);
    act_addLocalDanmu->setEnabled(false);
    const auto &sources =  GlobalObjects::danmuPool->getPool()->sources();
    int count=0;
    for(auto iter=sources.cbegin();iter!=sources.cend();++iter)
    {
        showMessage(tr("Updating: %1").arg(iter.value().title),NotifyMessageFlag::NM_PROCESS);
        count+=GlobalObjects::danmuPool->getPool()->update(iter.key());
    }
    showMessage(tr("Add %1 Danmu").arg(count), NotifyMessageFlag::NM_HIDE);
    act_autoMatch->setEnabled(true);
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
                if(fi.isFile() && "xml"==fi.suffix())
                {
					QVector<DanmuComment *> tmplist;
                    LocalProvider::LoadXmlDanmuFile(fi.filePath(),tmplist);
                    DanmuSource sourceInfo;
                    sourceInfo.title=fi.fileName();
                    sourceInfo.count=tmplist.count();
                    if(GlobalObjects::danmuPool->getPool()->addSource(sourceInfo,tmplist,true)==-1)
                    {
                        qDeleteAll(tmplist);
                        showMessage(tr("Add Failed: Pool is busy"), NotifyMessageFlag::NM_HIDE);
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
    QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
    const PlayListItem *curItem = GlobalObjects::playlist->setCurrentItem(model->mapToSource(index),playChild);
    if (curItem)
    {
        PlayContext::context()->playItem(curItem);
    }
}

void ListWindow::showMessage(const QString &msg, int flag, const QVariant &)
{
    infoTip->show();
    infoTip->setGeometry(0, height() - 60*logicalDpiY()/96, width(), 30*logicalDpiX()/96);
    static_cast<InfoTip *>(infoTip)->showMessage(msg,flag);
}

FilterBox::FilterBox(QWidget *parent) :
    QLineEdit(parent),
    patternGroup(new QActionGroup(this)),
    searchTypeGroup(new QActionGroup(this))
{
    setClearButtonEnabled(true);
    setFont(QFont(GlobalObjects::normalFont,12));
    connect(this, &QLineEdit::textChanged, this, &FilterBox::filterChanged);

    QMenu *menu = new QMenu(this);

    searchTypeGroup->setExclusive(true);
    QAction *searchTypeAction = menu->addAction(tr("Title"));
    searchTypeAction->setData(QVariant(int(SearchType::Title)));
    searchTypeAction->setCheckable(true);
    searchTypeAction->setChecked(true);
    searchTypeGroup->addAction(searchTypeAction);
    searchTypeAction = menu->addAction(tr("File Path"));
    searchTypeAction->setCheckable(true);
    searchTypeAction->setData(QVariant(int(SearchType::FilePath)));
    searchTypeGroup->addAction(searchTypeAction);
    connect(searchTypeGroup, &QActionGroup::triggered, this, &FilterBox::filterChanged);
    menu->addSeparator();

    actCaseSensitivity = menu->addAction(tr("Case Sensitive"));
    actCaseSensitivity->setCheckable(true);
    connect(actCaseSensitivity, &QAction::toggled, this, &FilterBox::filterChanged);

    menu->addSeparator();
    patternGroup->setExclusive(true);
    QAction *patternAction = menu->addAction(tr("Fixed String"));
    patternAction->setData(QVariant(int(QRegExp::FixedString)));
    patternAction->setCheckable(true);
    patternAction->setChecked(true);
    patternGroup->addAction(patternAction);
    patternAction = menu->addAction(tr("Regular Expression"));
    patternAction->setCheckable(true);
    patternAction->setData(QVariant(int(QRegExp::RegExp2)));
    patternGroup->addAction(patternAction);
    patternAction = menu->addAction(tr("Wildcard"));
    patternAction->setCheckable(true);
    patternAction->setData(QVariant(int(QRegExp::Wildcard)));
    patternGroup->addAction(patternAction);
    connect(patternGroup, &QActionGroup::triggered, this, &FilterBox::filterChanged);

    QToolButton *optionsButton = new QToolButton;

    optionsButton->setCursor(Qt::ArrowCursor);

    optionsButton->setFocusPolicy(Qt::NoFocus);
    optionsButton->setObjectName(QStringLiteral("FilterOptionButton"));
    GlobalObjects::iconfont->setPointSize(14);
    optionsButton->setFont(*GlobalObjects::iconfont);
    optionsButton->setText(QChar(0xe609));
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(optionsButton);
    addAction(optionsAction, QLineEdit::LeadingPosition);
}

AddUrlDialog::AddUrlDialog(QWidget *parent) : CFramelessDialog(tr("Add URL"), parent, true), decodeTitle(false)
{
    QLabel *tipLabel = new QLabel(tr("Enter URL(http, https, smb), separate multiple urls with line breaks"), this);
    edit = new QPlainTextEdit(this);
    newCollectionCheck = new QCheckBox(tr("Add to new collection"), this);
    collectionEdit = new QLineEdit(this);
    collectionEdit->setPlaceholderText(tr("Input Collection Name"));
    collectionEdit->setEnabled(false);
    QObject::connect(newCollectionCheck, &QCheckBox::stateChanged, collectionEdit, &QLineEdit::setEnabled);
    decodeTitleCheck = new QCheckBox(tr("Decode Title From URL"), this);

    QGridLayout *inputGLayout = new QGridLayout(this);
    inputGLayout->addWidget(tipLabel, 0, 0, 1, 2);
    inputGLayout->addWidget(edit, 1, 0, 1, 2);
    inputGLayout->addWidget(newCollectionCheck, 2, 0);
    inputGLayout->addWidget(collectionEdit, 2, 1);
    inputGLayout->addWidget(decodeTitleCheck, 3, 0, 1, 2);
    inputGLayout->setRowStretch(1, 1);
    inputGLayout->setColumnStretch(1, 1);
    inputGLayout->setContentsMargins(0, 0, 0, 0);

    setSizeSettingKey("AddURLDialog" ,QSize(200*logicalDpiX()/96, 60*logicalDpiY()/96));
}

void AddUrlDialog::onAccept()
{
    urls = edit->toPlainText().split("\n", Qt::SkipEmptyParts);
    if (urls.isEmpty())
    {
        showMessage(tr("URL can't be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    if (newCollectionCheck->isChecked())
    {
        collectionTitle = collectionEdit->text().trimmed();
        if (collectionTitle.isEmpty())
        {
            showMessage(tr("Collection Title can't be empty"), NM_ERROR | NM_HIDE);
            return;
        }
    }
    decodeTitle = decodeTitleCheck->isChecked();
    CFramelessDialog::onAccept();
}

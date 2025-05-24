#include "list.h"
#include <QHeaderView>
#include <QMenu>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QApplication>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QMessageBox>
#include <QStyle>

#include "Play/Subtitle/subitemdelegate.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/klineedit.h"
#include "UI/widgets/kplaintextedit.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "pooleditor.h"
#include "dialogs/adddanmu.h"
#include "matcheditor.h"
#include "dialogs/blockeditor.h"
#include "inputdialog.h"
#include "dlnadiscover.h"
#include "qlistview.h"
#include "widgets/loadingicon.h"
#include "Play/Danmu/Provider/localprovider.h"
#include "MediaLibrary/animeprovider.h"
#include "Play/playcontext.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "Play/Danmu/blocker.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Subtitle/subtitlemodel.h"
#include "Download/downloadmodel.h"
#include "widgets/lazycontainer.h"
#include "dialogs/subrecognizedialog.h"

namespace
{
    class PlayListItemDelegate: public QStyledItemDelegate
    {
    public:
        explicit PlayListItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) { }

        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            KLineEdit *editor = new KLineEdit(parent);
            QFont f(parent->font());
            f.setPointSize(11);
            editor->setFont(f);
            return editor;
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QStyleOptionViewItem viewOption(option);
            initStyleOption(&viewOption, index);
            QColor itemForegroundColor = index.data(Qt::ForegroundRole).value<QBrush>().color();
            if (itemForegroundColor.isValid())
            {
                if (itemForegroundColor != option.palette.color(QPalette::WindowText))
                {
                    viewOption.palette.setColor(QPalette::HighlightedText, itemForegroundColor);
                }

            }

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QRect itemRect = viewOption.rect;
            itemRect.setLeft(0);
            if (viewOption.state & QStyle::State_Selected)
            {
                if (viewOption.state & QStyle::State_MouseOver)
                {
                    painter->fillRect(itemRect, QColor(255, 255, 255, 40));
                }
                else
                {
                    painter->fillRect(itemRect, QColor(255, 255, 255, 40));
                }
            }
            else
            {
                if (viewOption.state & QStyle::State_MouseOver)
                {
                    painter->fillRect(itemRect, QColor(255, 255, 255, 40));
                }
            }
            painter->restore();


            static QIcon bgmCollectionIcon(":/res/images/playlist-bgm.svg");
            static QIcon folderIcon(":/res/images/playlist-folder.svg");
            static QIcon webdavIcon(":/res/images/playlist-web-folder.svg");
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
            if (index.data(PlayList::ItemRole::BgmCollectionRole).toBool())
            {
                marker = &bgmCollectionIcon;

            }
            else if (index.data(PlayList::ItemRole::FolderCollectionRole).toBool())
            {
                marker = &folderIcon;
            }
            else if (index.data(PlayList::ItemRole::WebDAVCollectionRole).toBool())
            {
                marker = &webdavIcon;
            }

            int markerWidth = 16;
            QStyle *style = viewOption.widget ? viewOption.widget->style() : QApplication::style();
            QRect iconRect = style->subElementRect(QStyle::SubElement::SE_ItemViewItemDecoration, &viewOption, viewOption.widget);
            QRect textRect = style->subElementRect(QStyle::SubElement::SE_ItemViewItemText, &viewOption, viewOption.widget);
            if (marker)
            {
                QRect markerRect = textRect;
                markerRect.setLeft(markerRect.right() - markerWidth - 4);
                markerRect.setRight(markerRect.right() - 4);
                marker->paint(painter, markerRect);
                textRect.setRight(markerRect.left());
            }

            PlayListItem::Marker colorMarker = (PlayListItem::Marker)index.data(PlayList::ItemRole::ColorMarkerRole).toInt();
            if (colorMarker != PlayListItem::Marker::M_NONE)
            {
                QRect markerRect = textRect;
                int right = markerRect.right() - 4;
                int left = markerRect.right() - markerWidth - 4;

                markerRect.setLeft(left);
                markerRect.setRight(right);
                textRect.setRight(left);
                colorMarkerIcon[colorMarker].paint(painter, markerRect);
            }


            painter->save();
            painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

            if (!viewOption.icon.isNull())
            {
                QIcon::Mode mode = QIcon::Normal;
                QIcon::State state = viewOption.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
                viewOption.icon.paint(painter, iconRect, viewOption.decorationAlignment, mode, state);
            }
            if (!viewOption.text.isEmpty())
            {
                textRect.adjust(4, 0, -4, 0);
                static QFont textFont(GlobalObjects::normalFont, 11);
                painter->setFont(textFont);
                if (painter->fontMetrics().horizontalAdvance(viewOption.text) > textRect.width())
                {
                    viewOption.text = painter->fontMetrics().elidedText(viewOption.text, Qt::ElideRight, textRect.width());
                }
                painter->setPen(viewOption.palette.color(QPalette::HighlightedText));
                painter->drawText(textRect, viewOption.displayAlignment, viewOption.text);
            }
            painter->restore();

        }
    };
    class TextColorDelegate: public QStyledItemDelegate
    {
    public:
        explicit TextColorDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent)
        { }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            QStyleOptionViewItem viewOption(option);
            initStyleOption(&viewOption, index);
            QColor itemForegroundColor = index.data(Qt::ForegroundRole).value<QBrush>().color();
            if (itemForegroundColor.isValid())
            {
                if (itemForegroundColor != option.palette.color(QPalette::WindowText))
                    viewOption.palette.setColor(QPalette::HighlightedText, itemForegroundColor);

            }

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QRect itemRect = viewOption.rect;
            itemRect.setLeft(0);
            if (viewOption.state & QStyle::State_Selected)
            {
                if (viewOption.state & QStyle::State_MouseOver)
                {
                    painter->fillRect(itemRect, QColor(255, 255, 255, 40));
                }
                else
                {
                    painter->fillRect(itemRect, QColor(255, 255, 255, 40));
                }
            }
            else
            {
                if (viewOption.state & QStyle::State_MouseOver)
                {
                    painter->fillRect(itemRect, QColor(255, 255, 255, 40));
                }
            }
            painter->restore();

            if (index.column() == 1)
            {
                const QString format = index.parent().isValid() ? "  %1 %2" : "%1 %2";
                viewOption.text = format.arg(index.siblingAtColumn(0).data().toString(), index.data().toString());
            }
            QStyle *style = viewOption.widget ? viewOption.widget->style() : QApplication::style();
            style->drawControl(QStyle::CE_ItemViewItem, &viewOption, painter, viewOption.widget);
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
            cancelBtn = new KPushButton(QObject::tr("Cancel"), this);
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
            if (hideTimer.isActive()) hideTimer.stop();
            if (flag & NotifyMessageFlag::NM_HIDE)
            {
                hideTimer.setSingleShot(true);
                hideTimer.start(3000);
            }
            if (flag & NotifyMessageFlag::NM_PROCESS)
                loadingIcon->show();
            else
                loadingIcon->hide();
            if (flag & NotifyMessageFlag::NM_SHOWCANCEL)
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
    initListUI();
    updatePlaylistActions();
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);

    QObject::connect(GlobalObjects::playlist,&PlayList::matchStatusChanged, this, [this](bool on){
        matchStatus+=(on?1:-1);
        if(matchStatus==0)
        {
            actionDisable=false;
            updatePlaylistActions();
            playlistView->setDragEnabled(true);
            act_addCollection->setEnabled(true);
            act_addFolder->setEnabled(true);
            act_addWebDAVCollection->setEnabled(true);
            act_addItem->setEnabled(true);
            act_addURL->setEnabled(true);
        }
        else
        {
            actionDisable=true;
            updatePlaylistActions();
            act_addCollection->setEnabled(false);
            act_addFolder->setEnabled(false);
            act_addWebDAVCollection->setEnabled(false);
            act_addItem->setEnabled(false);
            act_addURL->setEnabled(false);
            playlistView->setDragEnabled(false);
        }
    });
}

void ListWindow::initActions()
{
    act_play = new QAction(tr("Play"),this);
    QObject::connect(act_play, &QAction::triggered, this, [this](){
        QModelIndexList selection = playlistView->selectionModel()->selectedIndexes();
        QModelIndex index = selection.size() == 0 ? QModelIndex() : selection.last();
        playItem(index);
    });
    act_autoMatch=new QAction(tr("Associate Danmu Pool"),this);
    QObject::connect(act_autoMatch, &QAction::triggered, this, [=]() {matchPool(); });
    act_removeMatch=new QAction(tr("Remove Match"),this);
    QObject::connect(act_removeMatch, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndexList indexes(selection.indexes());
        GlobalObjects::playlist->removeMatch(indexes);
    });

    act_markBgmCollection=new QAction(tr("Mark/Unmark Bangumi Collecion"),this);
    QObject::connect(act_markBgmCollection, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndex selIndex(selection.indexes().first());
        GlobalObjects::playlist->switchBgmCollection(selIndex);
    });

    act_updateFolder = new QAction(tr("Scan Folder/WebDAV Collection Changes"),this);
    QObject::connect(act_updateFolder, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0) return;
        QModelIndex selIndex(selection.indexes().first());
        const PlayListItem *item = GlobalObjects::playlist->getItem(selIndex);
        if (!item || !item->isCollection()) return;
        if (item->isWebDAVCollection())
        {
            GlobalObjects::playlist->refreshWebDAVCollection(selIndex);
        }
        else
        {
            GlobalObjects::playlist->refreshFolder(selIndex);
        }
    });

    act_addWebDanmuSource=new QAction(tr("Add Web Danmu Source"),this);
    QObject::connect(act_addWebDanmuSource, &QAction::triggered, this, [this](){
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
        if (QDialog::Accepted == addDanmuDialog.exec())
        {
            auto &infoList = addDanmuDialog.danmuInfoList;
            for (SearchDanmuInfo &info : infoList)
            {
                Pool *pool = GlobalObjects::danmuManager->getPool(poolIdMap.value(info.pool));
                if (pool)
                {
                    showMessage(tr("Adding: %1").arg(pool->epTitle()), NotifyMessageFlag::NM_PROCESS);
                    if (pool->addSource(info.src, info.danmus, true) == -1)
                    {
                        qDeleteAll(info.danmus);
                    }
                }
                else
                {
                    qDeleteAll(info.danmus);
                }
            }
            showMessage(tr("Done"), NotifyMessageFlag::NM_HIDE);
        }

    });
    act_addLocalDanmuSource=new QAction(tr("Add Local Danmu Source"),this);
    QObject::connect(act_addLocalDanmuSource, &QAction::triggered, this, [this](){
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
        if (GlobalObjects::mpvplayer->needPause())
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        const QString dialogPath = item->path.isEmpty() ? "" : QFileInfo(item->path).absolutePath();
        QStringList files = QFileDialog::getOpenFileNames(this,tr("Select Xml File"), dialogPath, "Xml File(*.xml) ");
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
    QObject::connect(act_updateDanmu, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndexList indexes(selection.indexes());
        actionDisable=true;
        updatePlaylistActions();
        act_addCollection->setEnabled(false);
        act_addFolder->setEnabled(false);
        act_addWebDAVCollection->setEnabled(false);
        act_addItem->setEnabled(false);
        act_addURL->setEnabled(false);
        playlistView->setDragEnabled(false);
        GlobalObjects::playlist->updateItemsDanmu(indexes);
        actionDisable=false;
        updatePlaylistActions();
        playlistView->setDragEnabled(true);
        act_addCollection->setEnabled(true);
        act_addFolder->setEnabled(true);
        act_addWebDAVCollection->setEnabled(true);
        act_addItem->setEnabled(true);
        act_addURL->setEnabled(true);
    });
    act_exportDanmu=new QAction(tr("Export Danmu"),this);
    QObject::connect(act_exportDanmu, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QModelIndexList indexes(selection.indexes());
        actionDisable=true;
        updatePlaylistActions();
        act_addCollection->setEnabled(false);
        act_addFolder->setEnabled(false);
        act_addWebDAVCollection->setEnabled(false);
        act_addItem->setEnabled(false);
        act_addURL->setEnabled(false);
        playlistView->setDragEnabled(false);
        GlobalObjects::playlist->exportDanmuItems(indexes);
        actionDisable=false;
        updatePlaylistActions();
        playlistView->setDragEnabled(true);
        act_addCollection->setEnabled(true);
        act_addFolder->setEnabled(true);
        act_addWebDAVCollection->setEnabled(true);
        act_addItem->setEnabled(true);
        act_addURL->setEnabled(true);
    });

    act_shareResourceCode=new QAction(tr("Resource Code"),this);
    QObject::connect(act_shareResourceCode, &QAction::triggered, this, [this](){
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
    QObject::connect(act_sharePoolCode, &QAction::triggered, this, [this](){
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
    QObject::connect(act_addCollection, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QModelIndex newIndex = model->mapFromSource(GlobalObjects::playlist->addCollection(getPSParentIndex(),QObject::tr("new collection")));
        playlistView->scrollTo(newIndex, QAbstractItemView::PositionAtCenter);
        playlistView->edit(newIndex);
    });
    act_addItem = new QAction(tr("Add Item"),this);
    QObject::connect(act_addItem, &QAction::triggered, this, [this](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->needPause())
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
    QObject::connect(act_addURL, &QAction::triggered, this, [this](){
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
    QObject::connect(act_addFolder, &QAction::triggered, this, [this](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->needPause())
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

    act_addWebDAVCollection = new QAction(tr("Add WebDAV Collection"), this);
    QObject::connect(act_addWebDAVCollection, &QAction::triggered, this, [this](){
        AddWebDAVCollectionDialog dialog(this);
        if (QDialog::Accepted != dialog.exec()) return;
        QModelIndex parent = getPSParentIndex();
        parent = GlobalObjects::playlist->addWebDAVCollection(parent, dialog.title, dialog.url, dialog.user, dialog.password);
        GlobalObjects::playlist->refreshWebDAVCollection(parent);
    });

    act_PlayOnOtherDevices = new QAction(tr("Play on other devices"), this);
    QObject::connect(act_PlayOnOtherDevices, &QAction::triggered, this, [this](){
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
    QObject::connect(act_cut, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        GlobalObjects::playlist->cutItems(selection.indexes());
    });
    act_paste=new QAction(tr("Paste"),this);
    act_paste->setShortcut(QString("Ctrl+V"));
    QObject::connect(act_paste, &QAction::triggered, this, [this](){
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
    QObject::connect(act_moveDown, &QAction::triggered, this, [this](){
        QModelIndexList selection = playlistView->selectionModel()->selectedIndexes();
        if(selection.size()==0)return;
        QModelIndex index(selection.last());
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        GlobalObjects::playlist->moveUpDown(model->mapToSource(index),false);
    });
    act_merge=new QAction(tr("Merge"),this);
    QObject::connect(act_merge, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if(selection.size()==0)return;
        QModelIndex newIndex = model->mapFromSource(GlobalObjects::playlist->mergeItems(selection.indexes()));
        playlistView->scrollTo(newIndex, QAbstractItemView::PositionAtCenter);
        playlistView->edit(newIndex);
    });

    act_remove=new QAction(tr("Remove"),this);
    act_remove->setShortcut(QKeySequence::Delete);
    QObject::connect(act_remove, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        GlobalObjects::playlist->deleteItems(selection.indexes());
    });

    act_removeInvalid=new QAction(tr("Remove Invalid Items"),this);
    QObject::connect(act_removeInvalid, &QAction::triggered, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QItemSelection selection = model->mapSelectionToSource(playlistView->selectionModel()->selection());
        if (selection.size() == 0)return;
        showMessage(tr("Remove %1 Invalid Item(s)").arg(GlobalObjects::playlist->deleteInvalidItems(selection.indexes())), NM_HIDE);
    });

    act_clear=new QAction(tr("Clear"),this);
    QObject::connect(act_clear, &QAction::triggered, this, [this](){
       QMessageBox::StandardButton btn =QMessageBox::question(this,tr("Clear"),tr("Are you sure to clear the list ?"),QMessageBox::Yes|QMessageBox::No);
       if(btn==QMessageBox::Yes)
       {
           GlobalObjects::playlist->clear();
       }
    });

    act_browseFile=new QAction(tr("Browse File"),this);
    QObject::connect(act_browseFile, &QAction::triggered, this, [this](){
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
    QObject::connect(act_sortSelectionAscending, &QAction::triggered, this, [this](){
        sortSelection(false,true);
    });
    act_sortSelectionDescending = new QAction(tr("Sort Descending"),this);
    QObject::connect(act_sortSelectionDescending, &QAction::triggered, this, [this](){
        sortSelection(false,false);
    });
    act_sortAllAscending = new QAction(tr("Sort All Ascending"),this);
    QObject::connect(act_sortAllAscending, &QAction::triggered, this, [this](){
        sortSelection(true,true);
    });
    act_sortAllDescending = new QAction(tr("Sort All Descending"),this);
    QObject::connect(act_sortAllDescending, &QAction::triggered, this, [this](){
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
    QObject::connect(loopModeGroup, &QActionGroup::triggered, this, [this](QAction *action){
        GlobalObjects::playlist->setLoopMode(static_cast<PlayList::LoopMode>(action->data().toInt()));
        GlobalObjects::appSetting->setValue("List/LoopMode",loopModeGroup->actions().indexOf(loopModeGroup->checkedAction()));
    });
    loopModeGroup->actions().at(GlobalObjects::appSetting->value("List/LoopMode",1).toInt())->trigger();

    act_addOnlineDanmu=new QAction(tr("Add Online Danmu"),this);
    QObject::connect(act_addOnlineDanmu, &QAction::triggered, this, [this](){
        const PlayListItem *currentItem=GlobalObjects::playlist->getCurrentItem();
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->needPause())
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        AddDanmu addDanmuDialog(currentItem, this);
        if (QDialog::Accepted == addDanmuDialog.exec())
        {
            auto &infoList = addDanmuDialog.danmuInfoList;
            Pool *pool = GlobalObjects::danmuPool->getPool();
            for (SearchDanmuInfo &info : infoList)
            {
                if (pool->addSource(info.src, info.danmus, true) == -1)
                {
                    qDeleteAll(info.danmus);
                }
            }
        }
        if (restorePlayState) GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });
    act_addLocalDanmu=new QAction(tr("Add Local Danmu"),this);
    QObject::connect(act_addLocalDanmu, &QAction::triggered, this, [this](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->needPause())
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
    QObject::connect(act_editBlock, &QAction::triggered, this, [this](){
        BlockEditor blockEditor(this);
        blockEditor.exec();
    });
    act_editPool=new QAction(tr("Edit Danmu Pool"),this);
    QObject::connect(act_editPool, &QAction::triggered, this, [this](){
        static PoolEditor *poolEditor = nullptr;
        if (!poolEditor)
        {
            poolEditor = new PoolEditor(this);
            QObject::connect(poolEditor, &PoolEditor::finished, poolEditor, &PoolEditor::deleteLater);
            QObject::connect(poolEditor, &PoolEditor::destroyed, [](){
               poolEditor = nullptr;
            });
        }
        poolEditor->show();
    });
    act_copyDanmuText=new QAction(tr("Copy Danmu Text"),this);
    QObject::connect(act_copyDanmuText, &QAction::triggered, this, [this](){
        QClipboard *cb = QApplication::clipboard();
        cb->setText(getSelectedDanmu()->text);
    });
    act_copyDanmuColor=new QAction(tr("Copy Danmu Color"),this);
    QObject::connect(act_copyDanmuColor, &QAction::triggered, this, [this](){
        QClipboard *cb = QApplication::clipboard();
        cb->setText(QString::number(getSelectedDanmu()->color,16));
    });
    act_copyDanmuSender=new QAction(tr("Copy Danmu Sender"),this);
    QObject::connect(act_copyDanmuSender, &QAction::triggered, this, [this](){
        QClipboard *cb = QApplication::clipboard();
        cb->setText(getSelectedDanmu()->sender);
    });
    act_blockText=new QAction(tr("Block Text"),this);
    QObject::connect(act_blockText, &QAction::triggered, this, [this](){
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
    QObject::connect(act_blockColor, &QAction::triggered, this, [this](){
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
    QObject::connect(act_blockSender, &QAction::triggered, this, [this](){
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
    QObject::connect(act_deleteDanmu, &QAction::triggered, this, [this](){
        GlobalObjects::danmuPool->deleteDanmu(getSelectedDanmu());
    });
    act_jumpToTime=new QAction(tr("Jump to"),this);
    QObject::connect(act_jumpToTime, &QAction::triggered, this, [this](){
        MPVPlayer::PlayState state=GlobalObjects::mpvplayer->getState();
        if(state==MPVPlayer::PlayState::Play || state==MPVPlayer::PlayState::Pause)
            GlobalObjects::mpvplayer->seek(getSelectedDanmu()->time);
    });

    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::fileChanged, this, [this](){
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        playlistView->scrollTo(model->mapFromSource(GlobalObjects::playlist->getCurrentIndex()), QAbstractItemView::EnsureVisible);
    });

    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::triggerAdd, act_addOnlineDanmu, &QAction::trigger);
    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::triggerEditPool, act_editPool, &QAction::trigger);
    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::triggerEditBlockRules, act_editBlock, &QAction::trigger);

    act_addSub = new QAction(tr("Add Subtitle"), this);
    QObject::connect(act_addSub, &QAction::triggered, this, [=](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        QString dialogPath;
        if (!GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
        {
            dialogPath = QFileInfo(GlobalObjects::mpvplayer->getCurrentFile()).absolutePath();
        }
        QString file(QFileDialog::getOpenFileName(this, tr("Select Sub File"), dialogPath ,tr("Subtitle (%0);;All Files(*)").arg(GlobalObjects::mpvplayer->subtitleFormats.join(" "))));
        if (!file.isEmpty())
        {
            GlobalObjects::mpvplayer->addSubtitle(file);
        }
        if (restorePlayState) GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });

    act_saveSub = new QAction(tr("Save Subtitle"), this);
    QObject::connect(act_saveSub, &QAction::triggered, this, [=](){
        bool restorePlayState = false;
        if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
        {
            restorePlayState = true;
            GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        }
        if (subModel->curSub().format == SubFileFormat::F_ASS)
        {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save Subtitle"),"", tr("ASS Sub (*.ass);;SRT Sub (*.srt)"));
            if (!fileName.isEmpty())
            {
                if (fileName.endsWith(".ass"))
                {
                    QFile subFile(fileName);
                    if (subFile.open(QIODevice::WriteOnly|QIODevice::Text))
                    {
                        subFile.write(subModel->curSub().rawData.toUtf8());
                    }
                }
                else
                {
                    subModel->saveCurSRTSub(fileName);
                }
            }
        }
        else
        {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save Subtitle"),"", tr("SRT Sub (*.srt)"));
            if (!fileName.isEmpty())
            {
                subModel->saveCurSRTSub(fileName);
            }
        }
        if (restorePlayState) GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    });

    act_copySubTime = new QAction(tr("Copy Subtitle Time"), this);
    QObject::connect(act_copySubTime, &QAction::triggered, this, [=](){
        QItemSelection selection = sublistProxyModel->mapSelectionToSource(sublistView->selectionModel()->selection());
        if (selection.empty()) return;
        QModelIndex index = selection.indexes().first();
        QString timeStart = index.siblingAtColumn((int)SubtitleModel::Columns::START).data(Qt::DisplayRole).toString();
        QString timeEnd = index.siblingAtColumn((int)SubtitleModel::Columns::START).data(Qt::DisplayRole).toString();
        QString subTime = QString("%1 -> %2").arg(timeStart, timeEnd);
        QClipboard *cb = QApplication::clipboard();
        cb->setText(subTime);
    });

    act_copySubText = new QAction(tr("Copy Subtitle Text"), this);
    QObject::connect(act_copySubText, &QAction::triggered, this, [=](){
        QItemSelection selection = sublistProxyModel->mapSelectionToSource(sublistView->selectionModel()->selection());
        if (selection.empty()) return;
        QModelIndex index = selection.indexes().first();
        QString subText = index.data(Qt::DisplayRole).toString();
        QClipboard *cb = QApplication::clipboard();
        cb->setText(subText);
    });

    act_subRecognize = new QAction(tr("Subtitle Recognition"), this);
    QObject::connect(act_subRecognize, &QAction::triggered, this, [=](){
        const QString curFile = PlayContext::context()->path;
        if (curFile.isEmpty() || !QFileInfo::exists(curFile)) return;
        SubRecognizeDialog dialog(curFile, this);
        dialog.exec();
    });

    act_subTranslation = new QAction(tr("Subtitle Translation"), this);
    QObject::connect(act_subTranslation, &QAction::triggered, this, [=](){
        const QString curFile = PlayContext::context()->path;
        if (curFile.isEmpty() || !QFileInfo::exists(curFile)) return;
        if (!subModel->hasSub())
        {
            showMessage(tr("Current Video has no subtitle"), NM_HIDE);
            return;
        }
        SubRecognizeEditDialog dialog(subModel->curSub(), curFile, true, this);
        dialog.exec();
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
    return GlobalObjects::danmuPool->getDanmu(danmulistProxyModel->mapToSource(selection.last()));
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

    } else {
        GlobalObjects::playlist->matchItems(indexes);
    }
}

void ListWindow::initListUI()
{
    QGridLayout *listGLayout = new QGridLayout(this);
    listGLayout->setContentsMargins(0,0,0,0);
    listGLayout->setSpacing(0);

    QStackedLayout *titleSLayout = new QStackedLayout;
    titleContainer = new QWidget(this);
    titleContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QHBoxLayout *titleHLayout = new QHBoxLayout(titleContainer);
    titleHLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *listTitleLabel = new QLabel(tr("PlayList"), titleContainer);
    listTitleLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    listTitleLabel->setObjectName(QStringLiteral("ListTitleLabel"));
    QFont titleFont;
    titleFont.setFamily(GlobalObjects::normalFont);
    titleFont.setPointSize(12);
    listTitleLabel->setFont(titleFont);

    QPushButton *searchButton = new QPushButton(titleContainer);
    searchButton->setObjectName(QStringLiteral("ListSearchButton"));
    GlobalObjects::iconfont->setPointSize(14);
    searchButton->setFont(*GlobalObjects::iconfont);
    searchButton->setText(QChar(0xea8a));

    titleHLayout->addSpacing(8);
    titleHLayout->addWidget(listTitleLabel);
    titleHLayout->addStretch(1);
    titleHLayout->addWidget(searchButton);
    titleHLayout->addSpacing(8);

    QWidget *filterContainer = new QWidget(this);
    QHBoxLayout *filterHLayout = new QHBoxLayout(filterContainer);
    filterHLayout->setContentsMargins(8, 8, 8, 8);

    filterEdit = new KLineEdit(filterContainer);
    filterHLayout->addWidget(filterEdit);
    filterEdit->setFont(QFont(GlobalObjects::normalFont, 12));
    filterEdit->setObjectName(QStringLiteral("ListFilterLineEdit"));
    QMargins textMargins = filterEdit->textMargins();
    textMargins.setLeft(6);
    filterEdit->setTextMargins(textMargins);
    QPushButton *clearBtn = new QPushButton(filterEdit);
    clearBtn->setObjectName(QStringLiteral("ListSearchButton"));
    GlobalObjects::iconfont->setPointSize(12);
    clearBtn->setFont(*GlobalObjects::iconfont);
    clearBtn->setText(QChar(0xe60b));
    clearBtn->setCursor(Qt::ArrowCursor);
    clearBtn->setFocusPolicy(Qt::NoFocus);
    QWidgetAction *clearAction = new QWidgetAction(this);
    clearAction->setDefaultWidget(clearBtn);
    filterEdit->addAction(clearAction, QLineEdit::TrailingPosition);

    QObject::connect(searchButton, &QPushButton::clicked, this, [=](){titleSLayout->setCurrentIndex(1);});
    QObject::connect(clearBtn, &QPushButton::clicked, this, [=](){filterEdit->clear(); titleSLayout->setCurrentIndex(0);});
    QObject::connect(filterEdit, &QLineEdit::textChanged, this, [=](const QString &text){
        if (currentProxyModel)
        {
            QRegularExpression regExp(text, QRegularExpression::CaseInsensitiveOption);
            currentProxyModel->setFilterRole(Qt::DisplayRole);
            currentProxyModel->setFilterRegularExpression(regExp);
        }
    });

    titleSLayout->addWidget(titleContainer);
    titleSLayout->addWidget(filterContainer);

    contentStackLayout = new QStackedLayout();
    contentStackLayout->setContentsMargins(0,0,0,0);
    contentStackLayout->addWidget(initPlaylistPage());
    contentStackLayout->addWidget(new LazyContainer(this, contentStackLayout, [this](){return this->initDanmulistPage();}));
    contentStackLayout->addWidget(new LazyContainer(this, contentStackLayout, [this](){return this->initSublistPage();}));

    QObject::connect(contentStackLayout, &QStackedLayout::currentChanged, this, [=](int index){
        const QStringList listNames{tr("PlayList"), tr("Danmu"), tr("Subtitle")};
        if (index >= 0 && index < listNames.size())
        {
            listTitleLabel->setText(listNames[index]);
            titleSLayout->setCurrentIndex(0);
        }
    });

    listGLayout->addLayout(titleSLayout, 0, 0);
    listGLayout->addLayout(contentStackLayout, 1, 0);
    listGLayout->setRowStretch(1, 1);

    infoTip = new InfoTip(this);
    infoTip->hide();
}

void ListWindow::updatePlaylistActions()
{
    if (actionDisable)
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
    act_PlayOnOtherDevices->setEnabled(hasPlaylistSelection);
    act_sharePoolCode->setEnabled(hasPlaylistSelection);
    act_shareResourceCode->setEnabled(hasPlaylistSelection);
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

QWidget *ListWindow::initPlaylistPage()
{
    QWidget *playlistPage = new QWidget(this);
    playlistPage->setObjectName(QStringLiteral("playlistPage"));

    QVBoxLayout *playlistPageVLayout = new QVBoxLayout(playlistPage);
    playlistPageVLayout->setContentsMargins(0,0,0,0);
    playlistPageVLayout->setSpacing(0);

    playlistView = new QTreeView(playlistPage);
    playlistView->setObjectName(QStringLiteral("playlist"));
    playlistView->setAnimated(true);
    playlistView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    playlistView->setEditTriggers(QAbstractItemView::SelectedClicked);
    playlistView->setDragEnabled(true);
    playlistView->setAcceptDrops(true);
    playlistView->setDragDropMode(QAbstractItemView::InternalMove);
    playlistView->setDropIndicatorShown(true);
    playlistView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    playlistView->header()->hide();
    playlistView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    playlistView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    playlistView->setFont(QFont(GlobalObjects::normalFont, 16));
    playlistView->setContextMenuPolicy(Qt::CustomContextMenu);
    playlistView->setIndentation(12);
    playlistView->setItemDelegate(new PlayListItemDelegate(this));
    new FloatScrollBar(playlistView->verticalScrollBar(), playlistView);

    playlistProxyModel = new QSortFilterProxyModel(this);
    playlistProxyModel->setRecursiveFilteringEnabled(true);
    playlistProxyModel->setSourceModel(GlobalObjects::playlist);
    playlistView->setModel(playlistProxyModel);
    currentProxyModel = playlistProxyModel;

    QObject::connect(playlistView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(playlistView->model());
        QModelIndex itemIndex = model->mapToSource(index);
        const PlayListItem *item = GlobalObjects::playlist->getItem(itemIndex);
        if (item && item->isWebDAVCollection() && item->children->empty())
        {
            GlobalObjects::playlist->refreshWebDAVCollection(itemIndex);
        }
        else
        {
            playItem(index, false);
        }
    });
    QObject::connect(playlistView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ListWindow::updatePlaylistActions);

    QMenu *playlistContextMenu = new ElaMenu(playlistView);
    playlistContextMenu->addAction(act_play);

    QMenu *addSubMenu = new ElaMenu(tr("Add"),playlistContextMenu);
    addSubMenu->addAction(act_addCollection);
    addSubMenu->addAction(act_addItem);
    addSubMenu->addAction(act_addURL);
    addSubMenu->addAction(act_addFolder);
    addSubMenu->addAction(act_addWebDAVCollection);
    playlistContextMenu->addMenu(addSubMenu);

    matchSubMenu = new ElaMenu(tr("Match"),playlistContextMenu);
    matchSubMenu->addAction(act_autoMatch);
    matchSubMenu->addAction(act_removeMatch);

    QAction *matchSep = new QAction(this);
    matchSep->setSeparator(true);
    playlistContextMenu->addMenu(matchSubMenu);
    QActionGroup *matchCheckGroup = new QActionGroup(this);
    auto matchProviders = GlobalObjects::animeProvider->getMatchProviders();
    static QList<QAction *> matchActions;
    if (matchProviders.count() > 0)
    {
        matchSubMenu->addAction(matchSep);
        QString defaultSctiptId = GlobalObjects::animeProvider->defaultMatchScript();
        for (const auto &p : matchProviders)
        {
            QAction *mAct = new QAction(p.first);
            mAct->setData(p.second); //id
            matchActions.append(mAct);
            matchSubMenu->addAction(mAct);
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
    QObject::connect(matchCheckGroup, &QActionGroup::triggered, this, [](QAction *act){
        QString scriptId = act->data().toString();
        if(!scriptId.isEmpty())
        {
            GlobalObjects::animeProvider->setDefaultMatchScript(scriptId);
        }
    });
    QObject::connect(GlobalObjects::animeProvider, &AnimeProvider::matchProviderChanged, this, [=](){
         auto matchProviders = GlobalObjects::animeProvider->getMatchProviders();
         for (QAction *mAct : matchActions)
         {
             matchSubMenu->removeAction(mAct);
         }
         qDeleteAll(matchActions);
         matchActions.clear();
         matchSubMenu->removeAction(matchSep);

         if (matchProviders.count() > 0)
         {
             QString defaultSctiptId = GlobalObjects::animeProvider->defaultMatchScript();
             matchSubMenu->addAction(matchSep);
             for (const auto &p : matchProviders)
             {
                 QAction *mAct = new QAction(p.first);
                 mAct->setData(p.second); //id
                 matchActions.append(mAct);
                 matchSubMenu->addAction(mAct);
             }
         }
    });

    QMenu *danmuSubMenu = new ElaMenu(tr("Danmu"),playlistContextMenu);
    danmuSubMenu->addAction(act_addWebDanmuSource);
    danmuSubMenu->addAction(act_addLocalDanmuSource);
    danmuSubMenu->addAction(act_updateDanmu);
    danmuSubMenu->addAction(act_exportDanmu);
    playlistContextMenu->addMenu(danmuSubMenu);
    playlistContextMenu->addSeparator();

    QMenu *editSubMenu = new ElaMenu(tr("Edit"),playlistContextMenu);
    editSubMenu->addAction(act_merge);
    editSubMenu->addAction(act_cut);
    editSubMenu->addAction(act_paste);
    playlistView->addAction(act_cut);
    playlistView->addAction(act_paste);
    playlistContextMenu->addMenu(editSubMenu);

    markSubMenu = new ElaMenu(tr("Mark"),playlistContextMenu);
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
    for (int i = 0; i<=PlayListItem::Marker::M_NONE; ++i)
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
    playlistView->addAction(act_remove);

    QMenu *moveSubMenu = new ElaMenu(tr("Move"),playlistContextMenu);
    moveSubMenu->addAction(act_moveUp);
    moveSubMenu->addAction(act_moveDown);
    playlistContextMenu->addMenu(moveSubMenu);
    QMenu *sortSubMenu = new ElaMenu(tr("Sort"),playlistContextMenu);
    sortSubMenu->addAction(act_sortSelectionAscending);
    sortSubMenu->addAction(act_sortSelectionDescending);
    playlistContextMenu->addMenu(sortSubMenu);
    playlistContextMenu->addSeparator();

    QMenu *shareSubMenu = new ElaMenu(tr("Share"),playlistContextMenu);
    shareSubMenu->addAction(act_sharePoolCode);
    shareSubMenu->addAction(act_shareResourceCode);
    playlistContextMenu->addMenu(shareSubMenu);
    playlistContextMenu->addAction(act_PlayOnOtherDevices);
    playlistContextMenu->addSeparator();
    playlistContextMenu->addAction(act_updateFolder);
    playlistContextMenu->addAction(act_removeInvalid);
    playlistContextMenu->addAction(act_browseFile);

    QObject::connect(playlistView, &QTreeView::customContextMenuRequested, playlistView, [=](){
        playlistContextMenu->exec(QCursor::pos());
    });

    playlistPageVLayout->addWidget(playlistView);

    constexpr const int tbBtnCount = 2;
    QPair<QChar, QString> tbButtonTexts[tbBtnCount] = {
        {QChar(0xe667), tr("Add")},
        {QChar(0xe64c), tr("Loop Mode")}
    };
    QList<QAction *> tbActions[tbBtnCount] = {
        {act_addCollection, act_addItem, act_addURL, act_addFolder, act_addWebDAVCollection},
        {loopModeGroup->actions()}
    };

    GlobalObjects::iconfont->setPointSize(14);
    QHBoxLayout *listEditHLayout = new QHBoxLayout();
    listEditHLayout->setContentsMargins(4, 8, 4, 8);

    for (int i = 0; i < tbBtnCount; ++i)
    {
        QToolButton *tb = new QToolButton(playlistPage);
        tb->setFont(*GlobalObjects::iconfont);
        tb->setText(tbButtonTexts[i].first);
        tb->setToolTip(tbButtonTexts[i].second);
        tb->setObjectName(QStringLiteral("ListEditButton"));
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setPopupMode(QToolButton::InstantPopup);
        QMenu *popupMenu = new ElaMenu(tb);
        popupMenu->addActions(tbActions[i]);
        tb->setMenu(popupMenu);
        listEditHLayout->addWidget(tb);
    }
    listEditHLayout->addStretch(1);
    playlistPageVLayout->addLayout(listEditHLayout);

    return playlistPage;
}

QWidget *ListWindow::initDanmulistPage()
{
    QWidget *danmulistPage = new QWidget(this);
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
    danmulistView->header()->hide();
    danmulistView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    danmulistView->setItemDelegate(new TextColorDelegate(this));
    new FloatScrollBar(danmulistView->verticalScrollBar(), danmulistView);

    QMenu *danmulistContextMenu = new ElaMenu(danmulistView);

    danmulistView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(danmulistView, &QTreeView::customContextMenuRequested, danmulistView, [=](){
        danmulistContextMenu->exec(QCursor::pos());
    });
    danmulistContextMenu->addAction(act_copyDanmuText);
    danmulistContextMenu->addAction(act_copyDanmuColor);
    danmulistContextMenu->addAction(act_copyDanmuSender);
    danmulistContextMenu->addSeparator();

    danmulistContextMenu->addAction(act_blockText);
    danmulistContextMenu->addAction(act_blockColor);
    danmulistContextMenu->addAction(act_blockSender);
    danmulistContextMenu->addSeparator();

    danmulistContextMenu->addAction(act_deleteDanmu);
    danmulistContextMenu->addAction(act_jumpToTime);


    danmulistProxyModel = new QSortFilterProxyModel(this);
    danmulistProxyModel->setSourceModel(GlobalObjects::danmuPool);
    danmulistProxyModel->setFilterKeyColumn(1);
    danmulistView->setModel(danmulistProxyModel);
    danmulistView->hideColumn(0);
    currentProxyModel = danmulistProxyModel;
    QObject::connect(danmulistView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ListWindow::updateDanmuActions);
    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::modelReset, this, &ListWindow::updateDanmuActions);


    QHeaderView *danmulistHeader=danmulistView->header();
    danmulistHeader->setFont(normalFont);
    QFontMetrics fontMetrics(normalFont);
    danmulistHeader->resizeSection(0,fontMetrics.horizontalAdvance("00:00")+10);
    danmulistPageVLayout->addWidget(danmulistView);

    constexpr const int tbBtnCount = 5;
    QPair<QChar, QString> tbButtonTexts[tbBtnCount] = {
        {QChar(0xe631), tr("Update Danmu Pool")},
        {QChar(0xe667), tr("Add Danmu")},
        {QChar(0xe6c1), tr("Block")},
        {QChar(0xe60a), tr("Edit")},
        {QChar(0xe602), tr("Position")}
    };
    QList<QAction *> tbActions[tbBtnCount] = {
        {},
        {act_addOnlineDanmu, act_addLocalDanmu},
        {act_editBlock},
        {act_editPool},
        {}
    };

    QVector<QToolButton *> btns(tbBtnCount);
    GlobalObjects::iconfont->setPointSize(14);
    QHBoxLayout *poolEditHLayout = new QHBoxLayout();
    poolEditHLayout->setContentsMargins(4, 8, 4, 8);
    for (int i = 0; i < tbBtnCount; ++i)
    {
        QToolButton *tb = new QToolButton(danmulistPage);
        btns[i] = tb;
        tb->setFont(*GlobalObjects::iconfont);
        tb->setText(tbButtonTexts[i].first);
        tb->setToolTip(tbButtonTexts[i].second);
        tb->setObjectName(QStringLiteral("ListEditButton"));
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setPopupMode(QToolButton::InstantPopup);
        if (!tbActions[i].empty())
        {
            if (tbActions[i].size() == 1)
            {
                QObject::connect(tb, &QToolButton::clicked, tbActions[i][0], &QAction::trigger);
            }
            else
            {
                QMenu *popupMenu = new ElaMenu(tb);
                popupMenu->addActions(tbActions[i]);
                tb->setMenu(popupMenu);
            }
        }
        poolEditHLayout->addWidget(tb);

    }
    poolEditHLayout->addStretch(1);
    danmulistPageVLayout->addLayout(poolEditHLayout);

    constexpr const int Index_UpdatePool = 0;
    constexpr const int Index_LocatePosition = 4;
    QObject::connect(btns[Index_UpdatePool], &QToolButton::clicked, this, &ListWindow::updateCurrentPool);
    QObject::connect(btns[Index_LocatePosition], &QToolButton::clicked, this, [this]() {
        danmulistView->scrollTo(danmulistProxyModel->mapFromSource(GlobalObjects::danmuPool->getCurrentIndex()), QAbstractItemView::PositionAtTop);
    });

    updateDanmuActions();

    return danmulistPage;
}

QWidget *ListWindow::initSublistPage()
{
    QWidget *sublistPage = new QWidget(this);
    QVBoxLayout *sublistPageVLayout = new QVBoxLayout(sublistPage);
    sublistPageVLayout->setContentsMargins(0, 0, 0, 0);
    sublistPageVLayout->setSpacing(0);

    QFont normalFont(GlobalObjects::normalFont,11);

    sublistView = new QListView(sublistPage);
    sublistView->setObjectName(QStringLiteral("sublist"));
    sublistView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    sublistView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sublistView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sublistView->setFont(normalFont);
    sublistView->setResizeMode(QListView::Adjust);
    sublistView->setUniformItemSizes(false);
    sublistView->setViewMode(QListView::ListMode);
    sublistView->setWordWrap(true);
    sublistView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    new FloatScrollBar(sublistView->verticalScrollBar(), sublistView);

    sublistPageVLayout->addWidget(sublistView);

    subModel = new SubtitleModel(this);
    sublistProxyModel = new QSortFilterProxyModel(this);
    sublistProxyModel->setSourceModel(subModel);
    sublistView->setModel(sublistProxyModel);
    currentProxyModel = sublistProxyModel;

    SubItemDelegate *delegate = new SubItemDelegate(sublistView);
    QObject::connect(subModel, &SubtitleModel::modelReset, delegate, &SubItemDelegate::reset);
    sublistView->setItemDelegate(delegate);

    QMenu *sublistContextMenu = new ElaMenu(sublistView);

    sublistView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(sublistView, &QListView::customContextMenuRequested, sublistView, [=](){
        sublistContextMenu->exec(QCursor::pos());
    });
    sublistContextMenu->addAction(act_copySubText);
    sublistContextMenu->addAction(act_copySubTime);

    QObject::connect(sublistView, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        QModelIndex itemIndex = sublistProxyModel->mapToSource(index);
        SubItem item = subModel->getSub(itemIndex.row());
        MPVPlayer::PlayState state=GlobalObjects::mpvplayer->getState();
        if (state == MPVPlayer::PlayState::Play || state == MPVPlayer::PlayState::Pause)
        {
            GlobalObjects::mpvplayer->seek(item.startTime);
        }
    });

    constexpr const int tbBtnCount = 5;
    QPair<QChar, QString> tbButtonTexts[tbBtnCount] = {
        {QChar(0xe602), tr("Position")},
        {QChar(0xe667), tr("Add")},
        {QChar(0xe643), tr("Save")},
        {QChar(0xe681), tr("Subtitle Recognition")},
        {QChar(0xefd1), tr("Subtitle Translation")},
    };
    QList<QAction *> tbActions[tbBtnCount] = {
        {},
        {act_addSub},
        {act_saveSub},
        {act_subRecognize},
        {act_subTranslation},
    };

    QVector<QToolButton *> btns(tbBtnCount);
    GlobalObjects::iconfont->setPointSize(14);
    QHBoxLayout *subEditHLayout = new QHBoxLayout();
    subEditHLayout->setContentsMargins(4, 8, 4, 8);
    for (int i = 0; i < tbBtnCount; ++i)
    {
        QToolButton *tb = new QToolButton(sublistPage);
        btns[i] = tb;
        tb->setFont(*GlobalObjects::iconfont);
        tb->setText(tbButtonTexts[i].first);
        tb->setToolTip(tbButtonTexts[i].second);
        tb->setObjectName(QStringLiteral("ListEditButton"));
        tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
        tb->setPopupMode(QToolButton::InstantPopup);
        if (!tbActions[i].empty())
        {
            if (tbActions[i].size() == 1)
            {
                QObject::connect(tb, &QToolButton::clicked, tbActions[i][0], &QAction::trigger);
            }
            else
            {
                QMenu *popupMenu = new ElaMenu(tb);
                popupMenu->addActions(tbActions[i]);
                tb->setMenu(popupMenu);
            }
        }
        subEditHLayout->addWidget(tb);

    }
    subEditHLayout->addStretch(1);
    sublistPageVLayout->addLayout(subEditHLayout);

    constexpr const int Index_LocatePosition = 0;
    QObject::connect(btns[Index_LocatePosition], &QToolButton::clicked, this, [this]() {
        int timeMS = PlayContext::context()->playtime * 1000;
        sublistView->scrollTo(sublistProxyModel->mapFromSource(subModel->getTimeIndex(timeMS)), QAbstractItemView::PositionAtTop);
    });

    return sublistPage;
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
    QTreeView *refView = danmulistView && danmulistView->isVisible() ? danmulistView : playlistView;
    infoTip->setGeometry(0, titleContainer->height() + refView->height() - infoTipHeight, width(), infoTipHeight);
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
    if (currentList() == 0)
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
    else if (currentList() == 1)
    {
        for (QUrl &url : urls)
        {
            if (url.isLocalFile())
            {
                QFileInfo fi(url.toLocalFile());
                if (fi.isFile() && "xml" == fi.suffix())
                {
                    QVector<DanmuComment *> tmplist;
                    LocalProvider::LoadXmlDanmuFile(fi.filePath(),tmplist);
                    DanmuSource sourceInfo;
                    sourceInfo.scriptData = fi.filePath();
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
    else if (currentList() == 2)
    {
        for (QUrl &url : urls)
        {
            if (url.isLocalFile())
            {
                QFileInfo fi(url.toLocalFile());
                if (fi.isFile() && GlobalObjects::mpvplayer->subtitleFormats.contains("*." + fi.suffix().toLower()))
                {
                    GlobalObjects::mpvplayer->addSubtitle(fi.filePath());
                }
            }
        }
    }
}

void ListWindow::setCurrentList(int listType)
{
    filterEdit->clear();
    const QVector<QSortFilterProxyModel *> models{playlistProxyModel, danmulistProxyModel, sublistProxyModel, nullptr};
    currentProxyModel = nullptr;
    if (listType < models.size())
    {
        currentProxyModel = models[listType];
    }
    if (subModel)
    {
        subModel->setBlocked(listType != 2);
    }
    contentStackLayout->setCurrentIndex(listType);
}

int ListWindow::currentList() const
{
    return contentStackLayout->currentIndex();
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
            for (QModelIndex index : selection.indexes())
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
    infoTip->raise();
    QWidget *views[] = { playlistView, danmulistView, sublistView };
    QWidget *refView = views[currentList()];
    infoTip->setGeometry(0, titleContainer->height() + refView->height() - infoTipHeight, width(), infoTipHeight);
    static_cast<InfoTip *>(infoTip)->showMessage(msg,flag);
}


AddUrlDialog::AddUrlDialog(QWidget *parent) : CFramelessDialog(tr("Add URL"), parent, true), decodeTitle(false)
{
    QLabel *tipLabel = new QLabel(tr("Enter URL(http, https, smb), separate multiple urls with line breaks"), this);
    edit = new KPlainTextEdit(this, false);
    newCollectionCheck = new ElaCheckBox(tr("Add to new collection"), this);
    collectionEdit = new ElaLineEdit(this);
    collectionEdit->setPlaceholderText(tr("Input Collection Name"));
    collectionEdit->setEnabled(false);
    QObject::connect(newCollectionCheck, &QCheckBox::stateChanged, collectionEdit, &QLineEdit::setEnabled);
    decodeTitleCheck = new ElaCheckBox(tr("Decode Title From URL"), this);

    QGridLayout *inputGLayout = new QGridLayout(this);
    inputGLayout->addWidget(tipLabel, 0, 0, 1, 2);
    inputGLayout->addWidget(edit, 1, 0, 1, 2);
    inputGLayout->addWidget(newCollectionCheck, 2, 0);
    inputGLayout->addWidget(collectionEdit, 2, 1);
    inputGLayout->addWidget(decodeTitleCheck, 3, 0, 1, 2);
    inputGLayout->setRowStretch(1, 1);
    inputGLayout->setColumnStretch(1, 1);

    setSizeSettingKey("AddURLDialog" ,QSize(200, 120));
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

AddWebDAVCollectionDialog::AddWebDAVCollectionDialog(QWidget *parent) : CFramelessDialog(tr("Add WebDAV Collection"), parent, true)
{
    QLabel *titleTip = new QLabel(tr("Title"), this);
    titleEdit = new ElaLineEdit(this);
    QLabel *urlTip = new QLabel(tr("URL"), this);
    urlEdit = new ElaLineEdit(this);
    QLabel *portTip = new QLabel(tr("Port"), this);
    portEdit = new ElaLineEdit(this);
    QLabel *pathTip = new QLabel(tr("Path"), this);
    pathEdit = new ElaLineEdit(this);
    QLabel *userTip = new QLabel(tr("User"), this);
    userEdit = new ElaLineEdit(this);
    QLabel *passwordTip = new QLabel(tr("Password"), this);
    passwordEdit = new ElaLineEdit(this);

    QIntValidator *intValidator = new QIntValidator(this);
    intValidator->setBottom(0);
    portEdit->setValidator(intValidator);

    QGridLayout *inputGLayout = new QGridLayout(this);
    inputGLayout->addWidget(titleTip, 0, 0);
    inputGLayout->addWidget(titleEdit, 0, 1, 1, 3);
    inputGLayout->addWidget(urlTip, 1, 0);
    inputGLayout->addWidget(urlEdit, 1, 1, 1, 3);
    inputGLayout->addWidget(portTip, 2, 0);
    inputGLayout->addWidget(portEdit, 2, 1);
    inputGLayout->addWidget(pathTip, 2, 2);
    inputGLayout->addWidget(pathEdit, 2, 3);
    inputGLayout->addWidget(userTip, 3, 0);
    inputGLayout->addWidget(userEdit, 3, 1);
    inputGLayout->addWidget(passwordTip, 3, 2);
    inputGLayout->addWidget(passwordEdit, 3, 3);

    inputGLayout->setColumnStretch(1, 1);
    inputGLayout->setColumnStretch(3, 1);

    setSizeSettingKey("AddWebDAVCollectionDialog" ,QSize(300, 120));
}

void AddWebDAVCollectionDialog::onAccept()
{
    title = titleEdit->text().trimmed();
    url = urlEdit->text().trimmed();
    user = userEdit->text();
    password = passwordEdit->text();
    if (title.isEmpty() || url.isEmpty())
    {
        showMessage(tr("Title/URL can't be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    if (!url.endsWith("/"))
    {
        showMessage(tr("URL must end with /"), NM_ERROR | NM_HIDE);
        return;
    }
    if (!portEdit->text().isEmpty())
    {
        QUrl tUrl(url);
        bool ok = false;
        int port = portEdit->text().toInt(&ok);
        if (ok)
        {
            tUrl.setPort(port);
            url = tUrl.toString();
        }
    }
    if (!pathEdit->text().trimmed().isEmpty())
    {
        if (!pathEdit->text().endsWith("/"))
        {
            showMessage(tr("Path must end with /"), NM_ERROR | NM_HIDE);
            return;
        }
        QUrl tUrl(url);
        tUrl.setPath(pathEdit->text().trimmed());
        url = tUrl.toString();
    }
    CFramelessDialog::onAccept();
}

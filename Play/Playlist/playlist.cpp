#include "playlist.h"

#include <QMimeData>
#include <QColor>
#include <QBrush>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QCollator>

#include "playlistprivate.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/animeprovider.h"
#include "Common/notifier.h"

namespace
{
    static QCollator comparer;
    struct
    {
        inline bool operator()(const PlayListItem *item1,const PlayListItem *item2) const
        {
            return comparer.compare(item1->title,item2->title)>=0?false:true;
        }
    } titleCompareAscending;
    struct
    {
        inline bool operator()(const PlayListItem *item1,const PlayListItem *item2) const
        {
            return comparer.compare(item2->title,item1->title)>=0?false:true;
        }
    } titleCompareDescending;
}

PlayList::PlayList(QObject *parent) : QAbstractItemModel(parent), d_ptr(new PlayListPrivate(this))
{
    Q_D(PlayList);
    comparer.setNumericMode(true);
    d->loadRecentlist();
    d->loadPlaylist();
    qRegisterMetaType<QList<PlayListItem *> >("QList<PlayListItem *>");
    matchWorker=new MatchWorker();
    matchWorker->moveToThread(GlobalObjects::workThread);
    QObject::connect(GlobalObjects::workThread, &QThread::finished, matchWorker, &QObject::deleteLater);
    QObject::connect(matchWorker, &MatchWorker::matchDown, this, [this](const QList<PlayListItem *> &matchedItems){
        Q_D(PlayList);
        d->playListChanged = true;
        d->needRefresh = true;
        for(auto currentItem : matchedItems)
        {
            QModelIndex nIndex = createIndex(currentItem->parent->children->indexOf(currentItem), 0, currentItem);
            emit dataChanged(nIndex, nIndex);
            if (currentItem == d->currentItem) emit currentMatchChanged(currentItem->poolID);
            autoMoveToBgmCollection(nIndex);
        }
        d->savePlaylist();
        emit matchStatusChanged(false);
    });
}

PlayList::~PlayList()
{
    Q_D(PlayList);
    d->savePlaylist();
    d->saveRecentlist();
    delete d;
}

const PlayListItem *PlayList::getCurrentItem() const
{
    Q_D(const PlayList);
    return d->currentItem;
}

QModelIndex PlayList::getCurrentIndex() const
{
    Q_D(const PlayList);
    PlayListItem *currentItem = d->currentItem;
    return currentItem?createIndex(currentItem->parent->children->indexOf(currentItem),0,currentItem):QModelIndex();
}

QList<const PlayListItem *> PlayList::getSiblings(const PlayListItem *item, bool sameAnime)
{
    QList<const PlayListItem *> siblings;
    if(item->parent)
    {
        for(PlayListItem *sibling:*item->parent->children)
        {
            if(!sibling->children && !sibling->path.isEmpty() && (!sameAnime || sibling->animeTitle==item->animeTitle)) siblings<<sibling;
        }
    }
    return siblings;
}

PlayList::LoopMode PlayList::getLoopMode() const
{
    Q_D(const PlayList);
    return d->loopMode;
}

bool PlayList::canPaste() const
{
    Q_D(const PlayList);
    return d->itemsClipboard.count()>0;
}

const QList<QPair<QString, QString> > &PlayList::recent()
{
    Q_D(PlayList);
    return d->recentList;
}

void PlayList::removeRecentItem(const QString &path)
{
    Q_D(PlayList);
    for(auto iter= d->recentList.begin();iter!=d->recentList.end();)
    {
        if((*iter).first==path)
            iter=d->recentList.erase(iter);
        else
            iter++;
    }
    emit recentItemsUpdated();
}

void PlayList::setFinishTimeOnce(bool on)
{
    Q_D(PlayList);
    d->saveFinishTimeOnce = on;
}

bool PlayList::saveFinishTimeOnce()
{
    Q_D(PlayList);
    return d->saveFinishTimeOnce;
}

int PlayList::addItems(QStringList &items, QModelIndex parent)
{
    Q_D(PlayList);
	QStringList tmpItems;
    for(auto iter=items.cbegin();iter!=items.cend();++iter)
    {
        if (!d->fileItems.contains(*iter))
			tmpItems.append(*iter);
    }
    Notifier *notifier = Notifier::getNotifier();
    if(tmpItems.count()==0)
    {
        notifier->showMessage(Notifier::LIST_NOTIFY, tr("File exist or Unsupported format"), NM_HIDE);
        return 0;
    }
    int insertPosition(0);
    PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : d->root;
    if(parentItem->children)
        insertPosition = parentItem->children->size();
    else
    {
        insertPosition = parentItem->parent->children->indexOf(parentItem) + 1;
        parentItem = parentItem->parent;
        parent = this->parent(parent);
    }
    QVector<PlayListItem *> matchItems;
    beginInsertRows(parent, insertPosition, insertPosition+ tmpItems.count()-1);
    for (const QString &item : tmpItems)
	{
        int suffixPos = item.lastIndexOf('.'), pathPos = item.lastIndexOf('/') + 1;
		QString title = item.mid(pathPos,suffixPos-pathPos);
        PlayListItem *newItem = new PlayListItem(parentItem, true, insertPosition++);
        newItem->title = title;
		newItem->path = item;
        newItem->pathHash = QCryptographicHash::hash(item.toUtf8(),QCryptographicHash::Md5).toHex();
        newItem->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        d->fileItems.insert(newItem->path,newItem);
        if(d->autoMatch) matchItems<<newItem;
	}
	endInsertRows();
    d->addMediaPathHash(matchItems);
    d->playListChanged=true;
    d->needRefresh = true;
    d->incModifyCounter();
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Add %1 item(s)").arg(tmpItems.size()),NM_HIDE);
    if(d->autoMatch && matchItems.count()>0)
    {
        emit matchStatusChanged(true);
        QMetaObject::invokeMethod(matchWorker, [this, matchItems](){
            matchWorker->match(matchItems);
        },Qt::QueuedConnection);
    }
    return tmpItems.size();
}

int PlayList::addFolder(QString folderStr, QModelIndex parent)
{
    Q_D(PlayList);
    int insertPosition(0);
    PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : d->root;
	if (parentItem->children)
    {
        insertPosition = parentItem->children->size();
    }
	else
	{
        insertPosition = parentItem->parent->children->indexOf(parentItem) + 1;
		parentItem = parentItem->parent;
		parent = this->parent(parent);
	}
	
    PlayListItem folderRootCollection;
    int itemCount=0;
    d->addSubFolder(folderStr, &folderRootCollection,itemCount);
    QVector<PlayListItem *> matchItems;
    if (folderRootCollection.children->count())
	{
        PlayListItem *folderRoot(folderRootCollection.children->first());
        beginInsertRows(parent, insertPosition, insertPosition);
        folderRoot->moveTo(parentItem, insertPosition);
		endInsertRows();
        d->playListChanged=true;
        d->needRefresh = true;
        d->incModifyCounter();

        QVector<PlayListItem *> items({folderRoot});
        while(!items.empty())
        {
            PlayListItem *currentItem=items.takeFirst();
            if(currentItem->children)
            {
                for(PlayListItem *child:*currentItem->children)
                {
                    items.push_back(child);
                }
            }
            else if(currentItem->poolID.isEmpty())
            {
                matchItems<<currentItem;
            }
        }
    }
    d->addMediaPathHash(matchItems);
    Notifier *notifier = Notifier::getNotifier();
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Add %1 item(s)").arg(itemCount),NotifyMessageFlag::NM_HIDE);
    if(d->autoMatch && matchItems.count()>0)
    {
        emit matchStatusChanged(true);
        QMetaObject::invokeMethod(matchWorker, [this, matchItems](){
            matchWorker->match(matchItems);
        },Qt::QueuedConnection);
    }
    return itemCount;
}

void PlayList::deleteItems(const QModelIndexList &deleteIndexes)
{
    QList<PlayListItem *> items;
    for(const QModelIndex &index : deleteIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            items.append(item);
        }
    }
    std::sort(items.begin(),items.end(),[](const PlayListItem *item1,const PlayListItem *item2){return item1->level>item2->level;});
    for(PlayListItem *curItem:items)
    {
        int cr_row = curItem->parent->children->indexOf(curItem);
        const QModelIndex &itemIndex = createIndex(cr_row, 0, curItem);
        beginRemoveRows(itemIndex.parent(), cr_row, cr_row);
        curItem->parent->children->removeAt(cr_row);
        endRemoveRows();
        delete curItem;
    }
    Q_D(PlayList);
    d->playListChanged=true;
    d->needRefresh = true;
    d->incModifyCounter();
}

int PlayList::deleteInvalidItems(const QModelIndexList &indexes)
{
    QList<PlayListItem *> items, invalidItems;
    for(const QModelIndex &index : indexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            items.append(item);
        }
    }
    while(!items.empty())
    {
        PlayListItem *currentItem=items.takeFirst();
        if(currentItem->children)
        {
            for(PlayListItem *child:*currentItem->children)
            {
                items.push_back(child);
            }
            if(!currentItem->path.isEmpty())
            {
                if(!QFileInfo::exists(currentItem->path)) invalidItems.append(currentItem);
            }
        }
        else if(!currentItem->path.isEmpty())
        {
            if(!QFileInfo::exists(currentItem->path)) invalidItems.append(currentItem);
        }
    }
    if(invalidItems.size() == 0) return 0;
    std::sort(invalidItems.begin(),invalidItems.end(),[](const PlayListItem *item1,const PlayListItem *item2){return item1->level>item2->level;});
    for(PlayListItem *curItem:invalidItems)
    {
        int cr_row = curItem->parent->children->indexOf(curItem);
        const QModelIndex &itemIndex = createIndex(cr_row, 0, curItem);
        beginRemoveRows(itemIndex.parent(), cr_row, cr_row);
        curItem->parent->children->removeAt(cr_row);
        endRemoveRows();
        delete curItem;
    }
    Q_D(PlayList);
    d->playListChanged=true;
    d->needRefresh = true;
    d->incModifyCounter();
    return invalidItems.size();
}

void PlayList::clear()
{
    Q_D(PlayList);
    PlayListItem *root = d->root;
    if(root->children->count()==0)return;
    beginRemoveRows(QModelIndex(), 0, root->children->count()-1);
    for(PlayListItem *child:*root->children)
        delete child;
    root->children->clear();
    endRemoveRows();
    d->pathHashLock.lockForWrite();
    d->mediaPathHash.clear();
    d->pathHashLock.unlock();
    d->fileItems.clear();
    d->playListChanged=true;
    d->needRefresh = true;
}

void PlayList::sortItems(const QModelIndex &parent, bool ascendingOrder)
{
    Q_D(PlayList);
    PlayListItem *parentItem= parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : d->root;
    if(parentItem->children)
    {
        QList<QPersistentModelIndex> persistentIndexList;
        persistentIndexList.append(QPersistentModelIndex(parent));
        emit layoutAboutToBeChanged(persistentIndexList);
        if(ascendingOrder)
            std::sort(parentItem->children->begin(),parentItem->children->end(),titleCompareAscending);
        else
            std::sort(parentItem->children->begin(),parentItem->children->end(),titleCompareDescending);
        emit layoutChanged(persistentIndexList);
    }
    d->playListChanged=true;
    d->needRefresh = true;
    d->incModifyCounter();
}

void PlayList::sortAllItems(bool ascendingOrder)
{
    Q_D(PlayList);
    QList<PlayListItem *> items;
    items.push_back(d->root);
    emit layoutAboutToBeChanged();
    while(!items.empty())
    {
        PlayListItem *currentItem=items.front();
        items.pop_front();
        if(ascendingOrder)
            std::sort(currentItem->children->begin(),currentItem->children->end(),titleCompareAscending);
        else
            std::sort(currentItem->children->begin(),currentItem->children->end(),titleCompareDescending);
        for(PlayListItem *child:*currentItem->children)
            if(child->children)
                items.push_back(child);
    }
    d->playListChanged=true;
    d->needRefresh = true;
    emit layoutChanged();
}

QModelIndex PlayList::addCollection(QModelIndex parent, QString title)
{
    Q_D(PlayList);
    PlayListItem *newCollection(nullptr);
    PlayListItem *parentItem= parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : d->root;
    int insertPosition(0);
	if (parentItem->children)
	{
        insertPosition = parentItem->children->size();
	}
	else
	{
        insertPosition = parentItem->parent->children->indexOf(parentItem) + 1;
		parentItem = parentItem->parent;
		parent = this->parent(parent);
	}
    beginInsertRows(parent, insertPosition, insertPosition);
    newCollection = new PlayListItem(parentItem,false,insertPosition);
	newCollection->title = title;
    newCollection->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
	endInsertRows();
    d->playListChanged=true;
    d->needRefresh = true;
    d->incModifyCounter();
    return this->index(insertPosition,0,parent);
}

int PlayList::refreshFolder(const QModelIndex &index)
{
    Q_D(PlayList);
    if(!index.isValid())return 0;
    PlayListItem *item= static_cast<PlayListItem*>(index.internalPointer());
    if(!item->children) return 0;
    QVector<PlayListItem *> nItems;
    int c = d->refreshFolder(item, nItems);
    d->addMediaPathHash(nItems);
    Notifier *notifier = Notifier::getNotifier();
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Add %1 item(s)").arg(c),NotifyMessageFlag::NM_HIDE);
    if(c>0)
    {
        d->playListChanged=true;
        d->needRefresh = true;
        d->incModifyCounter();
        if(d->autoMatch)
        {
            emit matchStatusChanged(true);
            QMetaObject::invokeMethod(matchWorker, [this, nItems](){
                matchWorker->match(nItems);
            },Qt::QueuedConnection);
        }
        d->savePlaylist();
    }
    return c;
}

void PlayList::cutItems(const QModelIndexList &cutIndexes)
{
    Q_D(PlayList);
    if(!d->itemsClipboard.empty())
    {
        qDeleteAll(d->itemsClipboard);
        d->itemsClipboard.clear();
    }
    foreach (const QModelIndex &index, cutIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            d->itemsClipboard.append(item);
        }
    }
    std::sort(d->itemsClipboard.begin(),d->itemsClipboard.end(),[](const PlayListItem *item1,const PlayListItem *item2){return item1->level>item2->level;});
    for(PlayListItem *curItem:d->itemsClipboard)
    {
        int cr_row = curItem->parent->children->indexOf(curItem);
        const QModelIndex &itemIndex = createIndex(cr_row, 0, curItem);
        beginRemoveRows(itemIndex.parent(), cr_row, cr_row);
        curItem->parent->children->removeAt(cr_row);
        endRemoveRows();
    }
    d->playListChanged=true;
}

void PlayList::pasteItems(QModelIndex parent)
{
    Q_D(PlayList);
    if(d->itemsClipboard.count()==0)return;
    int insertPosition(0);
    PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : d->root;
    if (parentItem->children)
        insertPosition = parentItem->children->size();
    else
    {
        insertPosition = parentItem->parent->children->indexOf(parentItem) + 1;
        parentItem = parentItem->parent;
        parent = this->parent(parent);
    }
    beginInsertRows(parent, insertPosition, insertPosition+d->itemsClipboard.count()-1);
    for (PlayListItem *item : d->itemsClipboard)
    {
        item->parent=nullptr;
        item->moveTo(parentItem,insertPosition++);
    }
    endInsertRows();
    d->playListChanged=true;
    d->needRefresh = true;
    d->itemsClipboard.clear();
}

void PlayList::moveUpDown(QModelIndex index, bool up)
{
    if(!index.isValid())return;
    PlayListItem *item= static_cast<PlayListItem*>(index.internalPointer());
    PlayListItem *parent=item->parent;
    int row=parent->children->indexOf(item);
    int endPos=up?0:parent->children->count()-1;
    if(row==endPos)return;
    QModelIndex parentIndex=this->parent(index);
    beginMoveRows(parentIndex,row,row,parentIndex,up?row-1:row+2);
    parent->children->move(row,up?row-1:row+1);
    endMoveRows();
    Q_D(PlayList);
    d->playListChanged=true;
    d->needRefresh = true;
    d->incModifyCounter();
}

void PlayList::switchBgmCollection(const QModelIndex &index)
{
    Q_D(PlayList);
    if(!index.isValid())return;
    PlayListItem *item= static_cast<PlayListItem*>(index.internalPointer());
    if(!item->children || !item->path.isEmpty()) return;
    item->isBgmCollection=!item->isBgmCollection;
    if(item->isBgmCollection)
    {
        d->bgmCollectionItems.insert(item->title, item);
    }
    else
    {
        d->bgmCollectionItems.remove(item->title);
    }
    d->playListChanged=true;
    emit dataChanged(index, index);
}

void PlayList::setMarker(const QModelIndex &index, PlayListItem::Marker marker)
{
    Q_D(PlayList);
    if(!index.isValid())return;
    PlayListItem *item= static_cast<PlayListItem*>(index.internalPointer());
    if(item->marker==marker) return;
    item->marker = marker;
    d->playListChanged=true;
    d->needRefresh = true;
    d->incModifyCounter();
    emit dataChanged(index, index);
}

void PlayList::autoMoveToBgmCollection(const QModelIndex &index)
{
    Q_D(PlayList);
    PlayListItem *currentItem= static_cast<PlayListItem*>(index.internalPointer());
    if(currentItem->parent->isBgmCollection && currentItem->parent->title==currentItem->animeTitle) return;
    PlayListItem *bgmCollectionItem=d->bgmCollectionItems.value(currentItem->animeTitle, nullptr);
    if(!bgmCollectionItem)
    {
        PlayListItem *parentItem= currentItem->parent;
        int insertPosition = parentItem->children->indexOf(currentItem) + 1;
        QModelIndex pIndex = parentItem==d->root?QModelIndex():
                                                 createIndex(parentItem->parent->children->indexOf(parentItem), 0, parentItem);

        beginInsertRows(pIndex, insertPosition, insertPosition);
        bgmCollectionItem=new PlayListItem(parentItem,false,insertPosition);
        bgmCollectionItem->title = currentItem->animeTitle;
        bgmCollectionItem->isBgmCollection=true;
        bgmCollectionItem->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        endInsertRows();
        d->bgmCollectionItems.insert(bgmCollectionItem->title, bgmCollectionItem);
    }
    beginRemoveRows(index.parent(), index.row(), index.row());
    currentItem->parent->children->removeAt(index.row());
    endRemoveRows();
    QModelIndex bgmCollectionIndex=createIndex(bgmCollectionItem->parent->children->indexOf(bgmCollectionItem),0,bgmCollectionItem);
    int insertPosition = bgmCollectionItem->children->count();
    beginInsertRows(bgmCollectionIndex, insertPosition, insertPosition);
    currentItem->moveTo(bgmCollectionItem);
    endInsertRows();
    d->playListChanged = true;
    d->needRefresh = true;
}

QModelIndex PlayList::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const PlayList);
    if (!hasIndex(row, column, parent)) return QModelIndex();
    const PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : d->root;
	PlayListItem *childItem = parentItem->children->value(row);
    return childItem?createIndex(row, column, childItem):QModelIndex();
}

QModelIndex PlayList::parent(const QModelIndex &child) const
{
    Q_D(const PlayList);
    if (!child.isValid()) return QModelIndex();

    PlayListItem *childItem = static_cast<PlayListItem*>(child.internalPointer());
    PlayListItem *parentItem = childItem->parent;

    if (parentItem == d->root) return QModelIndex();
    int row=parentItem->parent->children->indexOf(parentItem);
    return createIndex(row, 0, parentItem);
}

int PlayList::rowCount(const QModelIndex &parent) const
{
    Q_D(const PlayList);
    if (parent.column() > 0) return 0;
    const PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : d->root;
    return parentItem->children?parentItem->children->size():0;
}

QVariant PlayList::data(const QModelIndex &index, int role) const
{
    Q_D(const PlayList);
    if (!index.isValid()) return QVariant();
    PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());

    switch (role)
    {
    case Qt::EditRole:
        return item->title;
    case Qt::DisplayRole:
        return item->title;
    case Qt::ToolTipRole:
    {
        QStringList tipContent;
        if(item->children)
        {
            tipContent<<item->title;
            if(item->isBgmCollection)
            {
                tipContent<< tr("Bangumi Collection");
            }
            else if(!item->path.isEmpty())
            {
                tipContent<< tr("Folder Collection");
                tipContent<< item->path;
            }
            if (item->addTime > 0)
            {
                QString addTime(QDateTime::fromSecsSinceEpoch(item->addTime).toString("yyyy-MM-dd hh:mm:ss"));
                tipContent<< tr("Add Time: %1").arg(addTime);
            }
        }
        else
        {
            if(!item->animeTitle.isEmpty())
                tipContent<<QString("%1-%2").arg(item->animeTitle, item->title);
            else
                tipContent<<QString("%1").arg(item->title);
            tipContent<<item->path;
            if (item->addTime > 0)
            {
                QString addTime(QDateTime::fromSecsSinceEpoch(item->addTime).toString("yyyy-MM-dd hh:mm:ss"));
                tipContent<< tr("Add Time: %1").arg(addTime);
            }
            if(item->playTimeState==PlayListItem::UNPLAY)
                tipContent<<tr("Unplayed");
            else if(item->playTimeState==PlayListItem::FINISH)
                tipContent<<tr("Finished");
            else
            {
                int cmin=item->playTime/60;
                int cls=item->playTime-cmin*60;
                tipContent<<(tr("PlayTo: %1:%2").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')));
            }
        }
        return tipContent.join('\n');
    }
    case Qt::ForegroundRole:
    {
        static QBrush curBrush(QColor(255,255,0));
        if(item==d->currentItem) return curBrush;
        static QBrush brs[]={QBrush(QColor(220,220,220)),QBrush(QColor(160,200,200)),QBrush(QColor(140,140,140))};
        return brs[item->playTimeState];
    }
    case Qt::DecorationRole:
        return item==d->currentItem?QIcon(":/res/images/playing.svg"):QVariant();
    case ItemRole::BgmCollectionRole:
        return (item->isBgmCollection && item->children);
    case ItemRole::FolderCollectionRole:
        return (item->children && !item->path.isEmpty());
    case ItemRole::ColorMarkerRole:
        return item->marker;
    case ItemRole::FilePathRole:
        return item->path;
    default:
        return QVariant();
    }
}

Qt::ItemFlags PlayList::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
    return index.isValid()? flags|Qt::ItemIsDragEnabled : flags;
}

QMimeData *PlayList::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    for(const QModelIndex &index : indexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            stream.writeRawData((const char *)&item,sizeof(PlayListItem *));
        }
    }
    mimeData->setData("application/x-kikoplaylistitem", encodedData);
    return mimeData;
}

bool PlayList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int , const QModelIndex &parent)
{
    if (!data->hasFormat("application/x-kikoplaylistitem")) return false;
    if (action == Qt::IgnoreAction) return true;

    QByteArray encodedData = data->data("application/x-kikoplaylistitem");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QList<PlayListItem *> newItems;
    int rows = 0;

    while (!stream.atEnd())
    {
        PlayListItem *item;
        stream.readRawData((char *)&item,sizeof(PlayListItem *));
        newItems << item;
        ++rows;
    }

    Q_D(PlayList);
    int beginRow=row!=-1? row : rowCount(parent);
    PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem *>(parent.internalPointer()) : d->root;
    QModelIndex pParentIndex=parent;
    if (!parentItem->children)
    {
        beginRow=parentItem->parent->children->indexOf(parentItem);
        parentItem=parentItem->parent;
        pParentIndex=this->parent(parent);
    }
    for (int cr = 0; cr < rows; ++cr)
	{
		PlayListItem *curItem = newItems[cr];
        PlayListItem *curParent=curItem->parent;
        int cr_row = curParent->children->indexOf(curItem);
        QModelIndex itemIndex = createIndex(cr_row, 0, curItem);
		beginRemoveRows(itemIndex.parent(), cr_row, cr_row);
        curParent->children->removeAt(cr_row);
		endRemoveRows();
        if (cr_row < beginRow && curParent==parentItem)
		{
			beginRow--;
		}
        //Moving within the same node, the pParentIndex may change when curItem is removed
        if (parentItem->parent == curParent)
		{
			int parentRow = curParent->children->indexOf(parentItem);
            pParentIndex = createIndex(parentRow, 0, parentItem);
		}
        beginInsertRows(pParentIndex, beginRow, beginRow);
		curItem->parent = parentItem;
        curItem->setLevel(parentItem->level+1);
		parentItem->children->insert(beginRow++, curItem);
		endInsertRows();
	}
    d->playListChanged=true;
    d->needRefresh = true;
    return true;
}

bool PlayList::setData(const QModelIndex &index, const QVariant &value, int)
{
    Q_D(PlayList);
    PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
    QString val=value.toString();
    if(!val.isEmpty() && item->title!=val)
    {
        if(item->children && item->isBgmCollection)
        {
            d->bgmCollectionItems.remove(item->title);
            d->bgmCollectionItems.insert(val, item);
        }
        item->title=val;
        d->playListChanged=true;
        d->needRefresh = true;
        d->incModifyCounter();
        return true;
    }
    return false;

}

const PlayListItem *PlayList::setCurrentItem(const QModelIndex &index,bool playChild)
{
    Q_D(PlayList);
    PlayListItem *current = index.isValid() ? static_cast<PlayListItem *>(index.internalPointer()) : d->root;
    PlayListItem *cur=current;
    if(playChild && cur->children)
    {
        while(cur->children && cur->children->count()>0)
            cur=cur->children->first();
        while (cur!=current)
        {
            if(!cur->children)break;
            int row=cur->parent->children->indexOf(cur);
            if(row==cur->parent->children->count()-1)
                cur=cur->parent;
            else
            {
                PlayListItem *next=cur->parent->children->at(row+1);
                while(next->children && next->children->count()>0)
                    next=next->children->first();
                cur=next;
            }
        }
    }
    if(cur->children || cur==d->currentItem)
        return nullptr;
    QFileInfo fileInfo(cur->path);
    if(!fileInfo.exists())
    {
        Notifier *notifier = Notifier::getNotifier();
        notifier->showMessage(Notifier::LIST_NOTIFY, tr("File Not Exist"), NM_HIDE);
        return nullptr;
    }
    PlayListItem *tmp = d->currentItem;
    d->currentItem = cur;
	if (tmp)
	{
		QModelIndex nIndex = createIndex(tmp->parent->children->indexOf(tmp), 0, tmp);
		emit dataChanged(nIndex, nIndex);
	}
    d->updateRecentlist(cur);
    if(!cur->animeTitle.isEmpty())
        AnimeWorker::instance()->updateEpTime(cur->animeTitle, cur->path);
    return cur;
}

const PlayListItem *PlayList::setCurrentItem(const QString &path)
{
    Q_D(PlayList);
    PlayListItem *curItem=d->fileItems.value(path,nullptr);
    if(curItem && d->currentItem!=curItem)
    {
        PlayListItem *tmp = d->currentItem;
        d->currentItem = curItem;
        if (tmp)
        {
            QModelIndex nIndex = createIndex(tmp->parent->children->indexOf(tmp), 0, tmp);
            emit dataChanged(nIndex, nIndex);
        }
        QModelIndex cIndex = createIndex(curItem->parent->children->indexOf(curItem), 0, curItem);
        emit dataChanged(cIndex, cIndex);
        if(!curItem->animeTitle.isEmpty())
            AnimeWorker::instance()->updateEpTime(curItem->animeTitle, curItem->path);
        d->updateRecentlist(curItem);
    }
    return curItem;
}

void PlayList::cleanCurrentItem()
{
    Q_D(PlayList);
    if (d->currentItem)
	{
        QModelIndex cIndex = createIndex(d->currentItem->parent->children->indexOf(d->currentItem), 0, d->currentItem);
        d->currentItem = nullptr;
		emit dataChanged(cIndex, cIndex);
	}
}

const PlayListItem *PlayList::playPrevOrNext(bool prev)
{
    Q_D(PlayList);
    PlayListItem *item(nullptr);
    item=d->getPrevOrNextItem(prev);
    if(item)
    {
        PlayListItem *tmp = d->currentItem;
        d->currentItem = item;
        if (tmp)
        {
            QModelIndex nIndex(createIndex(tmp->parent->children->indexOf(tmp), 0, tmp));
            emit dataChanged(nIndex, nIndex);
        }
        QModelIndex nIndex(createIndex(item->parent->children->indexOf(item),0,item));
        emit dataChanged(nIndex,nIndex);
        d->updateRecentlist(item);
        if(!item->animeTitle.isEmpty())
            AnimeWorker::instance()->updateEpTime(item->animeTitle, item->path);
        return item;
    }
    return nullptr;
}

void PlayList::setLoopMode(PlayList::LoopMode newMode)
{
    Q_D(PlayList);
    d->loopMode=newMode;
}

void PlayList::checkCurrentItem(PlayListItem *itemDeleted)
{
    Q_D(PlayList);
    if(!itemDeleted->path.isEmpty())
    {
        d->fileItems.remove(itemDeleted->path);
        d->pathHashLock.lockForWrite();
        d->mediaPathHash.remove(itemDeleted->pathHash);
        d->pathHashLock.unlock();
    }
    if(itemDeleted->isBgmCollection) d->bgmCollectionItems.remove(itemDeleted->title);
    if(itemDeleted==d->currentItem)
    {
        d->currentItem=nullptr;
        emit currentInvaild();
    }
}

void PlayList::setAutoMatch(bool on)
{
    Q_D(PlayList);
    d->autoMatch=on;
}

void PlayList::matchItems(const QModelIndexList &matchIndexes)
{
    QVector<PlayListItem *> items, selectedItems;
    for(const QModelIndex &index : matchIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            items.append(item);
        }
    }
    while(!items.empty())
    {
        PlayListItem *currentItem=items.takeFirst();
        if(currentItem->children)
        {
            for(PlayListItem *child:*currentItem->children)
            {
                items.push_back(child);
            }
        }
        else if(!currentItem->hasPool())
        {
            selectedItems<<currentItem;
        }
    }
    if(selectedItems.count()==0) return;
    emit matchStatusChanged(true);
    QMetaObject::invokeMethod(matchWorker, [this, selectedItems](){
        matchWorker->match(selectedItems);
    },Qt::QueuedConnection);
}

void PlayList::matchIndex(QModelIndex &index, const MatchResult &match)
{
    Q_D(PlayList);
    if(!index.isValid())return;
    PlayListItem *item=static_cast<PlayListItem *>(index.internalPointer());
    item->animeTitle=match.name;
    item->title=match.ep.toString();
    item->poolID=GlobalObjects::danmuManager->updateMatch(item->path,match);
    d->playListChanged = true;
    d->needRefresh = true;
    Notifier *notifier = Notifier::getNotifier();
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Success: %1").arg(item->title),NotifyMessageFlag::NM_HIDE);
    emit dataChanged(index, index);
    if (item == d->currentItem)
    {
        emit currentMatchChanged(item->poolID);
    }
    autoMoveToBgmCollection(index);
    AnimeWorker::instance()->addAnime(match);
    //GlobalObjects::library->addToLibrary(item->animeTitle,item->title,item->path);
    d->savePlaylist();
}

void PlayList::matchItems(const QList<const PlayListItem *> &items, const QString &title,  const QList<EpInfo> &eps)
{
    QVector<PlayListItem *> ncItems;
    for(auto i : items)
    {
        ncItems.append(const_cast<PlayListItem *>(i));
    }
    emit matchStatusChanged(true);
    QMetaObject::invokeMethod(matchWorker, [=](){
        matchWorker->match(ncItems, title, eps);
    },Qt::QueuedConnection);
}

void PlayList::removeMatch(const QModelIndexList &matchIndexes)
{
    Q_D(PlayList);
    QList<PlayListItem *> items, selectedItems;
    for(const QModelIndex &index : matchIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            items.append(item);
        }
    }
    while(!items.empty())
    {
        PlayListItem *currentItem=items.takeFirst();
        if(currentItem->children)
        {
            for(PlayListItem *child:*currentItem->children)
            {
                items.push_back(child);
            }
        }
        else if(!currentItem->poolID.isEmpty())
        {
            GlobalObjects::danmuManager->removeMatch(currentItem->path);
            currentItem->poolID = "";
            currentItem->animeTitle = "";
            int suffixPos = currentItem->path.lastIndexOf('.'), pathPos = currentItem->path.lastIndexOf('/') + 1;
            currentItem->title = currentItem->path.mid(pathPos, suffixPos - pathPos);
            if (currentItem == d->currentItem) emit currentMatchChanged(currentItem->poolID);
            d->playListChanged = true;
            d->needRefresh = true;
            QModelIndex nIndex = createIndex(currentItem->parent->children->indexOf(currentItem), 0, currentItem);
            emit dataChanged(nIndex, nIndex);
        }
    }
}

void PlayList::updateItemsDanmu(const QModelIndexList &itemIndexes)
{
    QList<PlayListItem *> items;
    for(const QModelIndex &index : itemIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            items.append(item);
        }
    }
    bool cancel = false;
    auto notifier = Notifier::getNotifier();
    auto conn = QObject::connect(notifier, &Notifier::cancelTrigger, [&](int nType){ if(nType&Notifier::LIST_NOTIFY) cancel=true;});
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Update Start"),NotifyMessageFlag::NM_PROCESS|NotifyMessageFlag::NM_SHOWCANCEL);
    while(!items.empty())
    {
        if(cancel) break;
        PlayListItem *currentItem=items.takeFirst();
        if(currentItem->children)
        {
            for(PlayListItem *child:*currentItem->children)
            {
                items.push_back(child);
            }
        }
        else if(!currentItem->poolID.isEmpty())
        {
            Pool *pool=GlobalObjects::danmuManager->getPool(currentItem->poolID);
            if(!pool) continue;
            notifier->showMessage(Notifier::LIST_NOTIFY, tr("Updating: %1").arg(currentItem->title),NotifyMessageFlag::NM_PROCESS|NotifyMessageFlag::NM_SHOWCANCEL);
            pool->update();
        }
    }
    QObject::disconnect(conn);
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Update Done"),NotifyMessageFlag::NM_HIDE);
}

void PlayList::setCurrentPlayTime(int playTime)
{
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    PlayListItem::PlayState lastState = currentItem->playTimeState;
    currentItem->playTime=playTime;
    const int ignoreLength = 15;
    int duration = GlobalObjects::mpvplayer->getDuration();
    if(playTime>duration-ignoreLength)
    {
        currentItem->playTimeState=PlayListItem::FINISH;//finished
    }
    else if(playTime<ignoreLength)//unplayed
    {
        if(currentItem->playTimeState!=PlayListItem::FINISH)
            currentItem->playTimeState=PlayListItem::UNPLAY;
    }
    else
    {
        currentItem->playTimeState=PlayListItem::UNFINISH;//playing
    }
    if(!currentItem->animeTitle.isEmpty() && currentItem->playTimeState==PlayListItem::FINISH)
    {
        if((d->saveFinishTimeOnce && lastState!=PlayListItem::FINISH) || !d->saveFinishTimeOnce)
            AnimeWorker::instance()->updateEpTime(currentItem->animeTitle, currentItem->path, true);
    }
    d->playListChanged=true;
    d->needRefresh=true;
    d->incModifyCounter();
}

void PlayList::addCurrentSub(const QString &subFile)
{
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    if(!currentItem->trackInfo)
    {
        currentItem->trackInfo = new ItemTrackInfo;
    }
    if(currentItem->trackInfo->subFiles.contains(subFile)) return;
    currentItem->trackInfo->subFiles.append(subFile);
    d->playListChanged=true;
    d->needRefresh=true;
    d->incModifyCounter();
}

void PlayList::clearCurrentSub()
{
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    if(!currentItem->trackInfo) return;
    currentItem->trackInfo->subFiles.clear();
    if(currentItem->trackInfo->subDelay == 0 && currentItem->trackInfo->audioFiles.isEmpty())
    {
        delete currentItem->trackInfo;
        currentItem->trackInfo = nullptr;
    }
    d->playListChanged=true;
    d->needRefresh=true;
    d->incModifyCounter();
}

void PlayList::setCurrentSubDelay(int delay)
{
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    if(!currentItem->trackInfo)
    {
        if(delay == 0) return;
        currentItem->trackInfo = new ItemTrackInfo;
    }
    currentItem->trackInfo->subDelay = delay;
    d->playListChanged=true;
    d->needRefresh=true;
    d->incModifyCounter();
}

void PlayList::setCurrentSubIndex(int index)
{
    if(index < 0) return;
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    if(!currentItem->trackInfo)
    {
        if(index == 0) return;
        currentItem->trackInfo = new ItemTrackInfo;
    }
    currentItem->trackInfo->subIndex = index;
    d->playListChanged=true;
    d->needRefresh=true;
    d->incModifyCounter();
}

void PlayList::addCurrentAudio(const QString &audioFile)
{
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    if(!currentItem->trackInfo)
    {
        currentItem->trackInfo = new ItemTrackInfo;
    }
    if(currentItem->trackInfo->audioFiles.contains(audioFile)) return;
    currentItem->trackInfo->audioFiles.append(audioFile);
    d->playListChanged=true;
    d->needRefresh=true;
    d->incModifyCounter();
}

void PlayList::clearCurrentAudio()
{
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    if(!currentItem->trackInfo) return;
    currentItem->trackInfo->audioFiles.clear();
    if(currentItem->trackInfo->subDelay == 0 && currentItem->trackInfo->subFiles.isEmpty())
    {
        delete currentItem->trackInfo;
        currentItem->trackInfo = nullptr;
    }
    d->playListChanged=true;
    d->needRefresh=true;
    d->incModifyCounter();
}

void PlayList::setCurrentAudioIndex(int index)
{
    if(index < 0) return;
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    if(!currentItem->trackInfo)
    {
        if(index == 0) return;
        currentItem->trackInfo = new ItemTrackInfo;
    }
    currentItem->trackInfo->audioIndex = index;
    d->playListChanged=true;
    d->needRefresh=true;
    d->incModifyCounter();
}

QModelIndex PlayList::mergeItems(const QModelIndexList &mergeIndexes)
{
    Q_D(PlayList);
    QList<PlayListItem *> items;
    int minLevel=INT_MAX;
    PlayListItem *mergeParent=d->root;
    int insertPosition=0;
    for(const QModelIndex &index : mergeIndexes)
    {
        if (!index.isValid())continue;
        PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
        items.append(item);
        if(item->level<minLevel)
        {
            minLevel=item->level;
            mergeParent=item->parent;
            insertPosition=mergeParent->children->indexOf(item);
        }
    }
    QModelIndex parentIndex;
    if (mergeParent != d->root)
    {
        int row=mergeParent->parent->children->indexOf(mergeParent);
        parentIndex= createIndex(row, 0, mergeParent);
    }
    beginInsertRows(parentIndex, insertPosition, insertPosition);
    PlayListItem *newParent=new PlayListItem(mergeParent,false,insertPosition);
    newParent->title = d->setCollectionTitle(items);
    endInsertRows();

    for(PlayListItem *curItem:items)
    {
        int cr_row = curItem->parent->children->indexOf(curItem);
        const QModelIndex &itemIndex = createIndex(cr_row, 0, curItem);
        beginRemoveRows(itemIndex.parent(), cr_row, cr_row);
        curItem->parent->children->removeAt(cr_row);
        curItem->parent=nullptr;
        endRemoveRows();
    }
	QModelIndex collectionIndex= createIndex(newParent->parent->children->indexOf(newParent), 0, newParent);
	beginInsertRows(collectionIndex, 0, items.count()-1);
    for(PlayListItem *curItem:items)
    {
        curItem->moveTo(newParent);
    }
	endInsertRows();
    d->playListChanged=true;
    return collectionIndex;
}

void PlayList::exportDanmuItems(const QModelIndexList &exportIndexes)
{
    QList<PlayListItem *> items;
    for(const QModelIndex &index : exportIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            items.append(item);
        }
    }
    Notifier *notifier = Notifier::getNotifier();
    while(!items.empty())
    {
        PlayListItem *currentItem=items.takeFirst();
        if(currentItem->children)
        {
            for(PlayListItem *child:*currentItem->children)
            {
                if(child->children || !child->poolID.isEmpty())
                {
                    items.push_back(child);
                }
            }
        }
        else if(!currentItem->poolID.isEmpty())
        {
            if(!currentItem->path.isEmpty())
            {
                notifier->showMessage(Notifier::LIST_NOTIFY, tr("Exporting: %1").arg(currentItem->title),NotifyMessageFlag::NM_PROCESS);
                QFileInfo fi(currentItem->path);
                QFileInfo dfi(fi.absolutePath(),fi.completeBaseName()+".xml");
                Pool *pool=GlobalObjects::danmuManager->getPool(currentItem->poolID);
                if(pool) pool->exportPool(dfi.absoluteFilePath());
            }
        }
    }
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Export Down"),NotifyMessageFlag::NM_HIDE);
}

void PlayList::dumpJsonPlaylist(QJsonDocument &jsonDoc)
{
    Q_D(PlayList);
    if(!d->needRefresh) return;
    QJsonDocument newDoc;
    QJsonArray rootArray;
    d->dumpItem(rootArray, d->root);
    newDoc.setArray(rootArray);
    jsonDoc.swap(newDoc);
    d->needRefresh=false;
}

QString PlayList::getPathByHash(const QString &hash)
{
    Q_D(PlayList);
    d->pathHashLock.lockForRead();
    QString path = d->mediaPathHash.value(hash);
    d->pathHashLock.unlock();
    return path;
}

const PlayListItem *PlayList::getPathItem(const QString &pathId)
{
    Q_D(PlayList);
    if(pathId == "0")
    {
        return d->root;
    }
    else
    {
        QStringList itemPosList = pathId.split("_");
        if(itemPosList.size() < 2 || itemPosList[0] != '0') return nullptr;
        itemPosList.pop_front();
        PlayListItem *item = d->root;
        for(const QString &sPos : itemPosList)
        {
            if(sPos.isEmpty()) return nullptr;
            bool ok = true;
            int pos = sPos.toInt(&ok);
            if(!ok) return nullptr;
            if(!item->children || item->children->size() <= pos) return nullptr;
            item = (*item->children)[pos];
        }
        return item;
    }
    return nullptr;
}

void PlayList::updatePlayTime(const QString &path, int time, PlayListItem::PlayState state)
{
    Q_D(PlayList);
    PlayListItem *item=d->fileItems.value(path,nullptr);
    if(item)
    {
        PlayListItem::PlayState lastState = item->playTimeState;
        item->playTime=time;
        item->playTimeState=state;
        QModelIndex cIndex = createIndex(item->parent->children->indexOf(item), 0, item);
        emit dataChanged(cIndex, cIndex);
        d->playListChanged=true;
        d->needRefresh=true;
        if(!item->animeTitle.isEmpty())
        {
            if(state==PlayListItem::PlayState::FINISH)
            {
                if((d->saveFinishTimeOnce && lastState!=PlayListItem::FINISH) || !d->saveFinishTimeOnce)
                    AnimeWorker::instance()->updateEpTime(item->animeTitle, item->path, true);
            }
            else
            {
                AnimeWorker::instance()->updateEpTime(item->animeTitle, item->path);
            }
        }
        d->updateRecentlist(item);
        d->incModifyCounter();
    }
}

void PlayList::renameItemPoolId(const QString &opid, const QString &npid)
{
    Q_D(PlayList);
    Pool *nPool = GlobalObjects::danmuManager->getPool(npid, false);
    if(!nPool) return;
    MatchResult match;
    match.success = true;
    match.name = nPool->animeTitle();
    match.ep = nPool->toEp();
    for(PlayListItem *item:d->fileItems)
    {
        if(item->poolID==opid)
        {
            item->poolID=npid;
            item->animeTitle=nPool->animeTitle();
            item->title=match.ep.toString();
            if (item == d->currentItem)
            {
                emit currentMatchChanged(item->poolID);
            }
            d->playListChanged=true;
            d->needRefresh=true;
            d->incModifyCounter();
            match.ep.localFile = item->path;
            AnimeWorker::instance()->addAnime(match);
        }
    }
}


void MatchWorker::match(const QVector<PlayListItem *> &items)
{
    QList<PlayListItem *> matchedItems;
    auto notifier = Notifier::getNotifier();
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Match Start"),NotifyMessageFlag::NM_PROCESS|NotifyMessageFlag::NM_SHOWCANCEL);
    bool cancel = false;
    auto conn = QObject::connect(notifier, &Notifier::cancelTrigger, [&](int nType){ if(nType & Notifier::LIST_NOTIFY) cancel=true;});
    for(auto currentItem: items)
    {
        if(cancel) break;
        if(currentItem->hasPool()) continue;
        if (!QFile::exists(currentItem->path))continue;
        MatchResult match;
        GlobalObjects::danmuManager->localMatch(currentItem->path, match);
        if(!match.success) GlobalObjects::animeProvider->match(GlobalObjects::animeProvider->defaultMatchScript(), currentItem->path, match);
        if(!match.success)
        {
            notifier->showMessage(Notifier::LIST_NOTIFY, tr("Failed: %1").arg(currentItem->title),NotifyMessageFlag::NM_PROCESS|NotifyMessageFlag::NM_SHOWCANCEL);
            continue;
        }
        currentItem->animeTitle=match.name;
        currentItem->title=match.ep.toString();
        currentItem->poolID=GlobalObjects::danmuManager->createPool(currentItem->path, match);
        matchedItems<<currentItem;
        notifier->showMessage(Notifier::LIST_NOTIFY, tr("Success: %1").arg(currentItem->title),NotifyMessageFlag::NM_PROCESS|NotifyMessageFlag::NM_SHOWCANCEL);
        AnimeWorker::instance()->addAnime(match);
    }
    QObject::disconnect(conn);
    emit matchDown(matchedItems);
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Match Done"),NotifyMessageFlag::NM_HIDE);
}

void MatchWorker::match(const QVector<PlayListItem *> &items, const QString &animeTitle, const QList<EpInfo> &eps)
{
    Q_ASSERT(items.size()==eps.size());
    QList<PlayListItem *> matchedItems;
    auto notifier = Notifier::getNotifier();
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Match Start"),NotifyMessageFlag::NM_PROCESS|NotifyMessageFlag::NM_SHOWCANCEL);
    bool cancel = false;
    auto conn = QObject::connect(notifier, &Notifier::cancelTrigger, [&](int nType){ if(nType & Notifier::LIST_NOTIFY) cancel=true;});
    for(int i=0; i<items.size(); ++i)
    {
        if(cancel) break;
        notifier->showMessage(Notifier::LIST_NOTIFY, tr("Success: %1").arg(eps[i].toString()),NotifyMessageFlag::NM_PROCESS|NotifyMessageFlag::NM_SHOWCANCEL);

        MatchResult match;
        match.success = true;
        match.name = animeTitle;
        match.ep = eps[i];

        items[i]->animeTitle=animeTitle;
        items[i]->title=eps[i].toString();
        items[i]->poolID=GlobalObjects::danmuManager->updateMatch(items[i]->path, match);

        matchedItems<<items[i];
        AnimeWorker::instance()->addAnime(match);
    }
    QObject::disconnect(conn);
    emit matchDown(matchedItems);
    notifier->showMessage(Notifier::LIST_NOTIFY, tr("Match Done"),NotifyMessageFlag::NM_HIDE);
}

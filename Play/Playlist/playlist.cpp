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
#include "Play/Danmu/Provider/matchprovider.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "MediaLibrary/animelibrary.h"

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

QList<QPair<QString, QString> > &PlayList::recent()
{
    Q_D(PlayList);
    return d->recentList;
}

int PlayList::addItems(QStringList &items, QModelIndex parent)
{
    Q_D(PlayList);
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
	QStringList tmpItems;
    for(auto iter=items.cbegin();iter!=items.cend();++iter)
    {
        if (!d->fileItems.contains(*iter))
			tmpItems.append(*iter);
    }
    if(tmpItems.count()==0)
    {
        emit message(tr("File exist or Unsupported format"), LPM_HIDE|LPM_INFO);
        return 0;
    }
    beginInsertRows(parent, insertPosition, insertPosition+ tmpItems.count()-1);
    for (const QString &item : tmpItems)
	{
        int suffixPos = item.lastIndexOf('.'), pathPos = item.lastIndexOf('/') + 1;
		QString title = item.mid(pathPos,suffixPos-pathPos);
        PlayListItem *newItem = new PlayListItem(parentItem, true, insertPosition++);
        newItem->title = title;
		newItem->path = item;
        d->fileItems.insert(newItem->path,newItem);
	}
	endInsertRows();
    d->playListChanged=true;
    d->needRefresh = true;
    emit message(tr("Add %1 item(s)").arg(tmpItems.size()),LPM_HIDE|LPM_OK);
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
    if (folderRootCollection.children->count())
	{
        beginInsertRows(parent, insertPosition, insertPosition);
        folderRootCollection.children->first()->moveTo(parentItem, insertPosition);
		endInsertRows();
        d->playListChanged=true;
	}
    folderRootCollection.children->clear();
    emit message(tr("Add %1 item(s)").arg(itemCount),ListPopMessageFlag::LPM_HIDE|ListPopMessageFlag::LPM_OK);
    return itemCount;
}

void PlayList::deleteItems(const QModelIndexList &deleteIndexes)
{
    QList<PlayListItem *> items;
    foreach (const QModelIndex &index, deleteIndexes)
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
	endInsertRows();
    d->playListChanged=true;
    d->needRefresh = true;
    return this->index(insertPosition,0,parent);
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
        if(item->children) return item->title;

        QStringList tipContent;

        if(!item->animeTitle.isEmpty())
            tipContent<<QString("%1-%2").arg(item->animeTitle).arg(item->title);
        else
            tipContent<<QString("%1").arg(item->title);
        tipContent<<item->path;
        if(item->playTimeState==0)
            tipContent<<tr("Unplayed");
        else if(item->playTimeState==2)
            tipContent<<tr("Finished");
        else
        {
            int cmin=item->playTime/60;
            int cls=item->playTime-cmin*60;
            tipContent<<(tr("PlayTo: %1:%2").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')));
        }

        return tipContent.join('\n');
    }
    case Qt::ForegroundRole:
    {
        if(item==d->currentItem) return QBrush(QColor(255,255,0));
        static QBrush brs[]={QBrush(QColor(220,220,220)),QBrush(QColor(160,200,200)),QBrush(QColor(140,140,140))};
        return brs[item->playTimeState];
    }
    case Qt::DecorationRole:
        return item==d->currentItem?QIcon(":/res/images/playing.png"):QVariant();
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
    if(!val.isEmpty())
    {
        item->title=val;
        emit dataChanged(index,index);
        d->playListChanged=true;
        d->needRefresh = true;
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
        emit message(tr("File Not Exist"),LPM_INFO|LPM_HIDE);
        return nullptr;
    }
    d->autoLocalMatch(cur);
    PlayListItem *tmp = d->currentItem;
    d->currentItem = cur;
	if (tmp)
	{
		QModelIndex nIndex = createIndex(tmp->parent->children->indexOf(tmp), 0, tmp);
		emit dataChanged(nIndex, nIndex);
	}
    d->updateRecentlist(cur);
    d->updateLibItemInfo(cur);
    return cur;
}

const PlayListItem *PlayList::setCurrentItem(const QString &path)
{
    Q_D(PlayList);
    PlayListItem *curItem=d->fileItems.value(path,nullptr);
    if(curItem && d->currentItem!=curItem)
    {
        d->autoLocalMatch(curItem);
        PlayListItem *tmp = d->currentItem;
        d->currentItem = curItem;
        if (tmp)
        {
            QModelIndex nIndex = createIndex(tmp->parent->children->indexOf(tmp), 0, tmp);
            emit dataChanged(nIndex, nIndex);
        }
        QModelIndex cIndex = createIndex(curItem->parent->children->indexOf(curItem), 0, curItem);
        emit dataChanged(cIndex, cIndex);
        d->updateRecentlist(curItem);
        d->updateLibItemInfo(curItem);
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
        d->autoLocalMatch(item);
        if (tmp)
        {
            QModelIndex nIndex(createIndex(tmp->parent->children->indexOf(tmp), 0, tmp));
            emit dataChanged(nIndex, nIndex);
        }
        QModelIndex nIndex(createIndex(item->parent->children->indexOf(item),0,item));
        emit dataChanged(nIndex,nIndex);
        d->updateRecentlist(item);
        d->updateLibItemInfo(item);
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
    if(!itemDeleted->path.isEmpty())d->fileItems.remove(itemDeleted->path);
    if(itemDeleted==d->currentItem)
    {
        d->currentItem=nullptr;
        emit currentInvaild();
    }
}

void PlayList::matchItems(const QModelIndexList &matchIndexes)
{
    Q_D(PlayList);
    QList<PlayListItem *> items;
    for(const QModelIndex &index : matchIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            items.append(item);
        }
    }
    emit message(tr("Match Start"),ListPopMessageFlag::LPM_PROCESS);
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
			if (!QFile::exists(currentItem->path))continue;
            MatchInfo *matchInfo=MatchProvider::MatchFromDandan(currentItem->path);
            if(!matchInfo) break;
            if(matchInfo->error)
            {
                emit message(tr("Failed: %1").arg(matchInfo->errorInfo),ListPopMessageFlag::LPM_PROCESS);
            }
            else
            {
                QList<MatchInfo::DetailInfo> &matchList=matchInfo->matches;
                if(matchInfo->success && matchList.count()>0)
                {
                    MatchInfo::DetailInfo &bestMatch=matchList.first();
                    emit message(tr("Success: %1").arg(currentItem->title),ListPopMessageFlag::LPM_PROCESS);
                    currentItem->animeTitle=bestMatch.animeTitle;
                    currentItem->title=bestMatch.title;
                    currentItem->poolID=matchInfo->poolID;
                    d->playListChanged = true;
					QModelIndex nIndex = createIndex(currentItem->parent->children->indexOf(currentItem), 0, currentItem);
					emit dataChanged(nIndex, nIndex);
                    if (currentItem == d->currentItem)
					{
						emit currentMatchChanged();
					}
                    GlobalObjects::library->addToLibrary(currentItem->animeTitle,currentItem->title,currentItem->path);
                }
                else
                {
                    emit message(tr("Need manually: %1").arg(currentItem->title),ListPopMessageFlag::LPM_PROCESS);
                }
            }
            delete matchInfo;
        }
    }
    emit message(tr("Match Done"),ListPopMessageFlag::LPM_HIDE|ListPopMessageFlag::LPM_OK);
    d->savePlaylist();
}

void PlayList::matchIndex(QModelIndex &index, MatchInfo *matchInfo)
{
    Q_D(PlayList);
    if(!index.isValid())return;
    PlayListItem *item=static_cast<PlayListItem *>(index.internalPointer());
    MatchInfo::DetailInfo &bestMatch=matchInfo->matches.first();
    item->animeTitle=bestMatch.animeTitle;
    item->title=bestMatch.title;
    item->poolID=MatchProvider::updateMatchInfo(item->path,matchInfo);
    d->playListChanged = true;
    d->needRefresh = true;
    emit message(tr("Success: %1").arg(item->title),ListPopMessageFlag::LPM_HIDE|ListPopMessageFlag::LPM_OK);
    emit dataChanged(index, index);
    if (item == d->currentItem)
    {
        emit currentMatchChanged();
    }
    GlobalObjects::library->addToLibrary(item->animeTitle,item->title,item->path);
    d->savePlaylist();

}

void PlayList::setCurrentPlayTime(int playTime)
{
    Q_D(PlayList);
    PlayListItem *currentItem=d->currentItem;
    if(!currentItem) return;
    currentItem->playTime=playTime;
    const int ignoreLength = 15;
    int duration = GlobalObjects::mpvplayer->getDuration();
    if(playTime>duration-ignoreLength)
    {
        currentItem->playTimeState=2;//finished
    }
    else if(playTime<ignoreLength)//unplayed
    {
        if(currentItem->playTimeState!=2)
            currentItem->playTimeState=0;
    }
    else
    {
        currentItem->playTimeState=1;//playing
    }
    d->playListChanged=true;
    d->needRefresh=true;
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
                emit message(tr("Exporting: %1").arg(currentItem->title),ListPopMessageFlag::LPM_PROCESS);
                QFileInfo fi(currentItem->path);
                QFileInfo dfi(fi.absolutePath(),fi.baseName()+".xml");
                GlobalObjects::danmuManager->exportPool(currentItem->poolID,dfi.absoluteFilePath());
            }
        }
    }
    emit message(tr("Export Down"),ListPopMessageFlag::LPM_HIDE|ListPopMessageFlag::LPM_OK);
}

void PlayList::dumpJsonPlaylist(QJsonDocument &jsonDoc, QHash<QString, QString> &mediaHash)
{
    Q_D(PlayList);
    if(!d->needRefresh) return;
    QJsonDocument newDoc;
    QJsonArray rootArray;
    QHash<QString, QString> newHash;
    d->dumpItem(rootArray,d->root,newHash);
    newDoc.setArray(rootArray);
    jsonDoc.swap(newDoc);
    mediaHash.swap(newHash);
    d->needRefresh=false;
}

void PlayList::updatePlayTime(const QString &path, int time, int state)
{
    Q_D(PlayList);
    PlayListItem *item=d->fileItems.value(path,nullptr);
    if(item)
    {
        item->playTime=time;
        item->playTimeState=state;
        QModelIndex cIndex = createIndex(item->parent->children->indexOf(item), 0, item);
        emit dataChanged(cIndex, cIndex);
        d->playListChanged=true;
        d->needRefresh=true;
        d->updateLibItemInfo(item);
        d->updateRecentlist(item);
    }
}


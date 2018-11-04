#include "playlist.h"

#include <QIODevice>
#include <QMimeData>
#include <QColor>
#include <QBrush>
#include <QFile>
#include <QXmlStreamReader>
#include <QCoreApplication>
#include <QDir>
#include <QRandomGenerator>
#include <QCollator>
#include <QSqlQuery>
#include <QSqlRecord>

#include "globalobjects.h"
#include "Play/Danmu/Provider/matchprovider.h"
#include "Play/Video/mpvplayer.h"
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
PlayList* PlayListItem::playlist=nullptr;

PlayList::PlayList(QObject *parent) : QAbstractItemModel(parent),currentItem(nullptr),playListChanged(false),
    loopMode(NO_Loop_All)
{
    comparer.setNumericMode(true);
    PlayListItem::playlist=this;
	loadRecentlist();
    loadPlaylist();
}

PlayList::~PlayList()
{
    savePlaylist();
    saveRecentlist();
}

QModelIndex PlayList::getCurrentIndex() const
{
    if(currentItem)
        return createIndex(currentItem->parent->children->indexOf(currentItem),0,currentItem);
    return QModelIndex();
}

int PlayList::addItems(QStringList &items, QModelIndex parent)
{
    int insertPosition(0);
	PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : &root;
	if(parentItem->children)
        insertPosition = parentItem->children->size();
	else
	{
        insertPosition = parentItem->parent->children->indexOf(parentItem) + 1;
		parentItem = parentItem->parent;
		parent = this->parent(parent);
	}
    QSet<QString> paths;
    addMediaItemPath(paths);
	QStringList tmpItems;
    for(auto iter=items.cbegin();iter!=items.cend();++iter)
    {
		if (!paths.contains(*iter))
			tmpItems.append(*iter);
    }
    if(tmpItems.count()==0)
    {
        emit message(tr("File exist or Unsupported format"),LPM_HIDE|LPM_INFO);
        return 0;
    }
    beginInsertRows(parent, insertPosition, insertPosition+ tmpItems.count()-1);
	for (const QString item : tmpItems)
	{
        int suffixPos = item.lastIndexOf('.'), pathPos = item.lastIndexOf('/') + 1;
		QString title = item.mid(pathPos,suffixPos-pathPos);
        PlayListItem *newItem = new PlayListItem(parentItem, true, insertPosition++);
        newItem->title = title;
		newItem->path = item;
	}
	endInsertRows();
    playListChanged=true;
    emit message(tr("Add %1 item(s)").arg(tmpItems.size()),LPM_HIDE|LPM_OK);
    return tmpItems.size();

}

void PlayList::addFolder(QString folderStr, QModelIndex parent)
{
    int insertPosition(0);
	PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : &root;
	if (parentItem->children)
        insertPosition = parentItem->children->size();
	else
	{
        insertPosition = parentItem->parent->children->indexOf(parentItem) + 1;
		parentItem = parentItem->parent;
		parent = this->parent(parent);
	}
	
    PlayListItem folderRootCollection;
    QSet<QString> paths;
    addMediaItemPath(paths);
    int itemCount=0;
    addSubFolder(folderStr, &folderRootCollection,paths,itemCount);
    if (folderRootCollection.children->count())
	{
        beginInsertRows(parent, insertPosition, insertPosition);
        folderRootCollection.children->first()->moveTo(parentItem, insertPosition);
		endInsertRows();
        playListChanged=true;
	}
    folderRootCollection.children->clear();
    emit message(tr("Add %1 item(s)").arg(itemCount),ListPopMessageFlag::LPM_HIDE|ListPopMessageFlag::LPM_OK);
}
bool PlayList::addSubFolder(QString folderStr, PlayListItem *parent,QSet<QString> &paths,int &itemCount)
{
	QDir folder(folderStr);
	bool containsVideoFile = false;
	PlayListItem *folderCollection = new PlayListItem();
	folderCollection->title = folder.dirName();
	for (QFileInfo fileInfo : folder.entryInfoList())
	{
		QString fileName = fileInfo.fileName();
		if (fileInfo.isFile())
		{
            if (GlobalObjects::mpvplayer->videoFileFormats.contains("*."+fileInfo.suffix().toLower()))
			{
                if(!paths.contains(fileInfo.filePath()))
                {
                    PlayListItem *newItem = new PlayListItem(folderCollection, true);
                    int suffixPos = fileName.lastIndexOf('.'), pathPos = fileName.lastIndexOf('/') + 1;
					QString title = fileName.mid(pathPos, suffixPos - pathPos);
                    newItem->title = title;
                    newItem->path = fileInfo.filePath();
                    containsVideoFile = true;
                    itemCount++;
                }
			}
		}
		else
		{
			if (fileName == "." || fileName == "..")continue;
            bool ret = addSubFolder(fileInfo.absoluteFilePath(), folderCollection,paths,itemCount);
			if (ret) containsVideoFile = true;
		}
	}
	if (containsVideoFile)
		folderCollection->moveTo(parent);
	else
		delete folderCollection;
    return containsVideoFile;
}

void PlayList::addMediaItemPath(QSet<QString> &paths)
{
    QList<PlayListItem *> items;
    items.push_back(&root);
    while(!items.empty())
    {
        PlayListItem *currentItem=items.front();
        items.pop_front();
        for(PlayListItem *child:*currentItem->children)
            if(child->children)
                items.push_back(child);
            else
                paths.insert(child->path);
    }
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
    playListChanged=true;
}

void PlayList::clear()
{
    if(root.children->count()==0)return;
    beginRemoveRows(QModelIndex(), 0, root.children->count()-1);
    for(PlayListItem *child:*root.children)
        delete child;
    root.children->clear();
    endRemoveRows();
    playListChanged=true;
}

void PlayList::sortItems(const QModelIndex &parent, bool ascendingOrder)
{
    PlayListItem *parentItem= parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : &root;
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
    playListChanged=true;
}

void PlayList::sortAllItems(bool ascendingOrder)
{
    QList<PlayListItem *> items;
    items.push_back(&root);
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
    playListChanged=true;
    emit layoutChanged();
}

QModelIndex PlayList::addCollection(QModelIndex parent, QString title)
{
    PlayListItem *newCollection(nullptr);
    PlayListItem *parentItem= parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : &root;
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
    playListChanged=true;
    return this->index(insertPosition,0,parent);
}

void PlayList::cutItems(const QModelIndexList &cutIndexes)
{
    if(!itemsClipboard.empty())
    {
        qDeleteAll(itemsClipboard);
        itemsClipboard.clear();
    }
    foreach (const QModelIndex &index, cutIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            itemsClipboard.append(item);
        }
    }
    std::sort(itemsClipboard.begin(),itemsClipboard.end(),[](const PlayListItem *item1,const PlayListItem *item2){return item1->level>item2->level;});
    for(PlayListItem *curItem:itemsClipboard)
    {
        int cr_row = curItem->parent->children->indexOf(curItem);
        const QModelIndex &itemIndex = createIndex(cr_row, 0, curItem);
        beginRemoveRows(itemIndex.parent(), cr_row, cr_row);
        curItem->parent->children->removeAt(cr_row);
        endRemoveRows();
    }
    playListChanged=true;
}

void PlayList::pasteItems(QModelIndex parent)
{
    if(itemsClipboard.count()==0)return;
    int insertPosition(0);
    PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem*>(parent.internalPointer()) : &root;
    if (parentItem->children)
        insertPosition = parentItem->children->size();
    else
    {
        insertPosition = parentItem->parent->children->indexOf(parentItem) + 1;
        parentItem = parentItem->parent;
        parent = this->parent(parent);
    }
    beginInsertRows(parent, insertPosition, insertPosition+itemsClipboard.count()-1);
    for (PlayListItem *item : itemsClipboard)
    {
        item->parent=nullptr;
        item->moveTo(parentItem,insertPosition++);
    }
    endInsertRows();
    playListChanged=true;
    itemsClipboard.clear();
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
    playListChanged=true;
}

QModelIndex PlayList::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

    const PlayListItem *parentItem;

    if (parent.isValid())
        parentItem = static_cast<PlayListItem*>(parent.internalPointer()); 
    else
        parentItem = &root;

	PlayListItem *childItem = parentItem->children->value(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

QModelIndex PlayList::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    PlayListItem *childItem = static_cast<PlayListItem*>(child.internalPointer());
    PlayListItem *parentItem = childItem->parent;

    if (parentItem == &root)
        return QModelIndex();
    int row=parentItem->parent->children->indexOf(parentItem);
    return createIndex(row, 0, parentItem);
}

int PlayList::rowCount(const QModelIndex &parent) const
{
    const PlayListItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (parent.isValid())
		parentItem = static_cast<PlayListItem*>(parent.internalPointer());
    else
		parentItem = &root;
    if(parentItem->children)
        return parentItem->children->size();
    return 0;
}

QVariant PlayList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());

    switch (role)
    {
    case Qt::EditRole:
        return item->title;
    case Qt::DisplayRole:
        return item->title;
    case Qt::ToolTipRole:
        if(item->children)
            return item->title;
        else
        {
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
        if(item==currentItem)
            return QBrush(QColor(255,255,0));
        else
        {
            if(item->playTimeState==0)
                return QBrush(QColor(220,220,220));
            else if(item->playTimeState==1)
                return QBrush(QColor(160,200,200));
            else
                return QBrush(QColor(140,140,140));
        }

    case Qt::DecorationRole:
        if(item==currentItem)
            return QIcon(":/res/images/playing.png");
    default:
        return QVariant();
    }
}

Qt::ItemFlags PlayList::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid())
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | Qt::ItemIsEditable | defaultFlags;
}

QStringList PlayList::mimeTypes() const
{
    QStringList types;
    types<<"application/x-kikoplaylistitem";
    return types;
}

QMimeData *PlayList::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    foreach (const QModelIndex &index, indexes)
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
    if (!data->hasFormat("application/x-kikoplaylistitem"))
        return false;
    if (action == Qt::IgnoreAction)
        return true;

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
	int beginRow;
	if (row != -1)
		beginRow = row;
	else if (parent.isValid())
		beginRow = rowCount(parent);
	else
		beginRow = rowCount(QModelIndex());
	PlayListItem *parentItem = parent.isValid() ? static_cast<PlayListItem *>(parent.internalPointer()) : &root;
    const QModelIndex *pParentIndex=&parent;
    if (!parentItem->children)
    {
        beginRow=parentItem->parent->children->indexOf(parentItem);
        parentItem=parentItem->parent;
        pParentIndex=&createIndex(parentItem->parent->children->indexOf(parentItem), 0, parentItem);
    }
    for (int cr = 0; cr < rows; ++cr)
	{
		PlayListItem *curItem = newItems[cr];
        PlayListItem *curParent=curItem->parent;
        int cr_row = curParent->children->indexOf(curItem);
		const QModelIndex &itemIndex = createIndex(cr_row, 0, curItem);
		beginRemoveRows(itemIndex.parent(), cr_row, cr_row);
        curParent->children->removeAt(cr_row);
		endRemoveRows();
        if (cr_row < beginRow && curParent==parentItem)
		{
			beginRow--;
		}
		if (parentItem->parent == curParent)
		{
			int parentRow = curParent->children->indexOf(parentItem);
			pParentIndex = &createIndex(parentRow, 0, parentItem);
		}
        beginInsertRows(*pParentIndex, beginRow, beginRow);
		curItem->parent = parentItem;
        curItem->setLevel(parentItem->level+1);
		parentItem->children->insert(beginRow++, curItem);
		endInsertRows();

	}
    playListChanged=true;
    return true;
}

bool PlayList::setData(const QModelIndex &index, const QVariant &value, int)
{
    PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
    QString val=value.toString();
    if(!val.isEmpty())
    {
        item->title=val;
        emit dataChanged(index,index);
        playListChanged=true;
        return true;
    }
    return false;

}

const PlayListItem *PlayList::setCurrentItem(const QModelIndex &index,bool playChild)
{
    PlayListItem *current = index.isValid() ? static_cast<PlayListItem *>(index.internalPointer()) : &root;
    PlayListItem *cur=current;
    if(playChild)
    {
        if(cur->children)
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
    }
    if(cur->children || cur==currentItem)
        return nullptr;
    QFileInfo fileInfo(cur->path);
    if(!fileInfo.exists())
    {
        emit message(tr("File Not Exist"),LPM_INFO|LPM_HIDE);
        return nullptr;
    }
    updateItemInfo(cur);
	PlayListItem *tmp = currentItem;
	currentItem = cur;
	if (tmp)
	{
		QModelIndex nIndex = createIndex(tmp->parent->children->indexOf(tmp), 0, tmp);
		emit dataChanged(nIndex, nIndex);
	}
    updateRecentlist();
    updateLibraryInfo(cur);
    return cur;
}

const PlayListItem *PlayList::setCurrentItem(const QString &path)
{
    QList<PlayListItem *> items;
    items.push_back(&root);
    while(!items.empty())
    {
        PlayListItem *currentItem=items.takeFirst();
        for(PlayListItem *child:*currentItem->children)
        {
            if(child->children)
            {
                items.push_back(child);
            }
            else
            {
                if(child->path==path)
                {
					if (this->currentItem && this->currentItem->path == child->path)
						return nullptr;
                    updateItemInfo(child);
                    PlayListItem *tmp = this->currentItem;
                    this->currentItem = child;
                    if (tmp)
                    {
                        QModelIndex nIndex = createIndex(tmp->parent->children->indexOf(tmp), 0, tmp);
                        emit dataChanged(nIndex, nIndex);
                    }
                    QModelIndex cIndex = createIndex(child->parent->children->indexOf(child), 0, child);
                    emit dataChanged(cIndex, cIndex);
                    updateRecentlist();
                    updateLibraryInfo(child);
                    return child;
                }
            }
        }
    }
	return nullptr;
}

void PlayList::cleanCurrentItem()
{
	if (currentItem)
	{
		QModelIndex cIndex = createIndex(currentItem->parent->children->indexOf(currentItem), 0, currentItem);
		currentItem = nullptr;
		emit dataChanged(cIndex, cIndex);
	}
}

PlayListItem *PlayList::getPrevOrNextItem(PlayList::LoopMode loopMode,bool prev)
{
    if(!currentItem)return nullptr;
    switch (loopMode) {
    case NO_Loop_All:
    case Loop_All:
    {
        PlayListItem *cur=currentItem;
        bool findAgain=false;
        int loopCounter=0;
        do{
            while (cur!=&root)
            {
                int row=cur->parent->children->indexOf(cur);
                int pos=prev?0:cur->parent->children->count()-1;
                if(row==pos)
                    cur=cur->parent;
                else
                {
                    PlayListItem *next=cur->parent->children->at(prev?row-1:row+1);
                    while(next->children && next->children->count()>0)
                        next=prev?next->children->last():next->children->first();
                    if(next->children)
                        cur=next;
                    else
                        return next;
                }
            }
            if(loopMode==Loop_All)
            {
                if(loopCounter>0)break;
                findAgain=true;
                loopCounter++;
                while(cur->children && cur->children->count()>0)
                    cur=prev?cur->children->last():cur->children->first();
                if (!cur->children)return cur;
            }
        }while(findAgain);
    }
        break;
    case Random:
    {
        QList<PlayListItem *> collectionItems,mediaItems;
        collectionItems.push_back(&root);
        while(!collectionItems.empty())
        {
            PlayListItem *currentItem=collectionItems.front();
            collectionItems.pop_front();
            for(PlayListItem *child:*currentItem->children)
                if(child->children)
                    collectionItems.push_back(child);
                else if(child!=currentItem)
                    mediaItems.push_back(child);
        }
        if(mediaItems.count()>0)
        {
            int i=QRandomGenerator::global()->bounded(mediaItems.count());
            return mediaItems[i];
        }
    }
        break;
    default:
        break;
    }
    return nullptr;
}

void PlayList::updateItemInfo(PlayListItem *item)
{
    if(item->poolID.isEmpty())
    {
        MatchInfo *matchInfo = MatchProvider::MatchFromDB(item->path);
        if(matchInfo)
        {
            item->animeTitle=matchInfo->matches.first().animeTitle;
            item->title=matchInfo->matches.first().title;
            item->poolID=matchInfo->poolID;
            playListChanged=true;
        }
    }
}

QString PlayList::setCollectionTitle(QList<PlayListItem *> &list)
{
    int minTitleLen=INT_MAX;
    PlayListItem *suffixItem=nullptr;
    for(PlayListItem *item:list)
    {
        if(!item->animeTitle.isEmpty())
            return item->animeTitle;
        if(item->title.length()<minTitleLen)
        {
            minTitleLen=item->title.length();
            suffixItem=item;
        }
    }
    if(list.count()<2)return QString("new collection");
    if(minTitleLen<5)return QString("new collection");
    std::function<int (const QString &,const QString &,int *)> kmpMatchLen
            = [](const QString &suffix,const QString &str,int *next)
    {
        int maxMatchLen=0,i=0,j=0;
        for (; i < str.length(); ++i)
        {
            while (j > -1 && str[i] != suffix[j])
                j = next[j];
            if (j == suffix.length() - 1)
                return j;
            j = j + 1;
            if(j>maxMatchLen)maxMatchLen=j;
        }
        return maxMatchLen;
    };
    std::function<int * (const QString &)> getNext
            =[](const QString &suffix)
    {
        int i = 0, j = -1;
        int *next = new int[suffix.length()];
        for (; i < suffix.length(); ++i) {
            next[i] = j;
            while (j > -1 && suffix[i] != suffix[j])
                j = next[j];
            j = j + 1;
        }
        return next;
    };
    Q_ASSERT(suffixItem!=nullptr);
    QString &title=suffixItem->title;
    QString matchStr;
    for(int i=minTitleLen;i>4;i--)
    {
        QString suffix(title.right(i));
        int *next=getNext(suffix);
        int tmpMaxMinMatch=INT_MAX;
        for(PlayListItem *item:list)
        {
            if(item!=suffixItem)
            {
                int len=kmpMatchLen(suffix,item->title,next);
                if(len<tmpMaxMinMatch)
                    tmpMaxMinMatch=len;
            }
        }
		delete[] next;
        if(tmpMaxMinMatch>matchStr.length())
            matchStr=suffix.left(tmpMaxMinMatch);
        if(tmpMaxMinMatch>i)
            break;
    }
    return matchStr.length()<5?QString("new collection"):matchStr;
}

void PlayList::updateLibraryInfo(PlayListItem *item)
{
    if(!item->poolID.isEmpty())
    {
        //GlobalObjects::library->addToLibrary(item->animeTitle,item->title,item->path);
        GlobalObjects::library->refreshEpPlayTime(item->animeTitle,item->path);
    }
}

const PlayListItem *PlayList::playPrevOrNext(bool prev)
{
    PlayListItem *item(nullptr);
    if(loopMode==Loop_One || loopMode==NO_Loop_One)
        item=getPrevOrNextItem(NO_Loop_All,prev);
    else
        item=getPrevOrNextItem(loopMode,prev);
    if(item)
    {
        PlayListItem *tmp = currentItem;
        currentItem = item;
        updateItemInfo(item);
        if (tmp)
        {
            QModelIndex nIndex(createIndex(tmp->parent->children->indexOf(tmp), 0, tmp));
            emit dataChanged(nIndex, nIndex);
        }
        QModelIndex nIndex(createIndex(currentItem->parent->children->indexOf(currentItem),0,currentItem));
        emit dataChanged(nIndex,nIndex);
        updateRecentlist();
        updateLibraryInfo(item);
        return item;
    }
    return nullptr;
}

void PlayList::checkCurrentItem(PlayListItem *itemDeleted)
{
    if(itemDeleted==currentItem)
    {
        currentItem=nullptr;
        emit currentInvaild();
    }
}

void PlayList::matchItems(const QModelIndexList &matchIndexes)
{
    QList<PlayListItem *> items;
    foreach (const QModelIndex &index, matchIndexes)
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
        PlayListItem *currentItem=items.front();
        items.pop_front();
        if(currentItem->children)
        {
            for(PlayListItem *child:*currentItem->children)
                items.push_back(child);
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
					playListChanged = true;
					QModelIndex nIndex = createIndex(currentItem->parent->children->indexOf(currentItem), 0, currentItem);
					emit dataChanged(nIndex, nIndex);
					if (currentItem == this->currentItem)
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
    savePlaylist();
}

void PlayList::matchCurrentItem(MatchInfo *matchInfo)
{
    if(!currentItem)return;
    MatchInfo::DetailInfo &bestMatch=matchInfo->matches.first();
    currentItem->animeTitle=bestMatch.animeTitle;
    currentItem->title=bestMatch.title;
    currentItem->poolID=MatchProvider::updateMatchInfo(currentItem->path,matchInfo);
    playListChanged = true;
    emit message(tr("Success: %1").arg(currentItem->title),ListPopMessageFlag::LPM_HIDE|ListPopMessageFlag::LPM_OK);
    QModelIndex nIndex = createIndex(currentItem->parent->children->indexOf(currentItem), 0, currentItem);
    emit dataChanged(nIndex, nIndex);
    emit currentMatchChanged();
    GlobalObjects::library->addToLibrary(currentItem->animeTitle,currentItem->title,currentItem->path);
    savePlaylist();
}

void PlayList::matchIndex(QModelIndex &index, MatchInfo *matchInfo)
{
    if(!index.isValid())return;
    PlayListItem *item=static_cast<PlayListItem *>(index.internalPointer());
    MatchInfo::DetailInfo &bestMatch=matchInfo->matches.first();
    item->animeTitle=bestMatch.animeTitle;
    item->title=bestMatch.title;
    item->poolID=MatchProvider::updateMatchInfo(item->path,matchInfo);
    playListChanged = true;
    emit message(tr("Success: %1").arg(item->title),ListPopMessageFlag::LPM_HIDE|ListPopMessageFlag::LPM_OK);
    emit dataChanged(index, index);
    if (item == this->currentItem)
    {
        emit currentMatchChanged();
    }
    GlobalObjects::library->addToLibrary(item->animeTitle,item->title,item->path);
    savePlaylist();

}

void PlayList::setCurrentPlayTime(int playTime)
{
    if(currentItem)
    {
        currentItem->playTime=playTime;
        int duration = GlobalObjects::mpvplayer->getDuration();
        if(playTime>duration-15)
            currentItem->playTimeState=2;//finished
        else if(playTime<15)//unplayed
        {
            if(currentItem->playTimeState!=2)
                currentItem->playTimeState=0;
        }
        else
            currentItem->playTimeState=1;//playing
        playListChanged=true;
    }
}

QModelIndex PlayList::mergeItems(const QModelIndexList &mergeIndexes)
{
    QList<PlayListItem *> items;
    int minLevel=INT_MAX;
    PlayListItem *mergeParent=&root;
    int insertPosition=0;
    foreach (const QModelIndex &index, mergeIndexes)
    {
        if (index.isValid())
        {
            PlayListItem *item = static_cast<PlayListItem*>(index.internalPointer());
            items.append(item);
            if(item->level<minLevel)
            {
                minLevel=item->level;
                mergeParent=item->parent;
                insertPosition=mergeParent->children->indexOf(item);
            }
        }
    }
    QModelIndex parentIndex;
    if (mergeParent != &root)
    {
        int row=mergeParent->parent->children->indexOf(mergeParent);
        parentIndex= createIndex(row, 0, mergeParent);
    }
    beginInsertRows(parentIndex, insertPosition, insertPosition);
    PlayListItem *newParent=new PlayListItem(mergeParent,false,insertPosition);
    newParent->title = setCollectionTitle(items);
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
    playListChanged=true;
	return collectionIndex;
}

void PlayList::loadPlaylist()
{
    QFile playlistFile(QCoreApplication::applicationDirPath()+"\\playlist.xml");
    bool ret=playlistFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamReader reader(&playlistFile);
    QList<PlayListItem *> parents;
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare("select PoolID from bangumi where PoolID=?");
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name=="playlist")
                parents.push_back(&root);
            else if(name=="collection")
            {
                PlayListItem *collection=new PlayListItem(parents.last(),false);
                collection->title=reader.attributes().value("title").toString();
                parents.push_back(collection);
            }
            else if(name=="item")
            {
				QString title= reader.attributes().value("title").toString();
                QString animeTitle= reader.attributes().value("animeTitle").toString();
                QString poolID=reader.attributes().value("poolID").toString();
                int playTime=reader.attributes().value("playTime").toInt();
                int playTimeState=reader.attributes().value("playTimeState").toInt();
                QString path = reader.readElementText().trimmed();
                QFileInfo fileInfo(path);
                if(fileInfo.exists())
                {
                    PlayListItem *item=new PlayListItem(parents.last(),true);
                    item->title=title;
                    item->path= path;
                    item->playTime=playTime;
                    item->playTimeState=playTimeState;
                    if(!animeTitle.isEmpty())item->animeTitle=animeTitle;
					if (!poolID.isEmpty())
					{
                        query.bindValue(0,poolID);
                        query.exec();
						if (query.first())item->poolID = poolID;
						else playListChanged = true;
					}
                    for(auto &pair :recentList)
                    {
                        if(pair.first==item->path)
                        {
                            pair.second=item->animeTitle.isEmpty()?item->title:QString("%1 %2").arg(item->animeTitle).arg(item->title);
                            break;
                        }
                    }
                }
                else
                    playListChanged=true;
            }
        }
        if(reader.isEndElement())
        {
            QStringRef name=reader.name();
            if(name=="playlist")
                break;
            else if(name=="collection")
                parents.pop_back();
        }
        reader.readNext();
    }
    for(auto iter= recentList.begin();iter!=recentList.end();)
    {
        if((*iter).second.isEmpty())
            iter=recentList.erase(iter);
        else
            iter++;
    }
    playlistFile.close();
}

void PlayList::loadRecentlist()
{
    QFile recentlistFile(QCoreApplication::applicationDirPath()+"\\recent.xml");
    bool ret=recentlistFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamReader reader(&recentlistFile);
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name=="item")
            {
                recentList.append(QPair<QString,QString>(reader.readElementText().trimmed(),QString()));
            }
        }
        reader.readNext();
    }
    recentlistFile.close();
}

void PlayList::saveRecentlist()
{
    QFile recentlistFile(QCoreApplication::applicationDirPath()+"\\recent.xml");
    bool ret=recentlistFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&recentlistFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("recentlist");
    for(auto &pair :recentList)
    {
        writer.writeStartElement("item");
        writer.writeCharacters(pair.first);
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndDocument();
    recentlistFile.close();
}

void PlayList::savePlaylist()
{
    if(!playListChanged)return;
    QFile playlistFile(QCoreApplication::applicationDirPath()+"\\playlist.xml");
    bool ret=playlistFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&playlistFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    saveItem(writer,&root);
    writer.writeEndDocument();
    playlistFile.close();
}

void PlayList::updateRecentlist()
{
    if(!currentItem)return;
    for(auto iter= recentList.begin();iter!=recentList.end();)
    {
        if((*iter).first==currentItem->path)
            iter=recentList.erase(iter);
        else
            iter++;
    }
    recentList.push_front(QPair<QString,QString>(currentItem->path,currentItem->animeTitle.isEmpty()?currentItem->title:QString("%1 %2").arg(currentItem->animeTitle).arg(currentItem->title)));
    if(recentList.count()>maxRecentItems)
        recentList.pop_back();
}

void PlayList::saveItem(QXmlStreamWriter &writer, PlayListItem *item)
{
    if(item==&root)
    {
        writer.writeStartElement("playlist");

    }
    else if(item->children)
    {
        writer.writeStartElement("collection");
        writer.writeAttribute("title",item->title);
    }
    for(PlayListItem *child:*item->children)
    {
        if(child->children)
            saveItem(writer,child);
        else
        {
            writer.writeStartElement("item");
            writer.writeAttribute("title", child->title);
            if(!child->animeTitle.isEmpty())
                writer.writeAttribute("animeTitle",child->animeTitle);
            if(!child->poolID.isEmpty())
                writer.writeAttribute("poolID",child->poolID);
            writer.writeAttribute("playTime",QString::number(child->playTime));
            writer.writeAttribute("playTimeState",QString::number(child->playTimeState));
            writer.writeCharacters(child->path);
            writer.writeEndElement();
        }
    }
    writer.writeEndElement();
}

PlayListItem::PlayListItem(PlayListItem *parent, bool leaf, int insertPosition):children(nullptr),level(0),playTimeState(0)
{
    this->parent=parent;
    if(!leaf)
    {
        children=new QList<PlayListItem *>();
    }
    if(parent)
    {
        if (insertPosition == -1)
            parent->children->append(this);
        else
            parent->children->insert(insertPosition, this);
        level=parent->level+1;
    }
}

PlayListItem::~PlayListItem()
{
    playlist->checkCurrentItem(this);
    if(children)
    {
        qDeleteAll(children->begin(),children->end());
        delete children;
    }
}

void PlayListItem::setLevel(int newLevel)
{
    level=newLevel;
    if(children)
    {
        for(PlayListItem *child:*children)
            child->setLevel(newLevel+1);
    }
}

void PlayListItem::moveTo(PlayListItem *newParent, int insertPosition)
{
    if (parent)
        parent->children->removeAll(this);
    if (newParent)
    {
        if (insertPosition == -1)
            newParent->children->append(this);
        else
            newParent->children->insert(insertPosition, this);
        setLevel(newParent->level + 1);
    }
    parent = newParent;
}

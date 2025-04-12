#include "managermodel.h"
#include "globalobjects.h"
#include "danmumanager.h"
#include "../danmupool.h"
#include "pool.h"
DanmuManagerModel::DanmuManagerModel(QObject *parent) : QAbstractItemModel(parent)
{

}

DanmuManagerModel::~DanmuManagerModel()
{
    qDeleteAll(animeNodeList);
}

void DanmuManagerModel::refreshList()
{
    beginResetModel();
    GlobalObjects::danmuManager->loadPoolInfo(animeNodeList);
    endResetModel();
}

void DanmuManagerModel::exportPool(const QString &dir, bool useTimeline, bool applyBlockRule)
{
    GlobalObjects::danmuManager->exportPool(animeNodeList,dir,useTimeline,applyBlockRule);
}

void DanmuManagerModel::exportKdFile(const QString &dir, const QString &comment)
{
    GlobalObjects::danmuManager->exportKdFile(animeNodeList,dir,comment);
}

void DanmuManagerModel::deletePool()
{
    GlobalObjects::danmuManager->deletePool(animeNodeList);
    QList<DanmuPoolNode *> nodes(animeNodeList);
    while(!nodes.empty())
    {
        DanmuPoolNode *cur=nodes.takeFirst();
        switch (cur->checkStatus)
        {
        case Qt::Unchecked:
        {
            break;
        }
        case Qt::Checked:
        {
            int row;
            QModelIndex parent;
            QList<DanmuPoolNode *> *pList;
            if(cur->type==DanmuPoolNode::AnimeNode)
            {
                row=animeNodeList.indexOf(cur);
                parent=QModelIndex();
                pList=&animeNodeList;
            }
            else
            {
                row=cur->parent->children->indexOf(cur);
                parent=this->parent(createIndex(row,0,cur));
                pList=cur->parent->children;

                DanmuPoolNode *tp=cur->parent;
                QModelIndex pIndex=parent;
                do
                {
                    tp->danmuCount-=cur->danmuCount;
                    emit dataChanged(pIndex.siblingAtColumn((int)Columns::COUNT),pIndex.siblingAtColumn((int)Columns::COUNT));
                    tp=tp->parent;
                    pIndex=pIndex.parent();
                }while(tp);
            }
            beginRemoveRows(parent,row,row);
            pList->removeAt(row);
            delete cur;
            endRemoveRows();
            break;
        }
        case Qt::PartiallyChecked:
        {
            for(DanmuPoolNode *child : *cur->children)
            {
                if(child->checkStatus!=Qt::Unchecked)
                {
                    nodes.append(child);
                }
            }
            break;
        }
        }
    }
}

void DanmuManagerModel::updatePool()
{
    GlobalObjects::danmuManager->updatePool(animeNodeList);
    for(DanmuPoolNode *animeNode:animeNodeList)
    {
        if(animeNode->checkStatus==Qt::Unchecked) continue;
        animeNode->setCount();
        QModelIndex aIndex=this->index(animeNodeList.indexOf(animeNode),(int)Columns::COUNT,QModelIndex());
        emit dataChanged(aIndex,aIndex);
        for(DanmuPoolNode *epNode:*animeNode->children)
        {
            if(epNode->checkStatus==Qt::Unchecked) continue;
            QModelIndex eIndex=this->index(animeNode->children->indexOf(epNode),(int)Columns::COUNT,aIndex);
            emit dataChanged(eIndex,eIndex);
            for(DanmuPoolNode *sourceNode:*epNode->children)
            {
                if(sourceNode->checkStatus==Qt::Unchecked) continue;
                QModelIndex sIndex=this->index(epNode->children->indexOf(sourceNode),(int)Columns::COUNT,eIndex);
                emit dataChanged(sIndex,sIndex);
            }
        }
    }
}

void DanmuManagerModel::setDelay(int delay)
{
    GlobalObjects::danmuManager->setPoolDelay(animeNodeList, delay*1000);
    for(DanmuPoolNode *animeNode:animeNodeList)
    {
        if(animeNode->checkStatus==Qt::Unchecked) continue;
        QModelIndex aIndex=this->index(animeNodeList.indexOf(animeNode),(int)Columns::DELAY,QModelIndex());
        for(DanmuPoolNode *epNode:*animeNode->children)
        {
            if(epNode->checkStatus==Qt::Unchecked) continue;
            QModelIndex eIndex=this->index(animeNode->children->indexOf(epNode),(int)Columns::DELAY,aIndex);
            for(DanmuPoolNode *sourceNode:*epNode->children)
            {
                if(sourceNode->checkStatus==Qt::Unchecked) continue;
                QModelIndex sIndex=this->index(epNode->children->indexOf(sourceNode),(int)Columns::DELAY,eIndex);
                emit dataChanged(sIndex,sIndex);
            }
        }
    }
}

int DanmuManagerModel::totalDanmuCount()
{
    int sum=0;
    for(DanmuPoolNode *node:animeNodeList)
    {
        sum+=node->danmuCount;
    }
    return sum;
}

int DanmuManagerModel::totalPoolCount()
{
    int sum=0;
    for(DanmuPoolNode *node:animeNodeList)
    {
        sum+=node->children->count();
    }
    return sum;
}

bool DanmuManagerModel::hasSelected()
{
    for(DanmuPoolNode *node:animeNodeList)
    {
        if(node->checkStatus!=Qt::Unchecked)
        {
            return true;
        }
    }
    return false;
}

DanmuPoolSourceNode *DanmuManagerModel::getSourceNode(const QModelIndex &index)
{
     DanmuPoolNode *item = static_cast<DanmuPoolNode*>(index.internalPointer());
     if(item->type!=DanmuPoolNode::SourecNode) return nullptr;
     return static_cast<DanmuPoolSourceNode *>(item);
}

DanmuPoolNode *DanmuManagerModel::getPoolNode(const QModelIndex &index)
{
    DanmuPoolNode *item = static_cast<DanmuPoolNode*>(index.internalPointer());
    if(item->type==DanmuPoolNode::AnimeNode) return nullptr;
    else if(item->type==DanmuPoolNode::EpNode) return item;
    else return item->parent;
}

QString DanmuManagerModel::getAnime(const QModelIndex &index)
{
    DanmuPoolNode *item = static_cast<DanmuPoolNode*>(index.internalPointer());
    if(item->type==DanmuPoolNode::AnimeNode) return item->title;
    else if(item->type==DanmuPoolNode::EpNode) return item->parent->title;
    else return item->parent->parent->title;
}

void DanmuManagerModel::addSrcNode(DanmuPoolNode *epNode, DanmuPoolSourceNode *srcNode)
{
    QModelIndex epIndex(createIndex(epNode->parent->children->indexOf(epNode),0,epNode));
    if(srcNode)
    {
        beginInsertRows(epIndex,epNode->children->count(),epNode->children->count());
        epNode->children->append(srcNode);
        srcNode->parent=epNode;
        endInsertRows();
    }
    epNode->parent->setCount();
    emit dataChanged(epIndex.siblingAtColumn(3),epIndex.siblingAtColumn(3));
    emit dataChanged(epIndex.parent().siblingAtColumn(3),epIndex.parent().siblingAtColumn(3));
}

void DanmuManagerModel::addPoolNode(const QString &animeTitle, const EpInfo &ep, const QString &pid)
{
    int i=0;
    for(DanmuPoolNode *node:animeNodeList)
    {
        if(node->title==animeTitle)
        {
            DanmuPoolEpNode *epNode=new DanmuPoolEpNode(ep, node);
            epNode->idInfo=pid;
            QModelIndex animeIndex(createIndex(i,0,node));
            beginInsertRows(animeIndex,node->children->count()-1,node->children->count()-1);
            endInsertRows();
            return;
        }
        ++i;
    }
    DanmuPoolNode *animeNode=new DanmuPoolNode(DanmuPoolNode::AnimeNode);
    animeNode->title=animeTitle;
    DanmuPoolEpNode *epNode=new DanmuPoolEpNode(ep, animeNode);
    epNode->idInfo=pid;
    beginInsertRows(QModelIndex(),i,i);
    animeNodeList.append(animeNode);
    endInsertRows();
}

void DanmuManagerModel::renamePoolNode(DanmuPoolNode *epNode, const QString &animeTitle, const QString &epTitle, const QString &pid)
{
    epNode->idInfo=pid;
    epNode->title=epTitle;
    if(animeTitle==epNode->parent->title)
    {
        QModelIndex epIndex(createIndex(epNode->parent->children->indexOf(epNode),0,epNode));
        emit dataChanged(epIndex,epIndex);
    }
    else
    {
        DanmuPoolNode *oldAnimeNode=epNode->parent;
        oldAnimeNode->danmuCount-=epNode->danmuCount;
        QModelIndex animeIndex(createIndex(animeNodeList.indexOf(oldAnimeNode),0,oldAnimeNode));
        int row=oldAnimeNode->children->indexOf(epNode);
        beginRemoveRows(animeIndex,row,row);
        oldAnimeNode->children->removeAt(row);
        endRemoveRows();
        if(oldAnimeNode->children->isEmpty())
        {
            int row=animeNodeList.indexOf(oldAnimeNode);
            beginRemoveRows(QModelIndex(),row,row);
            animeNodeList.removeAt(row);
            delete oldAnimeNode;
            endRemoveRows();
        }
        else
        {
            emit dataChanged(animeIndex.siblingAtColumn(3),animeIndex.siblingAtColumn(3));
        }

        DanmuPoolNode *animeNode(nullptr);
        int index=0;
        for(DanmuPoolNode *node:animeNodeList)
        {
            if(node->title==animeTitle)
            {
                animeNode=node;
                break;
            }
            ++index;
        }
        if(!animeNode)
        {
            animeNode=new DanmuPoolNode(DanmuPoolNode::AnimeNode);
            animeNode->title=animeTitle;
            epNode->parent=animeNode;
            animeNode->children->append(epNode);
            animeNode->danmuCount=epNode->danmuCount;
            beginInsertRows(QModelIndex(),animeNodeList.count(),animeNodeList.count());
            animeNodeList.append(animeNode);
            endInsertRows();
        }
        else
        {
            QModelIndex nAnimeIndex(createIndex(index,0,animeNode));
            epNode->parent=animeNode;
            animeNode->danmuCount+=epNode->danmuCount;
            beginInsertRows(nAnimeIndex,animeNode->children->count(),animeNode->children->count());
            animeNode->children->append(epNode);
            endInsertRows();
            emit dataChanged(nAnimeIndex.siblingAtColumn(3),nAnimeIndex.siblingAtColumn(3));
        }

    }
}

void DanmuManagerModel::setCheckable(bool on)
{
    nodeCheckAble = on;
    emit dataChanged(index(0, 0, QModelIndex()), index(animeNodeList.size() - 1, 0, QModelIndex()), QVector<int>{Qt::CheckStateRole});
}

void DanmuManagerModel::refreshChildrenCheckStatus(const QModelIndex &index)
{
    QList<QModelIndex> pIndexes;
    pIndexes.append(index);
    while (!pIndexes.isEmpty())
    {
        QModelIndex pIndex(pIndexes.takeFirst());
        DanmuPoolNode *parentItem = static_cast<DanmuPoolNode *>(pIndex.internalPointer());
        if(parentItem->children)
        {
            for(int i=0;i<parentItem->children->count();++i)
            {
                QModelIndex cIndex=this->index(i,0,pIndex);
                emit dataChanged(cIndex,cIndex);
                if(static_cast<DanmuPoolNode *>(cIndex.internalPointer())->children)
                    pIndexes.append(cIndex);
            }
        }
    }
}

void DanmuManagerModel::refreshParentCheckStatus(const QModelIndex &index)
{
    QModelIndex parentIndex(index.parent());
    while(parentIndex.isValid())
    {
        emit dataChanged(parentIndex,parentIndex);
        parentIndex=parentIndex.parent();
    }
}

QModelIndex DanmuManagerModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    DanmuPoolNode *childItem=nullptr;
    if(parent.isValid())
    {
        const DanmuPoolNode *parentItem = static_cast<DanmuPoolNode *>(parent.internalPointer());
        childItem = parentItem->children->value(row);
    }
    else
    {
        childItem = animeNodeList.value(row);
    }
    return childItem?createIndex(row,column,childItem):QModelIndex();
}

QModelIndex DanmuManagerModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();
    DanmuPoolNode *childItem = static_cast<DanmuPoolNode*>(child.internalPointer());
    DanmuPoolNode *parentItem = childItem->parent;
    if (!parentItem) return QModelIndex();
    int row=parentItem->parent?parentItem->parent->children->indexOf(parentItem):animeNodeList.indexOf(parentItem);
    return createIndex(row, 0, parentItem);
}

int DanmuManagerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    const DanmuPoolNode *parentItem;
    if (parent.isValid())
        parentItem = static_cast<DanmuPoolNode *>(parent.internalPointer());
    else
        return animeNodeList.size();
    return parentItem->children?parentItem->children->size():0;
}

QVariant DanmuManagerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    DanmuPoolNode *item = static_cast<DanmuPoolNode*>(index.internalPointer());
    Columns col = Columns(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if (col == Columns::TITLE)
            return item->title;
        else if (col == Columns::SOURCE && item->type == DanmuPoolNode::SourecNode)
            return item->idInfo;
        else if (col == Columns::DELAY && item->type == DanmuPoolNode::SourecNode)
            return static_cast<DanmuPoolSourceNode *>(item)->delay/1000;
        else if (col == Columns::COUNT)
            return item->danmuCount;
        break;
    }
    case Qt::ToolTipRole:
    {
        if (col == Columns::TITLE)
            return item->title;
        else if (col == Columns::DELAY && item->type == DanmuPoolNode::SourecNode)
        {
            if (static_cast<DanmuPoolSourceNode *>(item)->hasTimeline)
            {
                return tr("Danmu Source with Timeline Adjustment");
            }
        }
        break;
    }
    case Qt::EditRole:
    {
        if (col == Columns::DELAY && item->type == DanmuPoolNode::SourecNode)
            return static_cast<DanmuPoolSourceNode *>(item)->delay/1000;
        break;
    }
    case Qt::DecorationRole:
    {
        if (col == Columns::DELAY && item->type == DanmuPoolNode::SourecNode)
        {
            if (static_cast<DanmuPoolSourceNode *>(item)->hasTimeline)
            {
                static QIcon timeLineTip = GlobalObjects::context()->getFontIcon(QChar(0xe6b5), QColor(220, 220, 220));
                return timeLineTip;
            }
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        if (col == Columns::TITLE && nodeCheckAble)
            return item->checkStatus;
        break;
    }
    }
    return QVariant();
}

QVariant DanmuManagerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size())return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags DanmuManagerModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid() && index.column()==0 && nodeCheckAble)
    {
        return  Qt::ItemIsUserCheckable | defaultFlags;
    }
    else if(index.column()==2)
    {
        DanmuPoolNode *item = static_cast<DanmuPoolNode *>(index.internalPointer());
        if(item->type==DanmuPoolNode::SourecNode)
            return Qt::ItemIsEditable | defaultFlags;
    }
    return defaultFlags;
}

bool DanmuManagerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    DanmuPoolNode *item = static_cast<DanmuPoolNode *>(index.internalPointer());
    if(role==Qt::CheckStateRole && index.column()==0)
    {
        item->checkStatus=Qt::CheckState(value.toInt());
        item->setChildrenCheckStatus();
        item->setParentCheckStatus();
        refreshChildrenCheckStatus(index);
        refreshParentCheckStatus(index);
        return true;
    }
    else if(index.column()==2 && item->type==DanmuPoolNode::SourecNode)
    {
        bool isInt;
        int newDelay = value.toInt(&isInt)*1000;
        if(isInt)
        {
            DanmuPoolSourceNode *srcNode=static_cast<DanmuPoolSourceNode *>(item);
            Pool *pool=GlobalObjects::danmuManager->getPool(srcNode->parent->idInfo,false);
            if(pool && pool->setDelay(srcNode->srcId,newDelay))
            {
                srcNode->delay=newDelay;
                return true;
            }
        }
    }
    return false;
}

bool PoolSortProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if(sortColumn()==0)
        return comparer.compare(source_left.data(sortRole()).toString(),source_right.data(sortRole()).toString())<0;
    return QSortFilterProxyModel::lessThan(source_left,source_right);
}

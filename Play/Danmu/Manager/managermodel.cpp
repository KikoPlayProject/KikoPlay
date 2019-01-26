#include "managermodel.h"
#include "globalobjects.h"
#include "danmumanager.h"
#include "../danmupool.h"
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
                    emit dataChanged(pIndex.siblingAtColumn(3),pIndex.siblingAtColumn(3));
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
        QModelIndex aIndex=this->index(animeNodeList.indexOf(animeNode),3,QModelIndex());
        emit dataChanged(aIndex,aIndex);
        for(DanmuPoolNode *epNode:*animeNode->children)
        {
            if(epNode->checkStatus==Qt::Unchecked) continue;
            QModelIndex eIndex=this->index(animeNode->children->indexOf(epNode),3,aIndex);
            emit dataChanged(eIndex,eIndex);
            for(DanmuPoolNode *sourceNode:*epNode->children)
            {
                if(sourceNode->checkStatus==Qt::Unchecked) continue;
                QModelIndex sIndex=this->index(epNode->children->indexOf(sourceNode),3,eIndex);
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
    int col=index.column();
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col==0)
            return item->title;
        else if(col==1 && item->type==DanmuPoolNode::SourecNode)
            return item->idInfo;
        else if(col==2 && item->type==DanmuPoolNode::SourecNode)
            return static_cast<DanmuPoolSourceNode *>(item)->delay/1000;
        else if(col==3)
            return item->danmuCount;
        break;
    }
    case Qt::EditRole:
    {
        if(col==2 && item->type==DanmuPoolNode::SourecNode)
            return static_cast<DanmuPoolSourceNode *>(item)->delay/1000;
        break;
    }
    case Qt::CheckStateRole:
    {
        if(col==0)
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
    if (index.isValid() && index.column()==0)
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
        emit dataChanged(index,index);
        return true;
    }
    else if(index.column()==2 && item->type==DanmuPoolNode::SourecNode)
    {
        bool isInt;
        int newDelay = value.toInt(&isInt)*1000;
        if(isInt)
        {
            DanmuPoolSourceNode *srcNode=static_cast<DanmuPoolSourceNode *>(item);
            srcNode->delay=newDelay;
            if(GlobalObjects::danmuPool->getPoolID()==srcNode->parent->idInfo)
            {
                GlobalObjects::danmuPool->setDelay(&GlobalObjects::danmuPool->getSources()[srcNode->srcId],newDelay);
            }
            else
            {
                GlobalObjects::danmuManager->updateSourceDelay(srcNode);
            }
            emit dataChanged(index,index);
            return true;
        }
    }
    return false;
}

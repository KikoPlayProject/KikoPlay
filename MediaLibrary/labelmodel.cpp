#include "labelmodel.h"
#include "animeworker.h"
#include "globalobjects.h"
#include <QBrush>
#define TYPELEVEL 1
#define LABEL_YEAR 2
#define LABEL_MONTH 3
#define LABEL_TAG 4
#define CountRole Qt::UserRole+1
#define TypeRole Qt::UserRole+2
LabelModel::LabelModel(QObject *parent) : QAbstractItemModel(parent)
{
    root=new TagNode();
    QObject::connect(AnimeWorker::instance(),&AnimeWorker::addTagsTo,this,[this](const QString &title, const QStringList &tags){
        TagNode *tagCollectionNode(root->subNodes->at(1));
        for(const QString &tag:tags)
        {
            if(!tagMap.contains(tag))
            {
                tagMap.insert(tag,QSet<QString>({title}));
                int insPos = tagCollectionNode->subNodes?tagCollectionNode->subNodes->size():0;
                beginInsertRows(createIndex(1,0,tagCollectionNode),insPos,insPos);
                new TagNode(tag, tagCollectionNode, 1, LABEL_TAG);
                endInsertRows();
            }
            else
            {
                if(tagMap[tag].contains(title)) continue;
                tagMap[tag].insert(title);
                for(int i=0;i<tagCollectionNode->subNodes->count(); ++i)
                {
                    TagNode *tNode(tagCollectionNode->subNodes->at(i));
                    if(tNode->tagTitle==tag)
                    {
                        tNode->animeCount++;
                        QModelIndex index(createIndex(i,0,tNode));
                        emit dataChanged(index,index);
                        break;
                    }
                }
            }
        }
        AnimeWorker::instance()->saveTags(title,tags);
    });
    QObject::connect(AnimeWorker::instance(),&AnimeWorker::addTimeLabel,this,[this](const QString &time, const QString &oldTime){
        QString year(time.left(4)), month(time.right(2));
        TagNode *airDateNode(root->subNodes->at(0));
        auto iter = std::find_if(airDateNode->subNodes->begin(), airDateNode->subNodes->end(),[year](TagNode *node){return node->tagTitle==year;});
        if(iter!=airDateNode->subNodes->end()) //The year already exists
        {
            TagNode *yearNode = *iter;
            QModelIndex yIndex(createIndex(iter - airDateNode->subNodes->begin(),0,yearNode));
            auto mIter = std::find_if(yearNode->subNodes->begin(), yearNode->subNodes->end(),[month](TagNode *node){return node->tagTitle==month;});
            if(mIter==yearNode->subNodes->end())
            {
                beginInsertRows(yIndex,yearNode->subNodes->count(),yearNode->subNodes->count());
                new TagNode(month, yearNode, 1, LABEL_MONTH);
                endInsertRows();
            }
            else
            {
                (*mIter)->animeCount++;
                yearNode->animeCount++;
                QModelIndex index(createIndex(mIter-yearNode->subNodes->begin(),0,*mIter));
                emit dataChanged(index,index);
                emit dataChanged(yIndex, yIndex);
            }
        }
        else
        {
            beginInsertRows(createIndex(0,0,airDateNode),airDateNode->subNodes->count(),airDateNode->subNodes->count());
            TagNode *yearNode = new TagNode(year, airDateNode, 0, LABEL_YEAR);
            endInsertRows();
            beginInsertRows(createIndex(0,0,yearNode),0,0);
            new TagNode(month, yearNode, 1, LABEL_MONTH);
            endInsertRows();
            QModelIndex yIndex(createIndex(airDateNode->subNodes->indexOf(yearNode), 0, yearNode));
            emit dataChanged(yIndex, yIndex);
        }

        if(!oldTime.isEmpty())
        {
            QString oldYear(oldTime.left(4)), oldMonth(oldTime.right(2));
            auto iter = std::find_if(airDateNode->subNodes->begin(), airDateNode->subNodes->end(),[oldYear](TagNode *node){return node->tagTitle==oldYear;});
            if(iter!=airDateNode->subNodes->end())
            {
                TagNode *yearNode = *iter;
                QModelIndex yIndex(createIndex(iter - airDateNode->subNodes->begin(),0,yearNode));
                auto mIter = std::find_if(yearNode->subNodes->begin(), yearNode->subNodes->end(),[oldMonth](TagNode *node){return node->tagTitle==oldMonth;});
                if(mIter!=yearNode->subNodes->end())
                {
                    (*mIter)->animeCount--;
                    yearNode->animeCount--;
                    QModelIndex index(createIndex(mIter-yearNode->subNodes->begin(),0,*mIter));
                    if((*mIter)->animeCount==0)
                    {
                        beginRemoveRows(yIndex,index.row(),index.row());
                        yearNode->subNodes->removeAt(index.row());
                        endRemoveRows();
                        if(yearNode->animeCount==0)
                        {
                            beginRemoveRows(yIndex.parent(), yIndex.row(), yIndex.row());
                            airDateNode->subNodes->removeAt(yIndex.row());
                            endRemoveRows();
                        }
                    }
                    else
                    {
                        emit dataChanged(index, index);
                        emit dataChanged(yIndex, yIndex);
                    }
                }
            }
        }
    });
    QObject::connect(AnimeWorker::instance(),&AnimeWorker::removeTagFrom,this,[this](const QString &title, const QString &tag){
        if(!tagMap.contains(tag)) return;
        if(!tagMap[tag].contains(title)) return;
        tagMap[tag].remove(title);
        if(tagMap[tag].size()==0)tagMap.remove(tag);
        TagNode *tagCollectionNode(root->subNodes->at(1));
        for(int i=0;i<tagCollectionNode->subNodes->count(); ++i)
        {
            TagNode *tNode(tagCollectionNode->subNodes->at(i));
            if(tNode->tagTitle==tag)
            {
                tNode->animeCount--;
                QModelIndex index(createIndex(i,0,tNode));
                if(tNode->animeCount==0)
                {
                    beginRemoveRows(index.parent(),i,i);
                    tagCollectionNode->subNodes->removeAt(i);
                    endRemoveRows();
                }
                else
                {
                    emit dataChanged(index,index);
                }
                break;
            }
        }
    });
    QObject::connect(AnimeWorker::instance(),&AnimeWorker::removeTags,this,[this](const QString &title, const QString &time){
        QSet<QString> tagSet;
        for(auto iter=tagMap.begin();iter!=tagMap.end();)
        {
            if(iter.value().contains(title))
            {
                tagSet.insert(iter.key());
                iter.value().remove(title);
                if(iter.value().count()==0)
                    iter=tagMap.erase(iter);
                else
                    ++iter;
            }
            else
            {
                ++iter;
            }
        }
        TagNode *tagCollectionNode(root->subNodes->at(1));
        for(int i=0;i<tagCollectionNode->subNodes->count(); )
        {
            TagNode *tNode(tagCollectionNode->subNodes->at(i));
            if(tagSet.contains(tNode->tagTitle))
            {
                QModelIndex index(createIndex(i,0,tNode));
                tNode->animeCount--;
                if(tNode->animeCount==0)
                {
                    beginRemoveRows(index.parent(),i,i);
                    tagCollectionNode->subNodes->removeAt(i);
                    endRemoveRows();
                }
                else
                {
                    emit dataChanged(index,index);
                    ++i;
                }
            }
            else
            {
                ++i;
            }
        }

		if (time.isEmpty()) return;
        TagNode *airDateNode(root->subNodes->at(0));
        QString year(time.left(4)), month(time.right(2));
        auto iter = std::find_if(airDateNode->subNodes->begin(), airDateNode->subNodes->end(),[year](TagNode *node){return node->tagTitle==year;});
        if(iter!=airDateNode->subNodes->end())
        {
            TagNode *yearNode = *iter;
            QModelIndex yIndex(createIndex(iter - airDateNode->subNodes->begin(),0,yearNode));
            auto mIter = std::find_if(yearNode->subNodes->begin(), yearNode->subNodes->end(),[month](TagNode *node){return node->tagTitle==month;});
            if(mIter!=yearNode->subNodes->end())
            {
                (*mIter)->animeCount--;
                yearNode->animeCount--;
                QModelIndex index(createIndex(mIter-yearNode->subNodes->begin(),0,*mIter));
                if((*mIter)->animeCount==0)
                {
                    beginRemoveRows(yIndex,index.row(),index.row());
                    yearNode->subNodes->removeAt(index.row());
                    endRemoveRows();
                    if(yearNode->animeCount==0)
                    {
                        beginRemoveRows(yIndex.parent(), yIndex.row(), yIndex.row());
                        airDateNode->subNodes->removeAt(yIndex.row());
                        endRemoveRows();
                    }
                }
                else
                {
                    emit dataChanged(index, index);
                    emit dataChanged(yIndex, yIndex);
                }
            }
        }
    });
}

LabelModel::~LabelModel()
{
    if(root) delete root;
}

void LabelModel::refreshLabel()
{
    QMap<QString,int> timeMap;
    beginResetModel();
    tagMap.clear();
    if(root) delete root;
    root = new TagNode();
    AnimeWorker::instance()->loadTags(tagMap,timeMap);
    QMap<QString, TagNode *> yearNodes;
    TagNode *airDateNode=new TagNode(tr("Air Date"), root, 0, TYPELEVEL);
    airDateNode->subNodes=new QList<TagNode *>();
    for(auto iter=timeMap.begin();iter!=timeMap.end();++iter)
    {
        QString year(iter.key().left(4));
        TagNode *yearNode=yearNodes.value(year,nullptr);
        if(!yearNode)
        {
            yearNode = new TagNode(year, airDateNode, 0, LABEL_YEAR);
            yearNodes.insert(year, yearNode);
        }
        new TagNode(iter.key().right(2), yearNode, iter.value(), LABEL_MONTH);
    }
    TagNode *tagCollectionNode=new TagNode(tr("Tag"), root, 0, TYPELEVEL);
    tagCollectionNode->subNodes=new QList<TagNode *>();
    for(auto iter=tagMap.begin();iter!=tagMap.end();++iter)
    {
        new TagNode(iter.key(), tagCollectionNode, iter.value().count(), LABEL_TAG);
    }
    endResetModel();
}

void LabelModel::removeTag(const QModelIndex &index)
{
    if(!index.isValid()) return;
    TagNode *node = static_cast<TagNode*>(index.internalPointer());
    if(node->type!=LABEL_TAG) return;
    AnimeWorker::instance()->deleteTag(node->tagTitle);
    tagMap.remove(node->tagTitle);
    emit removedTag(node->tagTitle);
    beginRemoveRows(index.parent(),index.row(),index.row());
    node->parent->subNodes->removeOne(node);
    delete node;
    endRemoveRows();
}

void LabelModel::selLabelList(const QModelIndexList &indexes, QStringList &tags, QSet<QString> &times)
{
    for(const QModelIndex &index:indexes)
    {
        if(!index.isValid()) continue;
        TagNode *node = static_cast<TagNode*>(index.internalPointer());
        if(node->type==LABEL_YEAR)
        {
            for(TagNode *monthNode:*node->subNodes)
            {
                times<<node->tagTitle+'-'+monthNode->tagTitle;
            }
        }
        else if(node->type==LABEL_MONTH)
        {
            times<<node->parent->tagTitle+'-'+node->tagTitle;
        }
        else if(node->type==LABEL_TAG)
        {
            tags<<node->tagTitle;
        }
    }
}

void LabelModel::setBrushColor(LabelModel::BrushType type, const QColor &color)
{
    beginResetModel();
    foregroundColor[type]=color;
    endResetModel();
}

QModelIndex LabelModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    const TagNode *parentNode = parent.isValid() ? static_cast<TagNode*>(parent.internalPointer()) : root;
    TagNode *childNode = parentNode->subNodes->value(row);
    return childNode?createIndex(row, column, childNode):QModelIndex();
}

QModelIndex LabelModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();
    TagNode *childNode = static_cast<TagNode*>(child.internalPointer());
    TagNode *parentNode = childNode->parent;
    if (parentNode == root) return QModelIndex();
    int row=parentNode->parent->subNodes->indexOf(parentNode);
    return createIndex(row, 0, parentNode);
}

int LabelModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    const TagNode *parentNode = parent.isValid() ? static_cast<TagNode*>(parent.internalPointer()) : root;
    return parentNode->subNodes?parentNode->subNodes->size():0;
}

QVariant LabelModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    TagNode *node = static_cast<TagNode*>(index.internalPointer());
    switch (role)
    {
    case Qt::DisplayRole:
    {
        return node->tagTitle;
    }
    case CountRole:
    {
        return node->animeCount;
    }
    case TypeRole:
    {
        return node->type;
    }
    case Qt::ForegroundRole:
    {
        return node->type==TYPELEVEL?foregroundColor[BrushType::TopLevel]:foregroundColor[BrushType::ChildLevel];
    }
    }
    return QVariant();
}

bool LabelProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    TagNode *lNode = static_cast<TagNode*>(left.internalPointer()), *rNode = static_cast<TagNode*>(right.internalPointer());
    if(lNode->type==TYPELEVEL)
    {
        return lNode->parent->subNodes->indexOf(lNode)==0;
    }
    if(lNode->type==LABEL_YEAR || lNode->type==LABEL_MONTH)
    {
        return lNode->tagTitle>rNode->tagTitle;
    }
    return lNode->animeCount>rNode->animeCount;
}

bool LabelProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    TagNode *node = static_cast<TagNode*>(index.internalPointer());
    if(node->type==LABEL_TAG)
    {
        return node->tagTitle.contains(filterRegExp());
    }
    return true;
}

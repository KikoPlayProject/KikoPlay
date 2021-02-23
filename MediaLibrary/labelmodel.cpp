#include "labelmodel.h"
#include "animeworker.h"
#include "globalobjects.h"
#include "tagnode.h"
#include "animeinfo.h"
#include "Script/scriptmanager.h"
#include <QBrush>
#define CountRole Qt::UserRole+1
#define TypeRole Qt::UserRole+2
LabelModel::LabelModel(QObject *parent) : QAbstractItemModel(parent), root(nullptr)
{
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::renameEpTag, this, [=](const QString &oldAnimeName, const QString &newAnimeName){
        for(auto &animes : epPathAnimes)
        {
            if(animes.contains(oldAnimeName))
            {
                animes.remove(oldAnimeName);
                animes.insert(newAnimeName);
            }
        }
    });
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::addTimeTag, this, [=](const QString &tag){
        if(tag.isEmpty() || !root) return;
        insertNodeIndex(root->subNodes->value(C_TIME), tag.left(7), 1, TagNode::TAG_TIME, '-');
    });

    QObject::connect(AnimeWorker::instance(), &AnimeWorker::addScriptTag, this, [=](const QString &scriptId){
        if(scriptId.isEmpty() || !root) return;
        QString scriptName(scriptId);
        auto script = GlobalObjects::scriptManager->getScript(scriptId);
        if(script) scriptName = script->name();
        TagNode *node = insertNodeIndex(root->subNodes->value(C_SCRIPT), scriptName, 1, TagNode::TAG_SCRIPT);
        node->tagFilter = scriptId;
    });

    QObject::connect(AnimeWorker::instance(), &AnimeWorker::removeTimeTag, this, [=](const QString &tag){
        if(tag.isEmpty() || !root) return;
        removeNodeIndex(root->subNodes->value(C_TIME), tag.left(7),  false, '-');
    });

    QObject::connect(AnimeWorker::instance(), &AnimeWorker::removeScriptTag, this, [=](const QString &scriptId){
        if(scriptId.isEmpty() || !root) return;
        QString scriptName(scriptId);
        auto script = GlobalObjects::scriptManager->getScript(scriptId);
        if(script) scriptName = script->name();
        removeNodeIndex(root->subNodes->value(C_SCRIPT), scriptName, false, TagNode::TAG_SCRIPT);
    });
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::epAdded, this, [=](const QString &animeName, const EpInfo &ep){
        QString filePath = ep.localFile.mid(0, ep.localFile.lastIndexOf('/'));
        if(filePath.isEmpty() || !root) return;
        if(epPathAnimes.contains(filePath) && epPathAnimes[filePath].contains(animeName)) return;
        epPathAnimes[filePath].insert(animeName);
        insertNodeIndex(root->subNodes->value(C_FILE), filePath, 1, TagNode::TAG_FILE);
    });
}

LabelModel::~LabelModel()
{
    if(root) delete root;
}

void LabelModel::loadLabels()
{
    beginResetModel();
    if(root) delete root;
    root = new TagNode("root", nullptr, 0, TagNode::TAG_ROOT);
    addAnimeInfoTag();
    addEpPathTag();
    addCustomTag();
    endResetModel();
}

void LabelModel::selectedLabelList(const QModelIndexList &indexes,  SelectedLabelInfo &selectLabels)
{
    selectScript(selectLabels.scriptTags);
    selectTime(selectLabels.timeTags);
    selectFile(selectLabels.epPathTags);
    static QList<TagNode *> nodes;
    for(const QModelIndex &index:indexes)
    {
        if(!index.isValid()) continue;
        TagNode *node = static_cast<TagNode*>(index.internalPointer());
        if(node->tagType!=TagNode::TAG_CUSTOM) continue;
        nodes.clear();
        nodes.push_back(node);
        while(!nodes.empty())
        {
            TagNode *n = nodes.takeFirst();
            if(n->subNodes)
            {
                selectLabels.customPrefixTags.append(n->tagFilter);
            }
            else
            {
                selectLabels.customTags.append(n->tagFilter);
            }
        }
    }
}

void LabelModel::setBrushColor(LabelModel::BrushType type, const QColor &color)
{
    beginResetModel();
    foregroundColor[type]=color;
    endResetModel();
}

void LabelModel::addTag(const QString &animeName, const QString &tag, TagNode::TagType type)
{
    if(type == TagNode::TAG_ROOT_CATE || tag.isEmpty() || !root) return;
    TagNode * const cateNode[] = {
        root->subNodes->value(C_SCRIPT),
        root->subNodes->value(C_TIME),
        root->subNodes->value(C_FILE),
        root->subNodes->value(C_CUSTOM),
    };
    if(type == TagNode::TAG_FILE)
    {
        if(epPathAnimes.contains(tag) && epPathAnimes[tag].contains(animeName))  return;
        else epPathAnimes[tag].insert(animeName);
    }

    if(type == TagNode::TAG_CUSTOM)
    {
        if(tagAnimes.contains(tag) && tagAnimes[tag].contains(animeName)) return;
        else tagAnimes[tag].insert(animeName);
    }
    insertNodeIndex(cateNode[type], tag, 1, type, type==TagNode::TAG_TIME?'-':'/');
}

void LabelModel::addCustomTags(const QString &animeName, const QStringList &tagList)
{
    if(!root) return;
    for(const QString &tag : tagList)
    {
        if(tagAnimes.contains(tag) && tagAnimes[tag].contains(animeName)) continue;
        tagAnimes[tag].insert(animeName);
        insertNodeIndex(root->subNodes->value(C_CUSTOM), tag, 1, TagNode::TAG_CUSTOM);
    }
    AnimeWorker::instance()->saveTags(animeName, tagList);
}

void LabelModel::removeTag(const QString &animeName, const QString &tag, TagNode::TagType type)
{
    if(type == TagNode::TAG_ROOT_CATE || !root) return;
    TagNode * const cateNode[] = {
        root->subNodes->value(C_SCRIPT),
        root->subNodes->value(C_TIME),
        root->subNodes->value(C_FILE),
        root->subNodes->value(C_CUSTOM),
    };
    if(type == TagNode::TAG_FILE)
    {
        if(epPathAnimes.contains(tag) && epPathAnimes[tag].contains(animeName))
        {
            epPathAnimes[tag].remove(animeName);
        }
        else return;
    }

    if(type == TagNode::TAG_CUSTOM)
    {
        if(tagAnimes.contains(tag) && tagAnimes[tag].contains(animeName))
        {
            tagAnimes[tag].remove(animeName);
            AnimeWorker::instance()->deleteTag(tag, animeName);
        }
        else return;
    }
    removeNodeIndex(cateNode[type], tag, false, type==TagNode::TAG_TIME?'-':'/');
}

void LabelModel::removeTag(const QString &animeName, const QString &airDate, const QString &scriptId)
{
    if(!airDate.isEmpty())
    {
        removeTag(animeName, airDate, TagNode::TAG_TIME);
    }
    if(!scriptId.isEmpty())
    {
        QString scriptName(scriptId);
        auto script = GlobalObjects::scriptManager->getScript(scriptId);
        if(script) scriptName = script->name();
        removeTag(animeName, scriptName, TagNode::TAG_SCRIPT);
    }
    for(auto iter=epPathAnimes.begin(); iter!=epPathAnimes.end();++iter)
    {
        if(iter.value().contains(animeName))
        {
            removeTag(animeName, iter.key(), TagNode::TAG_FILE);
        }
    }
    for(auto iter=tagAnimes.begin(); iter!=tagAnimes.end();++iter)
    {
        if(iter.value().contains(animeName))
        {
            removeTag(animeName, iter.key(), TagNode::TAG_CUSTOM);
        }
    }
    AnimeWorker::instance()->deleteTag("", animeName);
}

void LabelModel::removeTag(const QModelIndex &index)
{
    if(!index.isValid()) return;
    TagNode *node = static_cast<TagNode*>(index.internalPointer());
    if(node->tagType != TagNode::TAG_CUSTOM) return;
    QStringList nodePaths;
    QList<TagNode *> nodes({node});
    while(!nodes.empty())
    {
        TagNode *n = nodes.takeFirst();
        if(n->subNodes)
        {
            nodes.append(*(n->subNodes));
        }
        else
        {
            nodePaths.append(n->tagFilter);
        }
    }
    for(const QString &tag : nodePaths)
    {
        tagAnimes.remove(tag);
        emit tagRemoved(tag);
    }
    TagNode *parent = node->parent;
    QModelIndex parentIndex = index.parent();
    int decCount = node->animeCount;

    AnimeWorker::instance()->deleteTags(nodePaths);
    beginRemoveRows(parentIndex,index.row(),index.row());
    parent->removeSubNode(index.row());
    endRemoveRows();

    while(parent && parent->tagType != TagNode::TAG_ROOT_CATE)
    {
        parent->animeCount -= decCount;
        if(parent->animeCount<=0)
        {
            TagNode *nParent = parent->parent;
            QModelIndex nIndex = parentIndex.parent();
            beginRemoveRows(nIndex, parentIndex.row(), parentIndex.row());
            nParent->removeSubNode(parentIndex.row());
            endRemoveRows();
            parent = nParent;
            parentIndex = nIndex;
        }
        else
        {
            emit dataChanged(parentIndex, parentIndex);
            parent = parent->parent;
            parentIndex = parentIndex.parent();
        }
    }
}

QModelIndex LabelModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    const TagNode *parentNode = parent.isValid() ? static_cast<TagNode*>(parent.internalPointer()) : root;
    if(!root) return QModelIndex();
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
    if(!root) return 0;
    return parentNode->subNodes?parentNode->subNodes->size():0;
}

QVariant LabelModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    TagNode *node = static_cast<TagNode*>(index.internalPointer());
    switch (role)
    {
    case Qt::DisplayRole:
        return node->tagTitle;
    case Qt::ToolTipRole:
        return node->tagFilter;
    case CountRole:
        return node->animeCount;
    case TypeRole:
        return node->tagType;
    case Qt::ForegroundRole:
        return node->tagType==TagNode::TAG_ROOT_CATE?foregroundColor[BrushType::TopLevel]:foregroundColor[BrushType::ChildLevel];
    case Qt::CheckStateRole:
        if (index.column()==0)
        {
            if((node->tagType == TagNode::TAG_SCRIPT  ||
                node->tagType == TagNode::TAG_TIME ||
                node->tagType == TagNode::TAG_FILE))
            return node->checkStatus;
        }
    }
    return QVariant();
}

bool LabelModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) return false;
    TagNode *node = static_cast<TagNode*>(index.internalPointer());
    if(role==Qt::CheckStateRole && index.column()==0)
    {
        setCheckStatus(node, value.toInt()==Qt::Checked);
        emit tagCheckedChanged();
        return true;
    }
    return false;
}

Qt::ItemFlags LabelModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if(!index.isValid()) return defaultFlags;
    TagNode *node = static_cast<TagNode*>(index.internalPointer());
    if (index.column()==0)
    {
        if((node->tagType == TagNode::TAG_SCRIPT  ||
            node->tagType == TagNode::TAG_TIME ||
            node->tagType == TagNode::TAG_FILE))
        return  Qt::ItemIsUserCheckable | defaultFlags;
    }
    return defaultFlags;
}

void LabelModel::addAnimeInfoTag()
{
    AnimeInfoTag animeInfoTag;
    AnimeWorker::instance()->loadAnimeInfoTag(animeInfoTag);

    auto &scriptIdCount = animeInfoTag.scriptIdCount;
    TagNode *scriptCate = new TagNode(tr("Source"), root, 0, TagNode::TAG_ROOT_CATE);
    cateTags[CategoryTag::C_SCRIPT] = scriptCate;
    for(auto iter = scriptIdCount.begin(); iter!=scriptIdCount.end(); ++iter)
    {
        QString scriptId(iter.key());
        auto script = GlobalObjects::scriptManager->getScript(scriptId);
        TagNode *scriptNode = new TagNode(script?script->name():scriptId, scriptCate, iter.value(), TagNode::TAG_SCRIPT);
        scriptNode->tagFilter  = scriptId;
    }

    auto &airDateCount = animeInfoTag.airDateCount;
    TagNode *airDateNode=new TagNode(tr("Air Date"), root, 0, TagNode::TAG_ROOT_CATE);
    cateTags[CategoryTag::C_TIME] = airDateNode;
    QHash<QString, TagNode *> yearNodes;

    for(auto iter=airDateCount.begin(); iter!=airDateCount.end(); ++iter)
    {
        QString year(iter.key().left(4));
        TagNode *yearNode=yearNodes.value(year,nullptr);
        if(!yearNode)
        {
            yearNode = new TagNode(year, airDateNode, 0, TagNode::TAG_TIME);
            yearNodes.insert(year, yearNode);
        }
        TagNode *monthNode = new TagNode(iter.key().right(2), yearNode, iter.value(), TagNode::TAG_TIME);
        monthNode->tagFilter = iter.key();
    }
}

void LabelModel::addEpPathTag()
{
    AnimeWorker::instance()->loadEpInfoTag(epPathAnimes);
    TagNode *epPathNode=new TagNode(tr("File"), root, 0, TagNode::TAG_ROOT_CATE);
    cateTags[CategoryTag::C_FILE] = epPathNode;
    for(auto iter=epPathAnimes.begin(); iter!=epPathAnimes.end();++iter)
    {
        insertNode(epPathNode, iter.key(), iter.value().size(), TagNode::TAG_FILE);
    }

}

void LabelModel::addCustomTag()
{
    AnimeWorker::instance()->loadCustomTags(tagAnimes);
    TagNode *customNode=new TagNode(tr("Tag"), root, 0, TagNode::TAG_ROOT_CATE);
    cateTags[CategoryTag::C_CUSTOM] = customNode;
    for(auto iter=tagAnimes.begin(); iter!=tagAnimes.end();++iter)
    {
        insertNode(customNode, iter.key(), iter.value().size(), TagNode::TAG_CUSTOM);
    }
}

TagNode *LabelModel::insertNode(TagNode *parent, const QString &strPath, int count, TagNode::TagType type, char split)
{
    auto titles(strPath.splitRef(split, QString::SkipEmptyParts));
    if(titles.size()==0) return parent;
    TagNode *current = parent;
    QString curPath;
    bool titleStart = strPath.startsWith(split);
    for(auto &title : titles)
    {
        if(!curPath.isEmpty() | titleStart) curPath.append(split);
        curPath.append(title);
        if(!current->subNodes) current->subNodes = new QList<TagNode *>;
        auto iter = std::find_if(current->subNodes->begin(), current->subNodes->end(), [=](TagNode *node){return  node->tagTitle == title;});
        TagNode *next = nullptr;
        if(iter == current->subNodes->end())
        {
            next = new TagNode(title.toString(), current, 0, type);
            next->tagFilter = curPath;
        }
        else
        {
            next = *iter;
        }
        current->animeCount += count;
        current = next;
    }
    current->animeCount += count;
    return current;

}

TagNode *LabelModel::insertNodeIndex(TagNode *parent, const QString &strPath, int count, TagNode::TagType type, char split)
{
    auto titles(strPath.splitRef(split, QString::SkipEmptyParts));
    if(titles.size()==0) return parent;
    TagNode *current = parent;
    QModelIndex currentIndex;
    if(current->parent) currentIndex = createIndex(current->parent->subNodes->indexOf(current), 0, current);
    current->animeCount += count;
    emit dataChanged(currentIndex, currentIndex);
    QString curPath;
    bool titleStart = strPath.startsWith(split);
    for(auto &title : titles)
    {
        if(!curPath.isEmpty() | titleStart) curPath.append(split);
        curPath.append(title);
        if(!current->subNodes) current->subNodes = new QList<TagNode *>;
        auto iter = std::find_if(current->subNodes->begin(), current->subNodes->end(), [=](TagNode *node){return  node->tagTitle == title;});
        TagNode *next = nullptr;
        if(iter == current->subNodes->end())
        {
            beginInsertRows(currentIndex, current->subNodes->count(), current->subNodes->count());
            next = new TagNode(title.toString(), current, 0, type);
            next->animeCount += count;
            next->tagFilter = curPath;
            endInsertRows();
            currentIndex = index(current->subNodes->count()-1,  0, currentIndex);
        }
        else
        {
            next = *iter;
            next->animeCount += count;
            currentIndex = index(iter - current->subNodes->begin(),  0, currentIndex);
            emit dataChanged(currentIndex, currentIndex);
        }
        current = next;
    }
    return current;
}

void LabelModel::removeNodeIndex(TagNode *parent, const QString &strPath, bool removeAll, char split)
{
    auto titles(strPath.splitRef(split, QString::SkipEmptyParts));
    if(titles.size()==0) return;
    TagNode *current = parent;
    QModelIndex currentIndex;
    if(current->parent) currentIndex = createIndex(current->parent->subNodes->indexOf(current), 0, current);
    QModelIndexList indexList({currentIndex});

    for(auto &title : titles)
    {
        if(!current->subNodes) return;
        auto iter = std::find_if(current->subNodes->begin(), current->subNodes->end(), [=](TagNode *node){return  node->tagTitle == title;});
        if(iter == current->subNodes->end()) return;
        TagNode *next = *iter;
        currentIndex = index(iter - current->subNodes->begin(),  0, currentIndex);
        indexList.append(currentIndex);
        current = next;
    }

    while(!indexList.isEmpty())
    {
        currentIndex = indexList.takeLast();
        --current->animeCount;
        if(current->animeCount<=0 || removeAll)
        {
            if(current->tagType == TagNode::TAG_ROOT_CATE) break;
            TagNode *parent = current->parent;
            beginRemoveRows(currentIndex.parent(), currentIndex.row(), currentIndex.row());
            current->parent->removeSubNode(currentIndex.row());
            endRemoveRows();
            current = parent;
        }
        else
        {
            emit dataChanged(currentIndex, currentIndex);
            current = current->parent;
        }
    }
}

void LabelModel::setCheckStatus(TagNode *node, bool checked)
{
    if(node->tagType == TagNode::TAG_ROOT_CATE || node == root) return;

    // down
    QList<TagNode *> subNodes({node});
    while(!subNodes.isEmpty())
    {
        TagNode *cn = subNodes.takeFirst();
        cn->checkStatus = checked?Qt::Checked:Qt::Unchecked;
        QModelIndex cIndex(createIndex(cn->parent->subNodes->indexOf(cn), 0, cn));
        emit dataChanged(cIndex, cIndex, {Qt::CheckStateRole});
        if(cn->subNodes)
        {
            subNodes.append(*(cn->subNodes));
        }
    }
    // up
    TagNode *parent = node->parent;
    QModelIndex parentIndex = createIndex(parent->parent->subNodes->indexOf(parent), 0, parent);
    while(parent && parent->tagType != TagNode::TAG_ROOT_CATE)
    {
        int checkStatusStatis[3]={0,0,0};
        for(TagNode *cn :*parent->subNodes)
        {
            checkStatusStatis[cn->checkStatus]++;
        }
        if(checkStatusStatis[2]==parent->subNodes->size())
            parent->checkStatus=Qt::Checked;
        else if(checkStatusStatis[0]==parent->subNodes->size())
            parent->checkStatus=Qt::Unchecked;
        else
            parent->checkStatus=Qt::PartiallyChecked;
        emit dataChanged(parentIndex, parentIndex, {Qt::CheckStateRole});
        parentIndex = parentIndex.parent();
        parent = parent->parent;
    }
}

void LabelModel::selectScript(QSet<QString> &scriptTags)
{
    TagNode *scriptCate = root->subNodes->value(C_SCRIPT);
    for(TagNode *c : *scriptCate->subNodes)
    {
        if(c->checkStatus==Qt::Checked)
            scriptTags.insert(c->tagFilter);
    }
}

void LabelModel::selectTime(QSet<QString> &timeTags)
{
    TagNode *timeCate = root->subNodes->value(C_TIME);
    for(TagNode *c : *timeCate->subNodes)
    {
        if(c->checkStatus!=Qt::Unchecked)
        {
            for(TagNode *monthNode:*c->subNodes)
				if(monthNode->checkStatus==Qt::Checked)
					timeTags.insert(monthNode->tagFilter);
        }
    }
}

void LabelModel::selectFile(QStringList &paths)
{
    TagNode *timeCate = root->subNodes->value(C_FILE);
    static QList<TagNode *> nodes;
    nodes.clear();
    for(TagNode *c : *timeCate->subNodes)
    {
        if(c->checkStatus!=Qt::Unchecked)
        {
            nodes.append(c);
        }
    }
    while(!nodes.empty())
    {
        TagNode *n = nodes.takeFirst();
        if(n->checkStatus == Qt::Checked)
        {
            paths.append(n->tagFilter);
        }
        else if(n->checkStatus == Qt::PartiallyChecked)
        {
            if(n->subNodes)
            {
                for(TagNode *s:*n->subNodes)
                {
                    nodes.append(s);
                }
            }
        }
    }
}

bool LabelProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    TagNode *lNode = static_cast<TagNode*>(left.internalPointer()), *rNode = static_cast<TagNode*>(right.internalPointer());
    switch (lNode->tagType)
    {
    case TagNode::TAG_ROOT_CATE:
        return lNode->parent->subNodes->indexOf(lNode)<rNode->parent->subNodes->indexOf(rNode);
    case TagNode::TAG_TIME:
        return lNode->tagTitle>rNode->tagTitle;
    case TagNode::TAG_FILE:
        return lNode->tagTitle<rNode->tagTitle;
    default:
        return lNode->animeCount>rNode->animeCount;
    }
}

bool LabelProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    TagNode *node = static_cast<TagNode*>(index.internalPointer());
    if(node->tagType==TagNode::TAG_CUSTOM)
    {
        return node->tagTitle.contains(filterRegExp());
    }
    return true;
}

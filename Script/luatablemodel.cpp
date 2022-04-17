#include "luatablemodel.h"

void LuaTableModel::setRoot(LuaItem *root)
{
    beginResetModel();
    mRootItem.reset(root);
    endResetModel();
}

QVariant LuaTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    LuaItem *item = static_cast<LuaItem*>(index.internalPointer());
    Columns col = static_cast<Columns>(index.column());

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col == Columns::KEY)
        {
            return QString("[%1]%2").arg(luaTypes[item->keyType], item->key);
        }
        else if(col == Columns::VALUE)
        {
            return QString("[%1]%2").arg(luaTypes[item->valType], item->value);
        }
        break;
    }
    case Qt::ToolTipRole:
    {
        if(col == Columns::KEY)
        {
            return item->key;
        }
        else if(col == Columns::VALUE)
        {
            return item->value;
        }
        break;
    }
    }
    return QVariant();
}

QVariant LuaTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section < headers.size())return headers.at(section);
    }
    return QVariant();
}

QModelIndex LuaTableModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();

    LuaItem *parentItem = nullptr;
    parentItem = parent.isValid() ? static_cast<LuaItem *>(parent.internalPointer()) : mRootItem.data();

    LuaItem *childItem = parentItem->mChilds.at(row);
    return childItem? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex LuaTableModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) return QModelIndex();

    LuaItem *childItem = static_cast<LuaItem*>(index.internalPointer());
    LuaItem *parentItem = childItem->mParent;

    if (parentItem == mRootItem) return QModelIndex();

    int parentRow = parentItem->mParent->mChilds.indexOf(parentItem);
    return createIndex(parentRow, 0, parentItem);
}

int LuaTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    LuaItem *parentItem = nullptr;
    parentItem = parent.isValid()? static_cast<LuaItem*>(parent.internalPointer()) : mRootItem.data();
    return parentItem->mChilds.size();
}

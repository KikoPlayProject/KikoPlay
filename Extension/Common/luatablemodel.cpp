#include "luatablemodel.h"
#include "Extension/Lua/lua.hpp"

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

void buildLuaItemTree(lua_State *L, LuaItem *parent, QSet<quintptr> &tableHash)
{
    if(!L) return;
    if(lua_gettop(L)==0) return;
    if(lua_type(L, -1) != LUA_TTABLE) return;
    lua_pushnil(L); // t nil
    auto getLuaValue = [](lua_State *L, int pos) -> QString {
        switch (lua_type(L, pos))
        {
        case LUA_TNIL:
            return "nil";
        case LUA_TNUMBER:
        {
            double d = lua_tonumber(L, pos);
            return QString::number(d);
        }
        case LUA_TBOOLEAN:
        {
            return bool(lua_toboolean(L, pos))? "true" : "false";
        }
        case LUA_TSTRING:
        {
            size_t len = 0;
            const char *s = (const char *)lua_tolstring(L, pos, &len);
            return QByteArray(s, len);
        }
        default:
            return "";
        }
    };
    while (lua_next(L, -2)) // t key value
    {
        LuaItem *item = new LuaItem(parent);
        item->keyType = static_cast<LuaItem::LuaType>(lua_type(L, -2));
        item->key = getLuaValue(L, -2);
        item->valType = static_cast<LuaItem::LuaType>(lua_type(L, -1));
        if(lua_type(L, -1) == LUA_TTABLE)
        {
            quintptr tablePtr = reinterpret_cast<quintptr>(lua_topointer(L, -1));
            if(!tableHash.contains(tablePtr))
            {
                tableHash.insert(tablePtr);
                buildLuaItemTree(L, item, tableHash);
            }
        }
        else
        {
            item->value = getLuaValue(L, -1);
        }
        lua_pop(L, 1); // key
    }
}

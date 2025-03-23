#include "ext_common.h"
#include "Extension/App/AppWidgets/appwidget.h"
#include "Extension/Modules/lua_appimage.h"
#include <QImage>

namespace Extension
{

void pushValue(lua_State *L, const QVariant &val)
{
    if(!L) return;
    switch (val.type())
    {
    case QVariant::Int:
    case QVariant::LongLong:
    case QVariant::UInt:
    case QVariant::ULongLong:
        lua_pushinteger(L, val.toLongLong());
        break;
    case QVariant::Double:
        lua_pushnumber(L, val.toDouble());
        break;
    case QVariant::ByteArray:
    {
        auto bytes = val.toByteArray();
        lua_pushlstring(L, bytes.constData(), bytes.size());
        break;
    }
    case QVariant::String:
        lua_pushstring(L, val.toString().toStdString().c_str());
        break;
    case QVariant::Bool:
        lua_pushboolean(L, val.toBool());
        break;
    case QVariant::Invalid:
        lua_pushnil(L);
        break;
    case QVariant::List:
    case QVariant::StringList:
    {
        lua_newtable(L); // table
        const auto &l = val.toList();
        for(int i=0; i<l.size(); ++i)
        {
            pushValue(L, l.value(i));
            lua_rawseti(L, -2, i+1);
        }
        break;
    }
    case QVariant::Map:
    {
        lua_newtable(L); // table
        const auto &m = val.toMap();
        for(auto i = m.constBegin(); i!=m.constEnd(); ++i)
        {
            lua_pushstring(L, i.key().toStdString().c_str()); // table key
            pushValue(L, i.value()); // table key value
            lua_rawset(L, -3);
        }
        break;
    }
    default:
        if (val.userType() == QMetaType::type("Extension::AppWidget*"))
        {
            val.value<Extension::AppWidget*>()->luaPush(L);
            break;
        }
        else if (val.userType() == QMetaType::type("Extension::LuaItemRef"))
        {
            Extension::LuaItemRef refItem = val.value<Extension::LuaItemRef>();
            if (refItem.ref > 0)
            {
                if (refItem.tableRef < 0)
                {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, refItem.ref);
                }
                else
                {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, refItem.tableRef);  // tb
                    lua_rawgeti(L, -1, refItem.ref);  // tb ref
                    lua_remove(L, -2);  // ref
                }
                break;
            }
        }
        else if (val.type() == QMetaType::QImage)
        {
            QImage* img = new QImage(val.value<QImage>());
            Extension::AppImage::pushImg(L, img);
            break;
        }
        lua_pushnil(L);
        break;
    }
}

QVariant getValue(lua_State *L, bool useString, int parseLevel, int curLevel)
{
    if(!L) return QVariant();
    if (parseLevel > -1 && curLevel > parseLevel) return QVariant();
    if(lua_gettop(L)==0) return QVariant();
    switch (lua_type(L, -1))
    {
    case LUA_TNUMBER:
    {
        double d = lua_tonumber(L, -1);
        return d;
    }
    case LUA_TBOOLEAN:
    {
        return bool(lua_toboolean(L, -1));
    }
    case LUA_TSTRING:
    {
        size_t len = 0;
        const char *s = (const char *)lua_tolstring(L, -1, &len);
        if (useString) return QString(s);
        else return QByteArray(s, len);
    }
    case LUA_TTABLE:
    {
        // If all keys are integers, and they're in sequence, take it as an list.
        int count = 0;
        for (int i = 1; ; ++i)
        {
            lua_pushinteger(L, i); // n
            lua_gettable(L, -2); // t[n]
            bool empty = lua_isnil(L, -1); // t[n]
            lua_pop(L, 1); // -
            if (empty)
            {
                count = i - 1;
                break;
            }
        }
        size_t length = getTableLength(L, -1);
        if(count < 0 || count != length) // map
        {
            QVariantMap map;
            lua_pushnil(L); // t nil
            while (lua_next(L, -2)) { // t key value
                if (lua_type(L, -2) != LUA_TSTRING)
                {
                    luaL_error(L, "key must be a string, but got %s",
                               lua_typename(L, lua_type(L, -2)));
                }
                QString key(lua_tostring(L, -2));
                map[key] = getValue(L, useString, parseLevel, curLevel + 1);
                lua_pop(L, 1); // key
            }
            return map;
        }
        else  // array
        {
            QVariantList list;
            for (int i = 0; ;++i) {
                lua_pushinteger(L, i + 1); //t n1
                lua_gettable(L, -2); //t t[n1]
                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    break;
                }
                list.append(getValue(L, useString, parseLevel, curLevel + 1));
                lua_pop(L, 1); // -
            }
            return list;
        }
    }
    default:
        return QVariant();
    }
}

int getTableLength(lua_State *L, int pos)
{
    if(!L) return 0;
    if (pos < 0)  pos = lua_gettop(L) + (pos + 1);
    lua_pushnil(L); // nil
    int length = 0;
    while (lua_next(L, pos))  // key value
    {
        ++length;
        lua_pop(L, 1); // key
    }
    return length;
}

QVariant getValue(lua_State *L, bool useString, const MapModifier &modifier, int parseLevel, int curLevel)
{
    if(!L) return QVariant();
    if (parseLevel > -1 && curLevel > parseLevel) return QVariant();
    if(lua_gettop(L)==0) return QVariant();
    switch (lua_type(L, -1))
    {
    case LUA_TNUMBER:
    {
        double d = lua_tonumber(L, -1);
        return d;
    }
    case LUA_TBOOLEAN:
    {
        return bool(lua_toboolean(L, -1));
    }
    case LUA_TSTRING:
    {
        size_t len = 0;
        const char *s = (const char *)lua_tolstring(L, -1, &len);
        if (useString) return QString(s);
        else return QByteArray(s, len);
    }
    case LUA_TTABLE:
    {
        // If all keys are integers, and they're in sequence, take it as an list.
        int count = 0;
        for (int i = 1; ; ++i)
        {
            lua_pushinteger(L, i); // n
            lua_gettable(L, -2); // t[n]
            bool empty = lua_isnil(L, -1); // t[n]
            lua_pop(L, 1); // -
            if (empty)
            {
                count = i - 1;
                break;
            }
        }
        size_t length = getTableLength(L, -1);
        if(count < 0 || count != length) // map
        {
            QVariantMap map;
            lua_pushnil(L); // t nil
            while (lua_next(L, -2)) { // t key value
                if (lua_type(L, -2) != LUA_TSTRING)
                {
                    luaL_error(L, "key must be a string, but got %s",
                               lua_typename(L, lua_type(L, -2)));
                }
                QString key(lua_tostring(L, -2));
                map[key] = getValue(L, useString, modifier, parseLevel, curLevel + 1);
                lua_pop(L, 1); // key
            }
            modifier(L, map, curLevel);
            return map;
        }
        else  // array
        {
            QVariantList list;
            for (int i = 0; ;++i) {
                lua_pushinteger(L, i + 1); //t n1
                lua_gettable(L, -2); //t t[n1]
                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    break;
                }
                list.append(getValue(L, useString, modifier, parseLevel, curLevel + 1));
                lua_pop(L, 1); // -
            }
            return list;
        }
    }
    default:
        return QVariant();
    }
}

}

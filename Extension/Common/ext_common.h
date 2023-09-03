#ifndef EXT_COMMON_H
#define EXT_COMMON_H


#include "Extension/Lua/lua.hpp"
#include <QVariant>

namespace Extension
{

struct AppRes
{
    AppRes() = default;
    AppRes &operator=(const AppRes&) = delete;
    AppRes(const AppRes&) = delete;
    virtual ~AppRes() {}
};
struct LuaItemRef
{
    int ref = -1;
    int tableRef = -1;
};


typedef std::function<void (lua_State *, QVariantMap &, int)> MapModifier;

int getTableLength(lua_State *L, int pos);
void pushValue(lua_State *L, const QVariant &val);
QVariant getValue(lua_State *L, bool useString, int parseLevel = -1, int curLevel = 0);
QVariant getValue(lua_State *L, bool useString, const MapModifier &modifier, int parseLevel = -1, int curLevel = 0);

}
Q_DECLARE_METATYPE(Extension::LuaItemRef)

#endif // EXT_COMMON_H

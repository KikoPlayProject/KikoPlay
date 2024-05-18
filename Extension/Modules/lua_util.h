#ifndef LUA_UTIL_H
#define LUA_UTIL_H
#include "modulebase.h"

namespace Extension
{
class LuaUtil : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
public:
    static int compress(lua_State *L);
    static int decompress(lua_State *L);
    static int writeSetting(lua_State *L);
    static int execute(lua_State *L);
    static int hashData(lua_State *L);
    static int base64(lua_State *L);
    static int logger(lua_State *L);
    static int message(lua_State *L);
    static int dialog(lua_State *L);
    static int simplifiedTraditionalTrans(lua_State *L);
    static int envInfo(lua_State *L);
    static int viewTable(lua_State *L);
    static int allScripts(lua_State *L);
};
}
#endif // LUA_UTIL_H

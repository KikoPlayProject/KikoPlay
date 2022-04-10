#include "modulebase.h"
#include "Common/logger.h"
namespace LuaModule
{

ModuleBase::ModuleBase(lua_State *state) : L(state)
{

}

void ModuleBase::registerFuncs(const char *tname, const luaL_Reg *funcs)
{
    if(!L) return;
    lua_getglobal(L, tname);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);
    }
    luaL_setfuncs(L, funcs, 0);
    lua_setglobal(L, tname);
}

void ModuleBase::registerMemberFuncs(const char *metaName, const luaL_Reg *funcs)
{
    if(!L) return;
    luaL_newmetatable(L, metaName);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); // pushes the metatable
    lua_rawset(L, -3); // metatable.__index = metatable
    luaL_setfuncs(L, funcs, 0);
    lua_pop(L, 1);
}

void ModuleBase::logInfo(const QString &info, const QString &scriptId)
{
    Logger::logger()->log(Logger::Script, QString("[%1]%2").arg(scriptId, info));
}

void ModuleBase::logError(const QString &info, const QString &scriptId)
{
    Logger::logger()->log(Logger::Script, QString("[ERROR][%1]%2").arg(scriptId, info));
}

}

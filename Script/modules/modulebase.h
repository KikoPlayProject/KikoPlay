#ifndef MODULEBASE_H
#define MODULEBASE_H
#include "../lua/lua.hpp"
#include <QString>
class ScriptBase;
namespace LuaModule
{
class ModuleBase
{
public:
    ModuleBase(lua_State *state);
    virtual void setup() = 0;

protected:
    lua_State *L;
    void registerFuncs(const char *tname, const luaL_Reg *funcs);
    void registerMemberFuncs(const char *metaName, const luaL_Reg *funcs);
    static void logInfo(const QString &info, const QString &scriptId);
    static void logError(const QString &info, const QString &scriptId);
};
}
#endif // MODULEBASE_H

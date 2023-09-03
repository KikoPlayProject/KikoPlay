#ifndef LUA_APPUTIL_H
#define LUA_APPUTIL_H
#include "modulebase.h"

namespace  Extension
{
class AppUtil : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int logger(lua_State *L);
    static int json2table(lua_State *L);
    static int table2json(lua_State *L);
    static int flash(lua_State *L);
    static int viewTable(lua_State *L);
};
}
#endif // LUA_APPUTIL_H

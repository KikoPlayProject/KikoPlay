#ifndef LUA_STRINGUTIL_H
#define LUA_STRINGUTIL_H

#include "modulebase.h"
namespace Extension
{
class StringUtil : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    enum CodeType
    {
        LOCAL, UTF_8
    };
    static int trim(lua_State *L);
    static int startswith(lua_State *L);
    static int endswith(lua_State *L);
    static int split(lua_State *L);
    static int indexof(lua_State *L);
    static int lastindexof(lua_State *L);
    static int encode(lua_State *L);
};
}
#endif // LUA_STRINGUTIL_H

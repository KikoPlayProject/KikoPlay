#ifndef LUA_PLAYERINTERFACE_H
#define LUA_PLAYERINTERFACE_H

#include "modulebase.h"
namespace Extension
{

class PlayerInterface : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int setmedia(lua_State *L);
    static int curfile(lua_State *L);
    static int property(lua_State *L);
    static int command(lua_State *L);
};

}
#endif // LUA_PLAYERINTERFACE_H

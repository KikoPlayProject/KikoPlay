#ifndef LUA_CLIPBOARDINTERFACE_H
#define LUA_CLIPBOARDINTERFACE_H

#include "modulebase.h"
namespace Extension
{

class ClipboardInterface : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int gettext(lua_State *L);
    static int settext(lua_State *L);
    static int getimg(lua_State *L);
    static int setimg(lua_State *L);
};

}
#endif // LUA_CLIPBOARDINTERFACE_H

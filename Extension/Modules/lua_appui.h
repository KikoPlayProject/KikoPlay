#ifndef APPGUI_H
#define APPGUI_H

#include "modulebase.h"

namespace  Extension
{

class AppUI : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int get(lua_State *L);
    static int remove(lua_State *L);
};

}
#endif // APPGUI_H

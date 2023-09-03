#ifndef APPEVENT_H
#define APPEVENT_H

#include "modulebase.h"

namespace  Extension
{

class AppEventBusEvent : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();

private:
    static int push(lua_State *L);
    static int listen(lua_State *L);
    static int unlisten(lua_State *L);
};

}
#endif // APPEVENT_H

#ifndef DIR_H
#define DIR_H

#include "modulebase.h"

namespace  Extension
{
class Dir : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int fileinfo(lua_State *L);
    static int exists(lua_State *L);
    static int mkpath(lua_State *L);
    static int rmpath(lua_State *L);
    static int entrylist(lua_State *L);
    static int rename(lua_State *L);
    static int syspath(lua_State *L);
};

}
#endif // DIR_H

#ifndef LIBRARYINTERFACE_H
#define LIBRARYINTERFACE_H

#include "modulebase.h"
namespace Extension
{

class LibraryInterface : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int getanime(lua_State *L);
    static int gettag(lua_State *L);
    static int addanime(lua_State *L);
    static int addtag(lua_State *L);
};

}
#endif // LIBRARYINTERFACE_H

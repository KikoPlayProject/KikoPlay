#ifndef DOWNLOADINTERFACE_H
#define DOWNLOADINTERFACE_H

#include "modulebase.h"
namespace  Extension
{

class DownloadInterface : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int addurl(lua_State *L);
    static int addtorrent(lua_State *L);
};

}
#endif // DOWNLOADINTERFACE_H

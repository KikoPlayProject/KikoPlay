#ifndef STORAGEINTERFACE_H
#define STORAGEINTERFACE_H

#include "modulebase.h"
namespace Extension
{
class AppStorage;
class StorageInterface : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int set(lua_State *L);
    static int get(lua_State *L);

    static AppStorage *getStorage(lua_State *L);
    static const char *resKey;
};

}
#endif // STORAGEINTERFACE_H

#ifndef APPIMAGE_H
#define APPIMAGE_H

#include "modulebase.h"
class QImage;

namespace  Extension
{

class AppImage : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
public:
    static const char *MetaName;
    static QImage *checkImg(lua_State *L, int pos);
    static void pushImg(lua_State *L, QImage *img);
private:
    static int createimg(lua_State *L);

    static int imgGC(lua_State *L);
    static int size(lua_State *L);
    static int save(lua_State *L);
    static int tobytes(lua_State *L);
    static int scale(lua_State *L);
};

}
#endif // APPIMAGE_H

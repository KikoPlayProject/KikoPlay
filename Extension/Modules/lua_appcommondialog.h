#ifndef APPCOMMONDIALOG_H
#define APPCOMMONDIALOG_H

#include "modulebase.h"
namespace  Extension
{

class AppCommonDialog : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();

private:
    static int openfile(lua_State *L);
    static int savefile(lua_State *L);
    static int selectdir(lua_State *L);
    static int dialog(lua_State *L);
};

}
#endif // APPCOMMONDIALOG_H

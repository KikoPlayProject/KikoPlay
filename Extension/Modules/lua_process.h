#ifndef PROCESS_H
#define PROCESS_H

#include "modulebase.h"
#include <QProcess>

namespace Extension
{
struct ProcessData
{
    ProcessData(QProcess *process) : p(process) {}
    ~ProcessData()
    {
        p->deleteLater();
        p = nullptr;
    }
    ProcessData &operator=(const ProcessData&) = delete;
    ProcessData(const ProcessData&) = delete;

    QProcess *p = nullptr;
    int eventCbRef = 0;

    static const char *MetaName;
    static ProcessData *checkProcess(lua_State *L, int pos);

    static int processGC(lua_State *L);
    static int start(lua_State *L);
    static int env(lua_State *L);
    static int setenv(lua_State *L);
    static int dir(lua_State *L);
    static int setdir(lua_State *L);
    static int pid(lua_State *L);
    static int terminate(lua_State *L);
    static int kill(lua_State *L);
    static int exitstate(lua_State *L);

    static int waitstart(lua_State *L);
    static int waitfinish(lua_State *L);
    static int waitwritten(lua_State *L);
    static int waitreadready(lua_State *L);

    static int readoutput(lua_State *L);
    static int readerror(lua_State *L);
    static int write(lua_State *L);

    static int onevent(lua_State *L);
};

class Process : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int create(lua_State *L);
    static int sysenv(lua_State *L);
};

}
#endif // PROCESS_H

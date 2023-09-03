#include "lua_process.h"
#include "Extension/Common/ext_common.h"
#include "Common/logger.h"

namespace Extension
{
const char *ProcessData::MetaName = "meta.kiko.process";

void Process::setup()
{
    const luaL_Reg processMembers[] = {
        {"start",  ProcessData::start},
        {"env",  ProcessData::env},
        {"setenv",  ProcessData::setenv},
        {"dir",  ProcessData::dir},
        {"setdir",  ProcessData::setdir},
        {"pid",  ProcessData::pid},
        {"terminate",  ProcessData::terminate},
        {"kill",  ProcessData::kill},
        {"exitstate",  ProcessData::exitstate},
        {"waitstart",  ProcessData::waitstart},
        {"waitfinish",  ProcessData::waitfinish},
        {"waitwritten",  ProcessData::waitwritten},
        {"waitreadready",  ProcessData::waitreadready},
        {"readoutput",  ProcessData::readoutput},
        {"readerror",  ProcessData::readerror},
        {"write",  ProcessData::write},
        {"onevent",  ProcessData::onevent},
        {"__gc",  ProcessData::processGC},
        {nullptr, nullptr}
    };
    registerMemberFuncs(ProcessData::MetaName, processMembers);

    const luaL_Reg funcs[] = {
        {"create", create},
        {"sysenv", sysenv},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "process"}, funcs);
}

ProcessData *ProcessData::checkProcess(lua_State *L, int pos)
{
    void *ud = luaL_checkudata(L, pos, ProcessData::MetaName);
    luaL_argcheck(L, ud != NULL, pos, "process expected");
    return *(ProcessData **)ud;
}

int Process::create(lua_State *L)
{
    ProcessData **p = (ProcessData **)lua_newuserdata(L, sizeof(ProcessData *));
    luaL_getmetatable(L, ProcessData::MetaName);
    lua_setmetatable(L, -2);
    ProcessData *d = new ProcessData(new QProcess);
    *p = d;
    QObject::connect((*p)->p, &QProcess::started, [=](){
        if (!d->eventCbRef) return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, d->eventCbRef);
        lua_pushstring(L, "start");  // t fname
        if (lua_gettable(L, -2) == LUA_TFUNCTION)
        {
            if (lua_pcall(L, 0, 0, 0))
            {
                Logger::logger()->log(Logger::Extension, "[process]" + QString(lua_tostring(L, -1)));
            }
        }
        lua_pop(L, 1);
    });
    QObject::connect((*p)->p, &QProcess::errorOccurred, [=](QProcess::ProcessError err){
        if (!d->eventCbRef) return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, d->eventCbRef);
        lua_pushstring(L, "error");  // t fname
        if (lua_gettable(L, -2) == LUA_TFUNCTION)
        {
            lua_pushinteger(L, static_cast<int>(err));
            if (lua_pcall(L, 1, 0, 0))
            {
                Logger::logger()->log(Logger::Extension, "[process]" + QString(lua_tostring(L, -1)));
            }
        }
        lua_pop(L, 1);
    });
    QObject::connect((*p)->p, (void (QProcess:: *)(int, QProcess::ExitStatus))&QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus){
        if (!d->eventCbRef) return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, d->eventCbRef);
        lua_pushstring(L, "finished");  // t fname
        if (lua_gettable(L, -2) == LUA_TFUNCTION)
        {
            lua_pushinteger(L, exitCode);
            lua_pushinteger(L, static_cast<int>(exitStatus));
            if (lua_pcall(L, 2, 0, 0))
            {
                Logger::logger()->log(Logger::Extension, "[process]" + QString(lua_tostring(L, -1)));
            }
        }
        lua_pop(L, 1);
    });
    QObject::connect((*p)->p, &QProcess::readyReadStandardOutput, [=](){
        if (!d->eventCbRef) return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, d->eventCbRef);
        lua_pushstring(L, "readready");  // t fname
        if (lua_gettable(L, -2) == LUA_TFUNCTION)
        {
            lua_pushinteger(L, 0);
            if (lua_pcall(L, 1, 0, 0))
            {
                Logger::logger()->log(Logger::Extension, "[process]" + QString(lua_tostring(L, -1)));
            }
        }
        lua_pop(L, 1);
    });
    QObject::connect((*p)->p, &QProcess::readyReadStandardError, [=](){
        if (!d->eventCbRef) return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, d->eventCbRef);
        lua_pushstring(L, "readready");  // t fname
        if (lua_gettable(L, -2) == LUA_TFUNCTION)
        {
            lua_pushinteger(L, 1);
            if (lua_pcall(L, 1, 0, 0))
            {
                Logger::logger()->log(Logger::Extension, "[process]" + QString(lua_tostring(L, -1)));
            }
        }
        lua_pop(L, 1);
    });
    return 1;
}

int Process::sysenv(lua_State *L)
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const auto keys = env.keys();
    lua_newtable(L); // table
    for (auto &k : keys)
    {
        lua_pushstring(L, k.toStdString().c_str()); // table key
        lua_pushstring(L, env.value(k).toStdString().c_str()); // table key
        lua_rawset(L, -3);
    }
    return 1;
}

int ProcessData::processGC(lua_State *L)
{
    ProcessData *p = checkProcess(L, 1);
    if (p)
    {
        if (p->eventCbRef)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, p->eventCbRef);
        }
        delete p;
    }
    return 0;
}

int ProcessData::start(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING)
    {
        return 0;
    }
    const QString program = lua_tostring(L, 2);
    QStringList params;
    if (lua_gettop(L) > 2)
    {
        params = getValue(L, true, 2).toStringList();
    }
    pd->p->start(program, params);
    return 0;
}

int ProcessData::env(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    const QProcessEnvironment env = pd->p->processEnvironment();
    const auto keys = env.keys();
    lua_newtable(L); // table
    for (auto &k : keys)
    {
        lua_pushstring(L, k.toStdString().c_str()); // table key
        lua_pushstring(L, env.value(k).toStdString().c_str()); // table key
        lua_rawset(L, -3);
    }
    return 1;
}

int ProcessData::setenv(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TTABLE)
    {
        return 0;
    }
    const QVariantMap envMap = getValue(L, true, 2).toMap();
    QProcessEnvironment env;
    for (auto iter = envMap.begin(); iter != envMap.end(); ++iter)
    {
        env.insert(iter.key(), iter.value().toString());
    }
    pd->p->setProcessEnvironment(env);
    return 0;
}

int ProcessData::dir(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    lua_pushstring(L, pd->p->workingDirectory().toStdString().c_str());
    return 1;
}

int ProcessData::setdir(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING)
    {
        return 0;
    }
    const QString workingDir = lua_tostring(L, 2);
    pd->p->setWorkingDirectory(workingDir);
    return 0;
}

int ProcessData::pid(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd)
    {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_pushinteger(L, pd->p->processId());
    return 1;
}

int ProcessData::terminate(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    pd->p->terminate();
    return 0;
}

int ProcessData::kill(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    pd->p->kill();
    return 0;
}

int ProcessData::exitstate(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    lua_pushinteger(L, pd->p->exitCode());
    lua_pushinteger(L, static_cast<int>(pd->p->exitStatus()));
    return 2;
}

int ProcessData::waitstart(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    int timeout = -1;
    if (lua_gettop(L) > 1 && lua_isinteger(L, 2))
    {
        timeout = lua_tointeger(L, 2);
    }
    pd->p->waitForStarted(timeout);
    return 0;
}

int ProcessData::waitfinish(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    int timeout = -1;
    if (lua_gettop(L) > 1 && lua_isinteger(L, 2))
    {
        timeout = lua_tointeger(L, 2);
    }
    pd->p->waitForFinished(timeout);
    return 0;
}

int ProcessData::waitwritten(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    int timeout = -1;
    if (lua_gettop(L) > 1 && lua_isinteger(L, 2))
    {
        timeout = lua_tointeger(L, 2);
    }
    pd->p->waitForBytesWritten(timeout);
    return 0;
}

int ProcessData::waitreadready(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    int timeout = -1;
    if (lua_gettop(L) > 1 && lua_isinteger(L, 2))
    {
        timeout = lua_tointeger(L, 2);
    }
    pd->p->waitForReadyRead(timeout);
    return 0;
}

int ProcessData::readoutput(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    const QByteArray output = pd->p->readAllStandardOutput();
    lua_pushlstring(L, output.constData(), output.size());
    return 1;
}

int ProcessData::readerror(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd) return 0;
    const QByteArray output = pd->p->readAllStandardError();
    lua_pushlstring(L, output.constData(), output.size());
    return 1;
}

int ProcessData::write(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd || lua_gettop(L) < 2 || !lua_isstring(L, 2)) return 0;
    size_t len = 0;
    const char *s = lua_tolstring(L, 2, &len);
    lua_pushinteger(L, pd->p->write(s, len));
    return 1;
}

int ProcessData::onevent(lua_State *L)
{
    ProcessData *pd = checkProcess(L, 1);
    if (!pd || lua_gettop(L) < 2 || !lua_istable(L, 2)) return 0;
    if (pd->eventCbRef != 0)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, pd->eventCbRef);
    }
    pd->eventCbRef = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}


}


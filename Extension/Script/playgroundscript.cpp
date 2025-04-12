#include "playgroundscript.h"
#include "Common/threadtask.h"
#include "globalobjects.h"

PlaygroundScript::PlaygroundScript() : ScriptBase()
{
    sType = ScriptType::PLAYGROUND;
    scriptMeta["id"] = "kiko.playground";
    scriptMeta["name"] = "playground";
    const struct luaL_Reg printlib [] = {
      {"print", print},
      {NULL, NULL}
    };
    registerFuncs("_G", printlib);
}

void PlaygroundScript::run(const QString &scriptContent)
{
    ThreadTask task(GlobalObjects::workThread);
    ScriptState state = task.Run([&](){
        return QVariant::fromValue(loadScriptStr(scriptContent));
    }).value<ScriptState>();
    if (!state)
    {
        printCallBack(state.info);
    }
}

void PlaygroundScript::setPrintCallback(PlaygroundScript::PrintCallback cb)
{
    printCallBack = cb;
}

int PlaygroundScript::print(lua_State *L)
{
    int n = lua_gettop(L);  /* number of arguments */
    lua_getglobal(L, "tostring");
    PlaygroundScript *script = static_cast<PlaygroundScript *>(ScriptBase::getScript(L));
    QString content;
    for (int i=1; i<=n; i++)
    {
        const char *s;
        size_t l;
        lua_pushvalue(L, -1);  /* function to be called */
        lua_pushvalue(L, i);   /* value to print */
        lua_call(L, 1, 1);
        s = lua_tolstring(L, -1, &l);  /* get result */
        if (s == NULL)
            return luaL_error(L, "'tostring' must return a string to 'print'");
        if (i>1) content.append('\t');
        content.append(QByteArray(s, l));
        lua_pop(L, 1);  /* pop result */
    }
    script->printCallBack(content);
    return 0;
}

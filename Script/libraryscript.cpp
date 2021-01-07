#include "libraryscript.h"

LibraryScript::LibraryScript() : ScriptBase()
{

}

ScriptState LibraryScript::load(const QString &scriptPath)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo = loadScript(scriptPath);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    matchSupported = checkType("match", LUA_TFUNCTION);
    bool hasSearchFunc = checkType("search", LUA_TFUNCTION);
    bool hasEpFunc = checkType("getep", LUA_TFUNCTION);
    hasTagFunc = checkType("gettags", LUA_TFUNCTION);
    if(!matchSupported || !(hasSearchFunc && hasEpFunc)) return ScriptState(ScriptState::S_ERROR, "search+getep/match function is not found");
    QVariant menus = get("menus"); //[{title=xx, id=xx}...]
    if(menus.type() == QVariant::List)
    {
        auto mlist = menus.toList();
        for(auto &m : mlist)
        {
            if(m.type() == QVariant::Map)
            {
                auto mObj = m.toMap();
                QString title = mObj.value("title").toString(), id =mObj.value("id").toString();
                if(title.isEmpty() || id.isEmpty()) continue;
                menuItems.append({title, id});
            }
        }
    }
    return ScriptState(ScriptState::S_NORM);
}


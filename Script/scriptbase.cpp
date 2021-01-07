#include "scriptbase.h"
#include <QFile>
#include <QVariant>
#include "Common/network.h"
namespace
{
static const luaL_Reg kikoFuncs[] = {
    {"", nullptr}
};
}

ScriptBase::ScriptBase() : L(nullptr)
{
    L = luaL_newstate();
    if(L)
    {
        luaL_openlibs(L);
        lua_newtable(L);
        luaL_setfuncs(L, kikoFuncs, 0);
        lua_setglobal(L, "kiko");
    }
}

ScriptBase::~ScriptBase()
{
    if(L) lua_close(L);
}

QString ScriptBase::setOption(int index, const QString &value)
{
    if(scriptSettings.size()<=index) return "OutRange";
    scriptSettings[index].value = value;
    QString errInfo;
    call("setoption", {scriptSettings[index].title, value}, 0, errInfo);
    return errInfo;
}

QString ScriptBase::loadScript(const QString &path)
{
    QFile luaFile(path);
    luaFile.open(QFile::ReadOnly);
    if(!luaFile.isOpen())
    {
        return QObject::tr("Open Script File Failed");
    }
    QString luaScript(luaFile.readAll());
    QString errInfo;
    if(luaL_loadstring(L,luaScript.toStdString().c_str()) || lua_pcall(L,0,0,0))
    {
        errInfo="Script Error: "+ QString(lua_tostring(L, -1));
        lua_pop(L,1);
        return errInfo;
    }
    //get script meta info
    errInfo = getMeta(path);
    if(!errInfo.isEmpty()) return errInfo;
    //get script settings
    loadSettings();
    int suffixPos = path.lastIndexOf('.');
    QFile settingSaved(path.mid(0,suffixPos)+".json");
    if(settingSaved.open(QFile::ReadOnly))
    {
        QJsonObject sObj(Network::toJson(settingSaved.readAll()).object());
        QHash<QString, ScriptSettingItem *> itemHash;
        for(auto &item: scriptSettings)
        {
            itemHash[item.title] = &item;
        }
        for(auto iter = sObj.constBegin(); iter != sObj.constEnd(); ++iter)
        {
            ScriptSettingItem *item = itemHash.value(iter.key(), nullptr);
            if(item)
            {
                item->value = iter.value().toString("");
            }
        }
    }
    return errInfo;
}

QVariantList ScriptBase::call(const char *fname, const QVariantList &params, int nRet, QString &errInfo)
{
    if(lua_getglobal(L, fname) != LUA_TFUNCTION) return QVariantList();
    for(auto &p : params)
    {
        pushValue(p);
    }
    if(lua_pcall(L, params.size(), nRet, 0))
    {
        errInfo="Script Error: "+ QString(lua_tostring(L, -1));
        lua_pop(L,1);
        return QVariantList();
    }
    QVariantList rets;
    for(int i=0; i<nRet; ++i)
    {
        rets.append(getValue());
        lua_pop(L, 1);
    }
    std::reverse(rets.begin(), rets.end());
    return rets;
}

QVariant ScriptBase::get(const char *name)
{
    lua_getglobal(L, name);
    QVariant val = getValue();
    lua_pop(L, 1);
    return val;
}

void ScriptBase::set(const char *name, const QVariant &val)
{
    pushValue(val);
    lua_setglobal(L, name);
}

int ScriptBase::setTable(const char *tname, const QVariant &key, const QVariant &val)
{
    int type = lua_getglobal(L, tname);
    if(type == LUA_TTABLE)
    {
        pushValue(key);
        pushValue(val);
        lua_settable(L, -3);
        lua_pop(L, 1);
        return 0;
    }
    lua_pop(L, 1);
    return -1;
}

bool ScriptBase::checkType(const char *name, int type)
{
    int ct = lua_getglobal(L, name);
    lua_pop(L, 1);
    return ct == type;
}

void ScriptBase::pushValue(const QVariant &val)
{
    switch (val.type())
    {
    case QVariant::Int:
    case QVariant::Double:
        lua_pushnumber(L, val.toDouble());
        break;
    case QVariant::String:
        lua_pushstring(L, val.toString().toStdString().c_str());
        break;
    case QVariant::Bool:
        lua_pushboolean(L, val.toBool());
        break;
    case QVariant::Invalid:
        lua_pushnil(L);
        break;
    case QVariant::List:
    {
        lua_newtable(L); // table
        const auto &l = val.toList();
        for(int i=0; i<l.size(); ++i)
        {
            pushValue(l.value(i));
            lua_rawseti(L, -2, i+1);
        }
        break;
    }
    case QVariant::Map:
    {
        lua_newtable(L); // table
        const auto &m = val.toMap();
        for(auto i = m.constBegin(); i!=m.constEnd(); ++i)
        {
            lua_pushstring(L, i.key().toStdString().c_str()); // table key
            pushValue(i.value()); // table key value
            lua_rawset(L, -3);
        }
        break;
    }
    default:
        lua_pushnil(L);
        break;
    }
}

QVariant ScriptBase::getValue()
{
    if(lua_gettop(L)==0) return QVariant();
    switch (lua_type(L, -1))
    {
    case LUA_TNUMBER:
    {
        double d = lua_tonumber(L, -1);
        return d;
    }
    case LUA_TBOOLEAN:
    {
        return bool(lua_toboolean(L, -1));
    }
    case LUA_TSTRING:
    {
        size_t len = 0;
        const char *s = (const char *)lua_tolstring(L, -1, &len);
        return QByteArray(s, len);
    }
    case LUA_TTABLE:
    {
        // If all keys are integers, and they're in sequence, take it as an list.
        int count = 0;
        for (int i = 1; ; ++i)
        {
            lua_pushinteger(L, i); // n
            lua_gettable(L, -2); // t[n]
            bool empty = lua_isnil(L, -1); // t[n]
            lua_pop(L, 1); // -
            if (empty)
            {
                count = i - 1;
                break;
            }
        }
        size_t length = getTableLength(-1);
        if(count < 0 || count != length) // map
        {
            QVariantMap map;
            lua_pushnil(L); // t nil
            while (lua_next(L, -2)) { // t key value
                if (lua_type(L, -2) != LUA_TSTRING)
                {
                    luaL_error(L, "key must be a string, but got %s",
                               lua_typename(L, lua_type(L, -2)));
                }
                map[lua_tostring(L, -2)] = getValue();
                lua_pop(L, 1); // key
            }
            return map;
        }
        else  // array
        {
            QVariantList list;
            for (int i = 0; ;++i) {
                lua_pushinteger(L, i + 1); //t n1
                lua_gettable(L, -2); //t t[n1]
                if (lua_isnil(L, -1))
                    break;
                list.append(getValue());
                lua_pop(L, 1); // -
            }
            return list;
        }
    }
    default:
        return QVariant();
    }
}

size_t ScriptBase::getTableLength(int pos)
{
    if (pos < 0)  pos = lua_gettop(L) + (pos + 1);
    lua_pushnil(L); // nil
    size_t length = 0;
    while (lua_next(L, pos))  // key value
    {
        ++length;
        lua_pop(L, 1); // key
    }
    return length;
}

QString ScriptBase::getMeta(const QString &scriptPath)
{
    QString errInfo;
    QVariant scriptInfo = get("info");
    if(!scriptInfo.canConvert(QVariant::Map))
    {
        errInfo = "Script Error: no info";
        return errInfo;
    }
    QVariantMap scriptInfoMap = scriptInfo.toMap();
    scriptMeta.clear();
    for(auto iter = scriptInfoMap.constBegin(); iter != scriptInfoMap.constEnd(); ++iter)
    {
        if(iter.value().canConvert(QVariant::String))
        {
            scriptMeta[iter.key()] = iter.value().toString();
        }
    }
    scriptMeta["path"] = scriptPath;
    if(!scriptMeta.contains("id")) scriptMeta["id"] = QFileInfo(scriptPath).baseName();
    if(!scriptMeta.contains("name")) scriptMeta["name"] = scriptMeta["id"];
    return errInfo;
}

void ScriptBase::loadSettings()
{
    //settings = {key = {type = str/strlist, default='', desc='', choices=',..,'},...}
    QString errInfo;
    QVariant settings = get("settings");
    if(!settings.canConvert(QVariant::Map)) return;
    QVariantMap settingMap = settings.toMap();
    scriptSettings.clear();
    for(auto iter = settingMap.constBegin(); iter != settingMap.constEnd(); ++iter)
    {
        if(iter.value().canConvert(QVariant::Map))
        {
            QVariantMap settingItemMap = iter.value().toMap();
            scriptSettings.append(
            {
                settingItemMap.value("type", "str").toString() == "str"?ScriptSettingItem::ValueType::SS_STRING :  ScriptSettingItem::ValueType::SS_STRINGLIST,
                iter.key(),
                settingItemMap.value("desc", "").toString(),
                settingItemMap.value("choices", "").toString(),
                settingItemMap.value("default", "").toString()
            });
        }
    }
}

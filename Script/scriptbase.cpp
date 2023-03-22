#include "scriptbase.h"
#include "modules/lua_net.h"
#include "modules/lua_util.h"
#include "modules/lua_xmlreader.h"
#include "modules/lua_htmlparser.h"
#include "modules/lua_regex.h"
#include <QFile>
#include <QVariant>
#include "Common/network.h"
#include "Common/htmlparsersax.h"
#include "Common/logger.h"
#include "globalobjects.h"

#define LOG_INFO(info, scriptId) Logger::logger()->log(Logger::Script, QString("[%1]%2").arg(scriptId, info))
#define LOG_ERROR(info, scriptId) Logger::logger()->log(Logger::Script, QString("[ERROR][%1]%2").arg(scriptId, info))
namespace
{
}
ScriptBase::ScriptBase() : L(nullptr), settingsUpdated(false),hasSetOptionFunc(false),sType(ScriptType::UNKNOWN_STYPE)
{
    L = luaL_newstate();
    if(L)
    {
        luaL_openlibs(L);
        LuaModule::LuaUtil(L).setup();
        LuaModule::Net(L).setup();
        LuaModule::XmlReader(L).setup();
        LuaModule::HtmlParser(L).setup();
        LuaModule::Regex(L).setup();

        lua_pushstring(L, "kiko_scriptobj");
        lua_pushlightuserdata(L, (void *)this);
        lua_settable(L, LUA_REGISTRYINDEX);
    }
}

ScriptBase::~ScriptBase()
{
    if(L)
    {
        QMutexLocker locker(&scriptLock);
        lua_close(L);
        L = nullptr;
    }
    if(settingsUpdated)
    {
        QString scriptPath(scriptMeta["path"]);
        int suffixPos = scriptPath.lastIndexOf('.');
        QFile settingSaved(scriptPath.mid(0,suffixPos)+".json");
        if(settingSaved.open(QFile::WriteOnly))
        {
            QMap<QString, QVariant> settingMap, searchSettingMap;
            for(const auto &item : scriptSettings)
                settingMap[item.key] = item.value;
            for(const auto &item : searchSettingItems)
            {
                if(item.save) searchSettingMap[item.key] = item.value;
            }
            QJsonObject settingObj
            {
                {"scriptSettings", QJsonObject::fromVariantMap(settingMap)},
                {"searchSettings", QJsonObject::fromVariantMap(searchSettingMap)}
            };
            QJsonDocument doc(settingObj);
            settingSaved.write(doc.toJson(QJsonDocument::Indented));
        }
        settingsUpdated = false;
    }
}

ScriptState ScriptBase::setOption(int index, const QString &value, bool callLua)
{
    if(scriptSettings.size()<=index) return "OutRange";
    scriptSettings[index].value = value;
    settingsUpdated = true;
    QString errInfo;
    if(callLua)
    {
        setTable(luaSettingsTable, scriptSettings[index].key, value);
        if(hasSetOptionFunc)
            call(luaSetOptionFunc, {scriptSettings[index].key, value}, 0, errInfo);
    }
    return errInfo;
}

ScriptState ScriptBase::setOption(const QString &key, const QString &value, bool callLua)
{
    QString errInfo;
    for(auto &item : scriptSettings)
    {
        if(item.key == key)
        {
            item.value = value;
            settingsUpdated = true;
            if(callLua)
            {
                setTable(luaSettingsTable, key, value);
                if(hasSetOptionFunc)
                    call(luaSetOptionFunc, {key, value}, 0, errInfo);
            }
            break;
        }
    }
    return errInfo;
}

ScriptState ScriptBase::setSearchOption(const QString &key, const QString &value)
{
    for(auto &item : searchSettingItems)
    {
        if(item.key == key)
        {
            item.value = value;
            if(item.save) settingsUpdated = true;
            break;
        }
    }
    return "";
}

ScriptState ScriptBase::scriptMenuClick(const QString &mid)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    call(scriptMenuFunc, {mid}, 0, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    return ScriptState(ScriptState::S_NORM);
}

ScriptState ScriptBase::loadScript(const QString &path)
{
    if(!L) return "Script Error: Wrong Lua State";
    QFile luaFile(path);
    luaFile.open(QFile::ReadOnly);
    if(!luaFile.isOpen()) return "Open Script File Failed";
    QString luaScript(luaFile.readAll());
    QString errInfo = loadScriptStr(luaScript);
    if(!errInfo.isEmpty()) return errInfo;
    errInfo = loadMeta(path);
    if(!errInfo.isEmpty()) return errInfo;
    loadSettings(path);
    loadSearchSettings(path);
    loadScriptMenus();
    return errInfo;
}

QVariantList ScriptBase::call(const char *fname, const QVariantList &params, int nRet, QString &errInfo)
{
    if(!L)
    {
        errInfo = "Wrong Lua State";
        LOG_ERROR(errInfo, "Lua");
        return QVariantList();
    }
    if(lua_getglobal(L, fname) != LUA_TFUNCTION)
    {
        errInfo = QString("%1 is not founded").arg(fname);
        LOG_ERROR(errInfo, id());
        return QVariantList();
    }
    for(auto &p : params)
    {
        pushValue(L, p);
    }
    if(lua_pcall(L, params.size(), nRet, 0))
    {
        errInfo=QString(lua_tostring(L, -1));
        lua_pop(L,1);
        LOG_ERROR(errInfo, id());
        return QVariantList();
    }
    QVariantList rets;
    for(int i=0; i<nRet; ++i)
    {
        rets.append(getValue(L));
        lua_pop(L, 1);
    }
    std::reverse(rets.begin(), rets.end());
    return rets;
}

QVariant ScriptBase::get(const char *name)
{
    if(!L) return QVariant();
    lua_getglobal(L, name);
    QVariant val = getValue(L);
    lua_pop(L, 1);
    return val;
}

void ScriptBase::set(const char *name, const QVariant &val)
{
    if(!L) return;
    pushValue(L, val);
    lua_setglobal(L, name);
}

ScriptState ScriptBase::setTable(const char *tname, const QVariant &key, const QVariant &val)
{
    if(!L) return "Script Error: Wrong Lua State";
    int type = lua_getglobal(L, tname);
    if(type == LUA_TTABLE)
    {
        pushValue(L, key);
        pushValue(L, val);
        lua_settable(L, -3);
        lua_pop(L, 1);
        return "";
    }
    lua_pop(L, 1);
    return QString("No table with name %1").arg(tname);
}

bool ScriptBase::checkType(const char *name, int type)
{
    if(!L) return false;
    int ct = lua_getglobal(L, name);
    lua_pop(L, 1);
    return ct == type;
}

void ScriptBase::pushValue(lua_State *L, const QVariant &val)
{
    if(!L) return;
    switch (val.type())
    {
    case QVariant::Int:
	case QVariant::LongLong:
	case QVariant::UInt:
	case QVariant::ULongLong:
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
            pushValue(L, l.value(i));
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
            pushValue(L, i.value()); // table key value
            lua_rawset(L, -3);
        }
        break;
    }
    default:
        lua_pushnil(L);
        break;
    }
}

QVariant ScriptBase::getValue(lua_State *L, bool useString)
{
    if(!L) return QVariant();
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
		if (useString) return QString(s);
		else return QByteArray(s, len);
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
        size_t length = getTableLength(L, -1);
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
                QString key(lua_tostring(L, -2));
                map[key] = getValue(L, useString);
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
				{
					lua_pop(L, 1);
					break;
				}
                list.append(getValue(L, useString));
                lua_pop(L, 1); // -
            }
            return list;
        }
    }
    default:
        return QVariant();
    }
}

int ScriptBase::getTableLength(lua_State *L, int pos)
{
    if(!L) return 0;
    if (pos < 0)  pos = lua_gettop(L) + (pos + 1);
    lua_pushnil(L); // nil
    int length = 0;
    while (lua_next(L, pos))  // key value
    {
        ++length;
        lua_pop(L, 1); // key
    }
    return length;
}

ScriptBase *ScriptBase::getScript(lua_State *L)
{
    lua_pushstring(L, "kiko_scriptobj");
    lua_gettable(L, LUA_REGISTRYINDEX);
    ScriptBase *script = (ScriptBase *)lua_topointer(L, -1);
    lua_pop(L, 1);
    return script;
}

QString ScriptBase::loadMeta(const QString &scriptPath)
{
    QString errInfo;
    QVariant scriptInfo = get(luaMetaTable);
    if(!scriptInfo.canConvert(QVariant::Map))
    {
        errInfo = "Script Error: no info table";
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
    QFileInfo scriptFileInfo(scriptPath);
    scriptMeta["path"] = scriptFileInfo.absoluteFilePath();
    scriptMeta["time"] = QString::number(scriptFileInfo.fileTime(QFile::FileModificationTime).toSecsSinceEpoch());
    if(!scriptMeta.contains("id")) scriptMeta["id"] = scriptMeta["path"];
    if(!scriptMeta.contains("name")) scriptMeta["name"] = scriptFileInfo.baseName();
    if(scriptMeta.contains("min_kiko"))
    {
        QStringList minVer(scriptMeta["min_kiko"].split('.', Qt::SkipEmptyParts));
        QStringList curVer(QString(GlobalObjects::kikoVersion).split('.', Qt::SkipEmptyParts));
        bool versionMismatch = false;
        if(minVer.size() == curVer.size())
        {
            for(int i = 0; i < minVer.size(); ++i)
            {
                int cv = curVer[i].toInt(), mv = minVer[i].toInt();
                if(cv < mv)
                {
                    versionMismatch = true;
                    break;
                }
                else if(cv > mv)
                {
                    versionMismatch = false;
                    break;
                }
            }
        }
        if(versionMismatch)
        {
            errInfo = QString("Script Version Mismatch, min: %1, cur: %2").arg(scriptMeta["min_kiko"], GlobalObjects::kikoVersion);
        }
    }
    return errInfo;
}

void ScriptBase::loadSettings(const QString &scriptPath)
{
    //settings = {key = {title = 'xx', default='', desc='', choices=',..,', group=''},...}
    QString errInfo;
    QVariant settings = get(luaSettingsTable);
    hasSetOptionFunc = checkType(luaSetOptionFunc, LUA_TFUNCTION);
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
                settingItemMap.value("title").toString(),
                settingItemMap.value("desc", "").toString(),
                settingItemMap.value("choices", "").toString(),
                settingItemMap.value("group", "").toString(),
                iter.key(),
                settingItemMap.value("default", "").toString()
            });
        }
    }

    int suffixPos = scriptPath.lastIndexOf('.');
    QFile settingSaved(scriptPath.mid(0,suffixPos)+".json");
    if(settingSaved.open(QFile::ReadOnly))
    {
        QJsonObject sObj(Network::toJson(settingSaved.readAll()).object());
        if(sObj.contains("scriptSettings") && sObj["scriptSettings"].isObject())  // 0.9.1 setting format
        {
            sObj = sObj["scriptSettings"].toObject();
        }
        else
        {
            settingsUpdated = true;  // from old version
        }
        QHash<QString, ScriptSettingItem *> itemHash;
        for(auto &item: scriptSettings)
        {
            itemHash[item.key] = &item;
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

    lua_newtable(L);
    for(const auto &item: scriptSettings)
    {
        lua_pushstring(L, item.key.toStdString().c_str());
        lua_pushstring(L, item.value.toStdString().c_str());
        lua_settable(L, -3);
    }
    lua_setglobal(L, luaSettingsTable);
}

void ScriptBase::loadSearchSettings(const QString &scriptPath)
{
    //settings = {key = {title = 'xx', default='', desc='', choices=',..,', display_type=0/1/2/3},...}
    QString errInfo;
    QVariant settings = get(luaSearchSettingsTable);
    if(!settings.canConvert(QVariant::Map)) return;
    QVariantMap settingMap = settings.toMap();
    searchSettingItems.clear();
    for(auto iter = settingMap.constBegin(); iter != settingMap.constEnd(); ++iter)
    {
        if(iter.value().canConvert(QVariant::Map))
        {
            QVariantMap settingItemMap = iter.value().toMap();
            int dtype = settingItemMap.value("display_type").toInt();
            if(dtype < 0 || dtype >= SearchSettingItem::DisplayType::UNKNOWN)
            {
                dtype = 0;
            }
            bool save = settingItemMap.value("save", true).toBool();
            searchSettingItems.append(
            {
                SearchSettingItem::DisplayType(dtype),
                settingItemMap.value("title").toString(),
                settingItemMap.value("desc", "").toString(),
                settingItemMap.value("choices", "").toString(),
                iter.key(),
                settingItemMap.value("default", "").toString(),
                save
            });
        }
    }
    if(searchSettingItems.isEmpty()) return;
    int suffixPos = scriptPath.lastIndexOf('.');
    QFile settingSaved(scriptPath.mid(0,suffixPos)+".json");
    if(settingSaved.open(QFile::ReadOnly))
    {
        QJsonObject sObj(Network::toJson(settingSaved.readAll()).object());
        if(!sObj.contains("searchSettings") || !sObj["searchSettings"].isObject()) return;
        sObj = sObj["searchSettings"].toObject();
        QHash<QString, SearchSettingItem *> itemHash;
        for(auto &item: searchSettingItems)
        {
            itemHash[item.key] = &item;
        }
        for(auto iter = sObj.constBegin(); iter != sObj.constEnd(); ++iter)
        {
            SearchSettingItem *item = itemHash.value(iter.key(), nullptr);
            if(item)
            {
                item->value = iter.value().toString("");
            }
        }
    }
}

void ScriptBase::loadScriptMenus()
{
    scriptMenuItems.clear();
    QVariant menus = get(scriptMenuTable); //[{title=xx, id=xx}...]
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
                scriptMenuItems.append({title, id});
            }
        }
    }
}

void ScriptBase::registerFuncs(const char *tname, const luaL_Reg *funcs)
{
    if(!L) return;
    lua_getglobal(L, tname);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);
    }
    luaL_setfuncs(L, funcs, 0);
    lua_setglobal(L, tname);
}

ScriptState ScriptBase::loadScriptStr(const QString &content)
{
    QString errInfo;
    if(luaL_loadstring(L, content.toStdString().c_str()) || lua_pcall(L,0,0,0))
    {
        errInfo="Script Error: "+ QString(lua_tostring(L, -1));
        lua_pop(L,1);
        return errInfo;
    }
    return errInfo;
}

void ScriptBase::addSearchOptions(QVariantList &params)
{
    if(!searchSettingItems.empty())
    {
        QVariantMap optionMap;
        for(const auto &item : searchSettingItems)
        {
            optionMap[item.key] = item.value;
        }
        params.append(optionMap);
    }
}

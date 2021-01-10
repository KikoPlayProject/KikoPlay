#include "scriptbase.h"
#include <QFile>
#include <QVariant>
#include "Common/network.h"
namespace
{
static int httpget(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //url <query> <header>
        if(params==0 || params>3) break;
        if(lua_type(L, 1)!=LUA_TSTRING) break;
        const char *curl = luaL_checkstring(L,1);
        QUrlQuery query;
        QStringList headers;
        if(params > 1)  //has query
        {
            lua_pushvalue(L, 2);
            auto q = ScriptBase::getValue(L);
            lua_pop(L, 1);
            if(q.type()!=QVariant::Map) break;
            auto qmap = q.toMap();
            for(auto iter=qmap.constBegin(); iter!=qmap.constEnd(); ++iter)
            {
                query.addQueryItem(iter.key(),iter.value().toString());
            }
        }
        if(params > 2)  //has header
        {
            auto h = ScriptBase::getValue(L);
            if(h.type()!=QVariant::Map) break;
            auto hmap = h.toMap();
            for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
            {
                headers<<iter.key()<<iter.value().toString();
            }
        }
        QString errInfo;
        QString content;
        try
        {
            content=Network::httpGet(curl,query,headers);
        }
        catch(Network::NetworkError &err)
        {
            errInfo=err.errorInfo;
        }
        if(errInfo.isEmpty()) lua_pushnil(L);
        else lua_pushstring(L,errInfo.toStdString().c_str());
        lua_pushstring(L,content.toStdString().c_str());
        return 2;
    }while(false);
    lua_pushstring(L, "httpget: param error");
    lua_pushnil(L);
    return 2;
}
static int httppost(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //url <data> <header>
        if(params<2 || params>3) break;
        if(lua_type(L, 1)!=LUA_TSTRING) break;
        if(lua_type(L, 2)!=LUA_TSTRING) break;
        const char *curl = luaL_checkstring(L,1);
        size_t dataLength = 0;
        const char *data = lua_tolstring(L, 2, &dataLength);
        QByteArray cdata(data, dataLength);
        QStringList headers;
        if(params > 2)  //has header
        {
            auto h = ScriptBase::getValue(L);
            if(h.type()!=QVariant::Map) break;
            auto hmap = h.toMap();
            for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
            {
                headers<<iter.key()<<iter.value().toString();
            }
        }
        QString errInfo;
        QString content;
        try
        {
            content=Network::httpPost(curl,cdata,headers);
        }
        catch(Network::NetworkError &err)
        {
            errInfo=err.errorInfo;
        }
        if(errInfo.isEmpty()) lua_pushnil(L);
        else lua_pushstring(L,errInfo.toStdString().c_str());
        lua_pushstring(L,content.toStdString().c_str());
        return 2;
    }while(false);
    lua_pushstring(L, "httppost: param error");
    lua_pushnil(L);
    return 2;
}
static int json2table(lua_State *L)
{
    int params = lua_gettop(L);  //jsonstr
    if(params!=1 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "json2table: param error");
        lua_pushnil(L);
        return 2;
    }
    size_t dataLength = 0;
    const char *data = lua_tolstring(L, 2, &dataLength);
    QByteArray cdata(data, dataLength);
    try {
       auto jdoc = Network::toJson(cdata);
       lua_pushnil(L);
       if(jdoc.isArray())
       {
           ScriptBase::pushValue(L, jdoc.array().toVariantList());
       }
       else if(jdoc.isObject())
       {
           ScriptBase::pushValue(L, jdoc.object().toVariantMap());
       }
       else
       {
           lua_newtable(L);
       }
       return 2;
    } catch (Network::NetworkError &err) {
        lua_pushstring(L,err.errorInfo.toStdString().c_str());
        lua_pushnil(L);
        return 2;
    }
}
static int httpGetBatch(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //urls([u1,u2,...]) <querys([{xx=xx,...},{xx=xx,...},...])> <headers([{xx=xx,..},{xx=xx,..},...])>
        if(params==0 || params>3) break;
        if(lua_type(L, 1)!=LUA_TTABLE) break;
        lua_pushvalue(L, 1);
        auto us = ScriptBase::getValue(L);
        lua_pop(L, 1);
        if(!us.canConvert(QVariant::StringList)) break;
        auto urls = us.toStringList();
        QList<QUrlQuery> querys;
        QList<QStringList> headers;
        if(params > 1)  //has query
        {
            lua_pushvalue(L, 2);
            auto q = ScriptBase::getValue(L);
            lua_pop(L, 1);
            if(q.type()!=QVariant::List) break;
            auto qs = q.toList();
            if(qs.size()>0 && qs.size()!=urls.size()) break;
            for(auto &qobj : qs)
            {
                auto qmap = qobj.toMap();
                QUrlQuery query;
                for(auto iter=qmap.constBegin(); iter!=qmap.constEnd(); ++iter)
                {
                    query.addQueryItem(iter.key(),iter.value().toString());
                }
                querys.append(query);
            }
        }
        if(params > 2)  //has header
        {
            auto h = ScriptBase::getValue(L);
            if(h.type()!=QVariant::List) break;
            auto hs = h.toList();
            if(hs.size()>0 && hs.size()!=urls.size()) break;
            for(auto &hobj : hs)
            {
                auto hmap = hobj.toMap();
                QStringList header;
                for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
                {
                    header<<iter.key()<<iter.value().toString();
                }
                headers.append(headers);
            }
        }
        QList<QPair<QString, QByteArray>> content = Network::httpGetBatch(urls,querys,headers); //[[hasError, content], [], ...]
        lua_pushnil(L);
        lua_newtable(L); // table
        for(int i=0; i<content.size(); ++i)
        {
            lua_newtable(L); // table table
            if(content[i].first.isEmpty())
                lua_pushnil(L);  // table table nil
            else
                lua_pushstring(L, content[i].first.toStdString().c_str()); // table table errStr
            lua_rawseti(L, -2, 1); // table table
            lua_pushstring(L, content[i].second.constData()); //table table data
            lua_rawseti(L, -2, 2); // table table
            lua_rawseti(L, -2, i+1); //table
        }
        return 2;
    }while(false);
    lua_pushstring(L, "httpget: param error");
    lua_pushnil(L);
    return 2;
}
static int compress(lua_State *L)
{
    int params = lua_gettop(L);  //jsonstr
    if(params==0 || params > 2 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "decompress: param error");
        lua_pushnil(L);
        return 2;
    }
    bool useGzip = false;
    if(params == 2 && lua_type(L, 2)==LUA_TSTRING)
    {
        const char *method = lua_tostring(L, 2);
        if(strcmp(method, "gzip")==0) useGzip = true;
    }
    size_t dataLength = 0;
    const char *data = lua_tolstring(L, 2, &dataLength);
    QByteArray cdata(data, dataLength), outdata;
    int ret = 0;
    if(useGzip) ret = Network::gzipCompress(cdata, outdata);
    else ret = Network::gzipCompress(cdata, outdata);
    if(ret!=0)
    {
        lua_pushstring(L, "compress: data error");
        lua_pushnil(L);
        return 2;
    }
    lua_pushnil(L);
    lua_pushlstring(L, outdata.constData(), outdata.size());
    return 2;
}
static int decompress(lua_State *L)
{
    int params = lua_gettop(L);  //jsonstr
    if(params==0 || params > 2 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "decompress: param error");
        lua_pushnil(L);
        return 2;
    }
    bool useGzip = false;
    if(params == 2 && lua_type(L, 2)==LUA_TSTRING)
    {
        const char *method = lua_tostring(L, 2);
        if(strcmp(method, "gzip")==0) useGzip = true;
    }
    size_t dataLength = 0;
    const char *data = lua_tolstring(L, 2, &dataLength);
    QByteArray cdata(data, dataLength), outdata;
    int ret = 0;
    if(useGzip) ret = Network::gzipDecompress(cdata, outdata);
    else ret = Network::decompress(cdata, outdata);
    if(ret!=0)
    {
        lua_pushstring(L, "decompress: data error");
        lua_pushnil(L);
        return 2;
    }
    lua_pushnil(L);
    lua_pushlstring(L, outdata.constData(), outdata.size());
    return 2;
}
static int writeSetting(lua_State *L)
{
    int params = lua_gettop(L);  //key value
    if(params!=2 || lua_type(L, 1)!=LUA_TSTRING || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushstring(L, "writesetting: param error");
        return 1;
    }
    lua_pushstring(L, "kiko_scriptobj");
    lua_gettable(L, LUA_REGISTRYINDEX);
    ScriptBase *script = (ScriptBase *)lua_topointer(L, -1);
    lua_pop(L, 1);
    QString errInfo = script->setOption(lua_tostring(L, 1), lua_tostring(L, 2), false);
    if(errInfo.isEmpty()) lua_pushnil(L);
    else lua_pushstring(L, errInfo.toStdString().c_str());
    return 1;
}
// XmlReader-------------
static int xmlreader (lua_State *L)
{
    int n = lua_gettop(L);
    const char *data = nullptr;
    if(n > 0 && lua_type(L, 1)==LUA_TSTRING)
    {
        data = lua_tostring(L, -1);
    }
    QXmlStreamReader **reader = (QXmlStreamReader **)lua_newuserdata(L, sizeof(QXmlStreamReader *));
    luaL_getmetatable(L, "meta.kiko.xmlreader");
    lua_setmetatable(L, -2);  // reader meta
    *reader = new QXmlStreamReader(data); //meta
    return 1;
}
static QXmlStreamReader *checkXmlReader(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "meta.kiko.xmlreader");
    luaL_argcheck(L, ud != NULL, 1, "`kiko.xmlreader' expected");
    return *(QXmlStreamReader **)ud;
}
static int xrAddData(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    if(lua_type(L, 2)==LUA_TSTRING)
    {
        const char *data = lua_tostring(L, 2);
        reader->addData(data);
    }
    return 0;
}
static int xrClear(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    reader->clear();
    return 0;
}
static int xrEnd(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool atEnd = reader->atEnd();
    lua_pushboolean(L, atEnd);
    return 1;
}
static int xrReadNext(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    reader->readNext();
    return 0;
}
static int xrStartElem(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool isStartElem = reader->isStartElement();
    lua_pushboolean(L, isStartElem);
    return 1;
}
static int xrEndElem(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool isEndElem = reader->isEndElement();
    lua_pushboolean(L, isEndElem);
    return 1;
}
static int xrName(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    lua_pushstring(L, reader->name().toString().toStdString().c_str());
    return 1;
}
static int xrAttr(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    if(lua_gettop(L) == 0 || lua_type(L, -1)!=LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    const char *attrName = lua_tostring(L, -1);
    lua_pushstring(L, reader->attributes().value(attrName).toString().toStdString().c_str());
    return 1;
}
static int xrReadElemText(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    lua_pushstring(L, reader->readElementText().toStdString().c_str());
    return 1;
}
static int xrError(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    QString errInfo = reader->errorString();
    if(errInfo.isEmpty()) lua_pushnil(L);
    else lua_pushstring(L, errInfo.toStdString().c_str());
    return 1;
}
static int xmlreaderGC (lua_State *L) {
    QXmlStreamReader *reader = checkXmlReader(L);
    if(reader) delete reader;
    return 0;
}
//XmlReader End------------------
static const luaL_Reg kikoFuncs[] = {
    {"httpget", httpget},
    {"httpgetbatch", httpGetBatch},
    {"httppost", httppost},
    {"json2table", json2table},
    {"compress", compress},
    {"decompress", decompress},
    {"writesetting", writeSetting},
    {"xmlreader", xmlreader},
    {nullptr, nullptr}
};
static const luaL_Reg xmlreaderFuncs[] = {
    {"adddata", xrAddData},
    {"clear", xrClear},
    {"end", xrEnd},
    {"readnext", xrReadNext},
    {"startelem", xrStartElem},
    {"endelem", xrEndElem},
    {"name", xrName},
    {"attr", xrAttr},
    {"elemtext", xrReadElemText},
    {"error", xrError},
    {"__gc", xmlreaderGC},
    {nullptr, nullptr}
};
}
ScriptBase::ScriptBase() : L(nullptr), settingsUpdated(false)
{
    L = luaL_newstate();
    if(L)
    {
        luaL_openlibs(L);
        registerFuncs("kiko", kikoFuncs);

        luaL_newmetatable(L, "meta.kiko.xmlreader");
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2); // pushes the metatable
        lua_rawset(L, -3); // metatable.__index = metatable
        luaL_setfuncs(L, xmlreaderFuncs, 0);
        lua_pop(L, 1);

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
            QMap<QString, QVariant> settingMap;
            for(const auto &item : scriptSettings)
                settingMap[item.key] = item.value;
            QJsonDocument doc(QJsonObject::fromVariantMap(settingMap));
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
                call(luaSetOptionFunc, {key, value}, 0, errInfo);
            }
            break;
        }
    }
    return errInfo;
}

ScriptState ScriptBase::loadScript(const QString &path)
{
    if(!L) return "Script Error: Wrong Lua State";
    QFile luaFile(path);
    luaFile.open(QFile::ReadOnly);
    if(!luaFile.isOpen()) return "Open Script File Failed";
    QString luaScript(luaFile.readAll());
    QString errInfo;
    if(luaL_loadstring(L,luaScript.toStdString().c_str()) || lua_pcall(L,0,0,0))
    {
        errInfo="Script Error: "+ QString(lua_tostring(L, -1));
        lua_pop(L,1);
        return errInfo;
    }
    errInfo = loadMeta(path);
    if(!errInfo.isEmpty()) return errInfo;
    loadSettings(path);
    return errInfo;
}

void ScriptBase::init()
{
    if(lua_getglobal(L, luaInitFunc) != LUA_TFUNCTION) return;
    lua_pop(L, 1);
    QString errInfo;
    call(luaInitFunc, {}, 0, errInfo);
}

QVariantList ScriptBase::call(const char *fname, const QVariantList &params, int nRet, QString &errInfo)
{
    if(!L)
    {
        errInfo = "Wrong Lua State";
        return QVariantList();
    }
    if(lua_getglobal(L, fname) != LUA_TFUNCTION)
    {
        errInfo = QString("%1 is not founded").arg(fname);
        return QVariantList();
    }
    for(auto &p : params)
    {
        pushValue(L, p);
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

QVariant ScriptBase::getValue(lua_State *L)
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
                map[lua_tostring(L, -2)] = getValue(L);
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
                list.append(getValue(L));
                lua_pop(L, 1); // -
            }
            return list;
        }
    }
    default:
        return QVariant();
    }
}

size_t ScriptBase::getTableLength(lua_State *L, int pos)
{
    if(!L) return 0;
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
    return errInfo;
}

void ScriptBase::loadSettings(const QString &scriptPath)
{
    //settings = {key = {title = 'xx', default='', desc='', choices=',..,'},...}
    QString errInfo;
    QVariant settings = get(luaSettingsTable);
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

    lua_newtable(L);
    for(const auto &item: scriptSettings)
    {
        lua_pushstring(L, item.key.toStdString().c_str());
        lua_pushstring(L, item.value.toStdString().c_str());
        lua_settable(L, -3);
    }
    lua_setglobal(L, luaSettingsTable);
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

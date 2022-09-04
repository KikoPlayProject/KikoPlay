#include "lua_util.h"
#include "../scriptbase.h"
#include "../luatablemodel.h"
#include "Common/network.h"
#include "Common/notifier.h"
#include "Common/logger.h"
#include <QSysInfo>
#include "globalobjects.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace LuaModule
{

void LuaUtil::setup()
{
    const luaL_Reg commonFuncs[] = {
        {"compress", compress},
        {"decompress", decompress},
        {"writesetting", writeSetting},
        {"execute", execute},
        {"hashdata", hashData},
        {"base64", base64},
        {"log", logger},
        {"message", message},
        {"dialog", dialog},
        {"sttrans", simplifiedTraditionalTrans},
        {"envinfo", envInfo},
        {"viewtable", viewTable},
        {nullptr, nullptr}
    };
    registerFuncs("kiko", commonFuncs);
}


int LuaUtil::compress(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || params > 2 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "decompress: param error, expect: content(string), <type(gzip)>");
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

int LuaUtil::decompress(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || params > 2 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "decompress: param error, expect: content(string), <type(inflate(default)/gzip)>");
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
    const char *data = lua_tolstring(L, 1, &dataLength);
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

int LuaUtil::writeSetting(lua_State *L)
{
    int params = lua_gettop(L);  //key value
    if(params!=2 || lua_type(L, 1)!=LUA_TSTRING || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushstring(L, "writesetting: param error, expect: key(string), value(string)");
        lua_pushnil(L);
        return 2;
    }
    ScriptBase *script = ScriptBase::getScript(L);
    QString errInfo = script->setOption(lua_tostring(L, 1), lua_tostring(L, 2), false);
    if(errInfo.isEmpty()) lua_pushnil(L);
    else lua_pushstring(L, errInfo.toStdString().c_str());
    lua_pushnil(L);
    return 2;
}

int LuaUtil::execute(lua_State *L)
{
    int params = lua_gettop(L);  // detached(bool) program(string) args(array)
    if(params!=3 || lua_type(L, 1)!=LUA_TBOOLEAN || lua_type(L, 2)!=LUA_TSTRING || lua_type(L, 3)!=LUA_TTABLE)
    {
        lua_pushstring(L, "execute: param error, expect: detached(bool), program(string), args(array)");
        lua_pushnil(L);
        return 2;
    }
    bool detached = lua_toboolean(L, 1);
    QString program = lua_tostring(L, 2);
    QStringList args = ScriptBase::getValue(L).toStringList();
    if(detached)
    {
        bool ret = QProcess::startDetached(program, args);
        lua_pushnil(L);
        lua_pushboolean(L, ret);
    }
    else
    {
        int ret = QProcess::execute(program, args);
        lua_pushnil(L);
        lua_pushinteger(L, ret);
    }
    return 2;
}

int LuaUtil::hashData(lua_State *L)
{
    int params = lua_gettop(L);  // path(string) <filesize(bytes,number)>
    if(params==0 || params>4 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "md5file: param error, expect: path/data(string), <ispath(boolean,default=true)>, <filesize(number)>, <algorithm(default=md5)>");
        lua_pushnil(L);
        return 2;
    }
    size_t len = 0;
    const char *pd = lua_tolstring(L, 1, &len);
    bool isPath = true;
    if(params > 1)  //has isdata
    {
        isPath = lua_toboolean(L, 2);
    }
    qint64 size = 0;
    if(params > 2)  //has filesize
    {
        size = lua_tonumber(L, 3);
    }
    static QHash<QString, QCryptographicHash::Algorithm> algorithms({
        {"md4", QCryptographicHash::Md4},
        {"md5", QCryptographicHash::Md5},
        {"sha1", QCryptographicHash::Sha1},
        {"sha224", QCryptographicHash::Sha224},
        {"sha256", QCryptographicHash::Sha256},
        {"sha384", QCryptographicHash::Sha384},
        {"sha512", QCryptographicHash::Sha512}
    });
    QCryptographicHash::Algorithm algo = QCryptographicHash::Md5;
    if(params > 3)  //has algorithm
    {
        algo = algorithms.value(QString(lua_tostring(L, 4)).toLower(), QCryptographicHash::Md5);
    }
    QByteArray hashResult;
    if(isPath)
    {
        QFile file(pd);
        if(!file.open(QIODevice::ReadOnly))
        {
            lua_pushstring(L, QString("md5file: open '%1' failed").arg(pd).toStdString().c_str());
            lua_pushnil(L);
            return 2;
        }
        QByteArray fileData(size==0?file.readAll():file.read(size));
        hashResult = QCryptographicHash::hash(fileData, algo).toHex();

    }
    else
    {
        QByteArray hashInput(pd, len);
        hashResult = QCryptographicHash::hash(hashInput, algo).toHex();
    }
    lua_pushnil(L);
    lua_pushlstring(L, hashResult.data(), hashResult.size());
    return 2;
}

int LuaUtil::base64(lua_State *L)
{
    int params = lua_gettop(L);  // string, <type=from/to>
    if(params==0 || params>2 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "base64: param error, expect: data(string), <type(string, from/to, default=from)>");
        lua_pushnil(L);
        return 2;
    }
    size_t len = 0;
    const char *pd = lua_tolstring(L, 1, &len);
    bool from = true;
    if(params > 1)  //has type
    {
        const char *type = lua_tostring(L, 2);
        from = strcmp(type, "from") == 0;
    }
    QByteArray input(pd, len);
    QByteArray result = from? QByteArray::fromBase64(input) : input.toBase64();
    lua_pushnil(L);
    lua_pushlstring(L, result.data(), result.size());
    return 2;
}

int LuaUtil::logger(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0) return 0;
    ScriptBase *script = ScriptBase::getScript(L);
    if(params > 1)
    {
        QStringList vals;
        for(int i=1;i<=params;++i)
        {
            if(lua_type(L, i) == LUA_TTABLE)
            {
                vals.append("[Table]");
            }
            else
            {
                lua_pushvalue(L, i);
                vals.append(ScriptBase::getValue(L).toString());
                lua_pop(L, 1);
            }
        }
        logInfo(vals.join('\t'), script->id());
        return 0;
    }
    lua_pushvalue(L, 1);
    auto val = ScriptBase::getValue(L);
    lua_pop(L, 1);
    QString logText;
    if(val.type()==QVariant::List || val.type()==QVariant::Map)
    {
        QString json(QJsonDocument::fromVariant(val).toJson(QJsonDocument::JsonFormat::Indented));
        logInfo("Show Table: ", script->id());
        Logger::logger()->log(Logger::Script, json);
    }
    else
    {
        logInfo(val.toString(), script->id());
    }
    return 0;
}

int LuaUtil::message(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || lua_type(L, 1)!=LUA_TSTRING) return 0;
    ScriptBase *script = ScriptBase::getScript(L);
    QString message(lua_tostring(L, 1));
    int flags = NotifyMessageFlag::NM_HIDE;
    if(params > 1 && lua_type(L, 2)==LUA_TNUMBER)
    {
        flags = lua_tonumber(L, 2);
    }
    switch (script->type())
    {
    case ScriptType::LIBRARY:
        Notifier::getNotifier()->showMessage(Notifier::NotifyType::LIBRARY_NOTIFY, message, flags);
        break;
    default:
        break;
    }
    return 0;
}

int LuaUtil::dialog(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || lua_type(L, 1)!=LUA_TTABLE) return 0;
    QVariant table = ScriptBase::getValue(L, false);
    if(table.type()==QVariant::Map)
    {
        QVariantMap map = table.toMap();
        QVariantMap inputs;
        if(map.contains("title"))
            inputs["title"] = QString(map["title"].toByteArray());
        if(map.contains("tip"))
            inputs["tip"] = QString(map["tip"].toByteArray());
        if(map.contains("text"))
            inputs["text"] = QString(map["text"].toByteArray());
        if(map.contains("image"))
            inputs["image"] = map["image"].toByteArray();
        QStringList rets = Notifier::getNotifier()->showDialog(Notifier::MAIN_DIALOG_NOTIFY, inputs).toStringList();
        lua_pushstring(L, rets[0].toStdString().c_str()); // accept/reject
        lua_pushstring(L, rets[1].toStdString().c_str());  //text
        return 2;
    }
    return 0;
}

#ifdef Q_OS_WIN
const QString stTrans(const QString &str, bool toSimplified)
{
    WORD wLanguageID = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
    LCID Locale = MAKELCID(wLanguageID, SORT_CHINESE_PRCP);
    QScopedPointer<QChar> buf(new QChar[str.length()]);
    LCMapString(Locale,toSimplified?LCMAP_SIMPLIFIED_CHINESE:LCMAP_TRADITIONAL_CHINESE,reinterpret_cast<LPCWSTR>(str.constData()),str.length(),reinterpret_cast<LPWSTR>(buf.data()),str.length());
    return QString(buf.data(), str.length());
}
#endif
int LuaUtil::simplifiedTraditionalTrans(lua_State *L)
{
#ifdef Q_OS_WIN
    int params = lua_gettop(L);  // str(string) toSimpOrTrad(bool)
    if(params!=2 ||  lua_type(L, 1)!=LUA_TSTRING || lua_type(L, 2)!=LUA_TBOOLEAN)
    {
        lua_pushstring(L, "stTrans: param error, expect: str(string) toSimpOrTrad(bool, true: toSimp)");
        lua_pushnil(L);
        return 2;
    }
    QString input(lua_tostring(L, 1));
    QString trans(stTrans(input, lua_toboolean(L, 2)));
    lua_pushnil(L);
    lua_pushstring(L, trans.toStdString().c_str());
    return 2;
#else
    lua_pushnil(L);
    lua_pushvalue(L, 1);
    return 2;
#endif
}

int LuaUtil::envInfo(lua_State *L)
{
    lua_newtable(L); // table

    lua_pushstring(L, "os"); // table key
    lua_pushstring(L, QSysInfo::productType().toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "os_version"); // table key
    lua_pushstring(L, QSysInfo::productVersion().toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "kikoplay"); // table key
    lua_pushstring(L, GlobalObjects::kikoVersion); // tabel key value
    lua_rawset(L, -3); //table

    return 1;
}

void buildLuaItemTree(lua_State *L, LuaItem *parent, QSet<quintptr> &tableHash)
{
    if(!L) return;
    if(lua_gettop(L)==0) return;
    if(lua_type(L, -1) != LUA_TTABLE) return;
    lua_pushnil(L); // t nil
    auto getLuaValue = [](lua_State *L, int pos) -> QString {
        switch (lua_type(L, pos))
        {
        case LUA_TNIL:
            return "nil";
        case LUA_TNUMBER:
        {
            double d = lua_tonumber(L, pos);
            return QString::number(d);
        }
        case LUA_TBOOLEAN:
        {
            return bool(lua_toboolean(L, pos))? "true" : "false";
        }
        case LUA_TSTRING:
        {
            size_t len = 0;
            const char *s = (const char *)lua_tolstring(L, pos, &len);
            return QByteArray(s, len);
        }
        default:
            return "";
        }
    };
    while (lua_next(L, -2)) // t key value
    {
        LuaItem *item = new LuaItem(parent);
        item->keyType = static_cast<LuaItem::LuaType>(lua_type(L, -2));
        item->key = getLuaValue(L, -2);
        item->valType = static_cast<LuaItem::LuaType>(lua_type(L, -1));
        if(lua_type(L, -1) == LUA_TTABLE)
        {
            quintptr tablePtr = reinterpret_cast<quintptr>(lua_topointer(L, -1));
            if(!tableHash.contains(tablePtr))
            {
				tableHash.insert(tablePtr);
                buildLuaItemTree(L, item, tableHash);
            }
        }
        else
        {
            item->value = getLuaValue(L, -1);
        }
        lua_pop(L, 1); // key
    }
}
int LuaUtil::viewTable(lua_State *L)
{
    int params = lua_gettop(L);
    if(params==0 || lua_type(L, 1)!=LUA_TTABLE) return 0;
    LuaItem *root = new LuaItem;
    QSet<quintptr> tableHash;
    buildLuaItemTree(L, root, tableHash);
    LuaTableModel model;
    model.setRoot(root);
    QVariantMap dialogParam = {
        {"_type", "luaTabelViewer"},
        {"_modelPtr", QVariant::fromValue<void *>(&model)}
    };
    Notifier::getNotifier()->showDialog(Notifier::MAIN_DIALOG_NOTIFY, dialogParam);
    return 0;
}

}

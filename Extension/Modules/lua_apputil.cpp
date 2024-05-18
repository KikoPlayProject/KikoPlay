#include "lua_apputil.h"
#include <QSysInfo>
#include <QCryptographicHash>
#include <QHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include "Extension/Common/ext_common.h"
#include "Extension/App/kapp.h"
#include "Extension/Common/luatablemodel.h"
#include "Extension/Script/scriptbase.h"
#include "Extension/Script/scriptmanager.h"
#include "Common/logger.h"
#include "Common/notifier.h"
#include "UI/luatableviewer.h"
#include "lua_util.h"
#include "globalobjects.h"

namespace Extension
{

void AppUtil::setup()
{
    const luaL_Reg commonFuncs[] = {
        {"log", logger},
        {"json2table", json2table},
        {"table2json", table2json},
        {"flash", flash},
        {"gtip", gtip},
        {"viewtable", viewTable},
        {"allscripts", LuaUtil::allScripts},
        {"refreshscripts", refreshscripts},
        {"compress", LuaUtil::compress},
        {"decompress", LuaUtil::decompress},
        {"execute", LuaUtil::execute},
        {"hashdata", LuaUtil::hashData},
        {"base64", LuaUtil::base64},
        {"sttrans", LuaUtil::simplifiedTraditionalTrans},
        {nullptr, nullptr}
    };
    registerFuncs("kiko", commonFuncs);
    addDataMembers({"kiko", "msg"}, {
        { "NM_HIDE", NotifyMessageFlag::NM_HIDE },
        { "NM_PROCESS", NotifyMessageFlag::NM_PROCESS },
        { "NM_SHOWCANCEL", NotifyMessageFlag::NM_SHOWCANCEL },
        { "NM_ERROR", NotifyMessageFlag::NM_ERROR },
        { "NM_DARKNESS_BACK", NotifyMessageFlag::NM_DARKNESS_BACK },
    });
    addDataMembers({"kiko", "launch_scene"}, {
        { "APP_MENU", KApp::LaunchScene::LaunchScene_AppMenu },
        { "AUTO_START", KApp::LaunchScene::LaunchScene_AutoStart },
    });
}


int AppUtil::logger(lua_State *L)
{
    int params = lua_gettop(L);
    if (params==0) return 0;
    KApp *app = KApp::getApp(L);
    if (params > 1)
    {
        QStringList vals;
        for (int i = 1; i <= params; ++i)
        {
            if (lua_type(L, i) == LUA_TTABLE)
            {
                vals.append("[Table]");
            }
            else
            {
                lua_pushvalue(L, i);
                vals.append(getValue(L, true).toString());
                lua_pop(L, 1);
            }
        }
        Logger::logger()->log(Logger::Extension, QString("[%1]%2").arg(app->id(), vals.join('\t')));
        return 0;
    }
    lua_pushvalue(L, 1);
    auto val = getValue(L, true);
    lua_pop(L, 1);
    if (val.type()==QVariant::List || val.type()==QVariant::Map)
    {
        QString json(QJsonDocument::fromVariant(val).toJson(QJsonDocument::JsonFormat::Indented));
        Logger::logger()->log(Logger::Extension, QString("[%1]%2").arg(app->id(), "Show Table: \n" + json));
    }
    else
    {
        Logger::logger()->log(Logger::Extension, QString("[%1]%2").arg(app->id(), val.toString()));
    }
    return 0;
}

int AppUtil::json2table(lua_State *L)
{
    if (lua_gettop(L) != 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        lua_pushstring(L, "json2table: param error, expect: jsonstr(string)");
        lua_pushnil(L);
        return 2;
    }
    size_t dataLength = 0;
    const char *data = lua_tolstring(L, 1, &dataLength);
    const QByteArray cdata(data, dataLength);
    QJsonParseError jsonError;
    QJsonDocument jdoc = QJsonDocument::fromJson(cdata, &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        lua_pushstring(L, "Decode JSON Failed");
        lua_pushnil(L);
        return 2;
    }
    lua_pushnil(L);
    if (jdoc.isArray())
    {
        pushValue(L, jdoc.array().toVariantList());
    }
    else if(jdoc.isObject())
    {
        pushValue(L, jdoc.object().toVariantMap());
    }
    else
    {
        lua_newtable(L);
    }
    return 2;
}

int AppUtil::table2json(lua_State *L)
{
    int params = lua_gettop(L);
    if(params < 1 || lua_type(L, 1) != LUA_TTABLE)
    {
        lua_pushstring(L, "table2json: param error, expect: table, <format(indented/compact, default=indented)>");
        lua_pushnil(L);
        return 2;
    }
    lua_pushvalue(L, 1);
    QVariant table = getValue(L, true);
    lua_pop(L, 1);
    QJsonDocument::JsonFormat format = QJsonDocument::JsonFormat::Indented;
    if(params > 1)
    {
        const char *type = lua_tostring(L, 2);
        if(strcmp(type, "compact") == 0)
        {
            format = QJsonDocument::JsonFormat::Compact;
        }
    }
    QByteArray json = QJsonDocument::fromVariant(table).toJson(format);
    lua_pushnil(L);
    lua_pushstring(L, json.constData());
    return 2;
}

int AppUtil::flash(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app) return 0;
    emit app->appFlash(app);
    return 0;
}

int AppUtil::viewTable(lua_State *L)
{
    const int params = lua_gettop(L);
    KApp *app = KApp::getApp(L);
    if (!app || !app->window() || params == 0 || lua_type(L, 1) != LUA_TTABLE) return 0;
    LuaItem *root = new LuaItem;
    QSet<quintptr> tableHash;
    buildLuaItemTree(L, root, tableHash);
    LuaTableModel model;
    model.setRoot(root);
    LuaTableViewer viewer(&model, app->window()->getWidget());
    viewer.exec();
    return 0;
}

int AppUtil::gtip(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app) return 0;
    // gtip(string)
    // gtip(param_table)
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        TipParams param;
        param.message = lua_tostring(L, 1);
        param.title = app->name();
        Notifier::getNotifier()->showMessage(Notifier::NotifyType::GLOBAL_NOTIFY, "", 0, QVariant::fromValue(param));
    }
    else if (lua_type(L, 1) == LUA_TTABLE)
    {
        QVariantMap paramsMap = getValue(L, true, 2).toMap();
        TipParams param;
        param.title = app->name();
        if (paramsMap.contains("title")) param.title = paramsMap["title"].toString();
        param.group = paramsMap["group"].toString();
        param.message = paramsMap["message"].toString();
        if (paramsMap.contains("timeout")) param.timeout = paramsMap["timeout"].toInt();
        if (paramsMap.contains("bg")) param.bgColor.setRgba(paramsMap["bg"].toUInt());
        if (paramsMap.contains("showclose")) param.showClose = paramsMap["showclose"].toBool();
        Notifier::getNotifier()->showMessage(Notifier::NotifyType::GLOBAL_NOTIFY, "", 0, QVariant::fromValue(param));
    }
    return 0;
}

int AppUtil::refreshscripts(lua_State *L)
{
    ScriptType stype = ScriptType::UNKNOWN_STYPE;
    if (lua_gettop(L) > 0 && lua_type(L, 1) == LUA_TNUMBER)
    {
        const int t = lua_tointeger(L, 1);
        if ( t >= 0 && t < ScriptType::UNKNOWN_STYPE)
        {
            stype = ScriptType(t);
        }
    }
    QMetaObject::invokeMethod(GlobalObjects::scriptManager, [stype](){
        if (stype == ScriptType::UNKNOWN_STYPE)
        {
            for(int i = ScriptType::DANMU; i < ScriptType::UNKNOWN_STYPE; ++i)
            {
                GlobalObjects::scriptManager->refreshScripts(ScriptType(i));
            }
        }
        else
        {
            GlobalObjects::scriptManager->refreshScripts(ScriptType(stype));
        }
    }, Qt::QueuedConnection);
    return 0;
}



}

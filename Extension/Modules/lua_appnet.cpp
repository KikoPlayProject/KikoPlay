#include "lua_appnet.h"
#include "lua_net.h"
#include "Common/network.h"
#include "Extension/Common/ext_common.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include "Common/logger.h"

namespace Extension
{
const char *RequestData::MetaName = "meta.kiko.reqdata";
const char *RequestData::selfKey = "_req_data";

const char *WebSocketData::MetaName = "meta.kiko.wsdata";
const char *WebSocketData::selfKey = "_ws_data";

void AppNet::setup()
{
    const luaL_Reg reqDataMembers[] = {
        {"status",   RequestData::status},
        {"header",   RequestData::header},
        {"content",  RequestData::content},
        {"read",     RequestData::read},
        {"error",    RequestData::error},
        {"extra",    RequestData::extra},
        {"abort",    RequestData::abort},
        {"finished", RequestData::finished},
        {"running",  RequestData::running},
        {"__gc",     RequestData::reqGC},
        {nullptr, nullptr}
    };
    registerMemberFuncs(RequestData::MetaName, reqDataMembers);

    const luaL_Reg wsDataMembers[] = {
        {"open",     WebSocketData::open},
        {"close",    WebSocketData::close},
        {"ping",     WebSocketData::ping},
        {"send",     WebSocketData::send},
        {"address",  WebSocketData::address},
        {"state",    WebSocketData::state},
        {"error",    WebSocketData::error},
        {"extra",    WebSocketData::extra},
        {"__gc",     WebSocketData::wsGC},
        {nullptr, nullptr}
    };
    registerMemberFuncs(WebSocketData::MetaName, wsDataMembers);
    addDataMembers({"kiko", "net"}, {
        { "WS_UNCONNECTED", QAbstractSocket::UnconnectedState },
        { "WS_HOST_LOOKUP", QAbstractSocket::HostLookupState },
        { "WS_CONNECTING", QAbstractSocket::ConnectingState },
        { "WS_CONNECTED", QAbstractSocket::ConnectedState },
        { "WS_BOUND", QAbstractSocket::BoundState },
        { "WS_CLOSING", QAbstractSocket::ClosingState },
    });

    const luaL_Reg netFuncs[] = {
        {"request", request},
        {"websocket", websocket},
        {"httpget", Net::httpGet},
        {"httpgetbatch", Net::httpGetBatch},
        {"httppost", Net::httpPost},
        {"httphead", Net::httpHead},
        {"dcomment", Net::getDComment},
        {"dsearch", Net::dSearch},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "net"}, netFuncs);
}

int AppNet::request(lua_State *L)
{
    if (lua_gettop(L) != 1 || lua_type(L, -1) != LUA_TTABLE)
    {
        lua_pushnil(L);
        lua_pushstring(L, "request: param error");
        return 2;
    }
    QVariantMap reqInfo = getValue(L, false, 2).toMap();
    if (!reqInfo.contains("url") || !reqInfo.contains("method"))
    {
        lua_pushnil(L);
        lua_pushstring(L, "request: must have url,method");
        return 2;
    }
    QNetworkRequest req;
    buildRequest(reqInfo, req);
    static const QHash<QString, RequestMethod> methodHash = {
        {"get", RequestMethod::GET},
        {"post", RequestMethod::POST},
        {"head", RequestMethod::HEAD},
        {"put", RequestMethod::PUT},
        {"delete", RequestMethod::DELETE},
    };
    RequestMethod method = methodHash.value(reqInfo["method"].toString().toLower(), RequestMethod::UNKNOWN);
    QNetworkAccessManager *manager = Network::getManager();
    manager->setCookieJar(nullptr);
    QNetworkReply *reply = nullptr;
    switch (method)
    {
    case RequestMethod::GET:
        reply = manager->get(req);
        break;
    case RequestMethod::POST:
        reply = manager->post(req, reqInfo.value("data").toByteArray());
        break;
    case RequestMethod::HEAD:
        reply = manager->head(req);
        break;
    case RequestMethod::PUT:
        reply = manager->put(req, reqInfo.value("data").toByteArray());
        break;
    case RequestMethod::DELETE:
        reply = manager->deleteResource(req);
        break;
    default:
        reply = manager->sendCustomRequest(req, reqInfo["method"].toByteArray(), reqInfo.value("data").toByteArray());
        break;
    }

    reply->setParent(nullptr);
    RequestData *dt = new RequestData(reply);
    dt->push(L);

    // Keep the request table as the userdata's uservalue. Unlike a registry
    // reference, this relationship is visible to Lua's garbage collector.
    lua_pushvalue(L, 1);  // table_req, dt, table_req
    lua_setuservalue(L, -2);  // table_req, dt
    lua_pushstring(L, RequestData::selfKey);  // table_req, dt, key
    lua_pushvalue(L, -2);  // table_req, dt, key, dt
    lua_rawset(L, 1);  // table_req, dt
    lua_pushvalue(L, -1);
    dt->selfRef = luaL_ref(L, LUA_REGISTRYINDEX);

    QObject::connect(reply, &QNetworkReply::finished, [dt, L](){
        LuaStackGuard stackGuard(L);
        const char *func = dt->reply->error() == QNetworkReply::NoError? "success" : "error";
        if (lua_checkstack(L, 4) && pushLuaUserdataCallback(L, dt->selfRef, func))
        {
            if (dt->pushFromRef(L) && lua_pcall(L, 1, 0, 0))
            {
                Logger::logger()->log(Logger::Extension, "[request]" + QString(lua_tostring(L, -1)));
            }
        }
        dt->releaseRef(L);
    });
    if (reqInfo.contains("progress"))
    {
        QObject::connect(reply, &QNetworkReply::downloadProgress, [dt, L](qint64 recv, qint64 total){
            LuaStackGuard stackGuard(L);
            if (lua_checkstack(L, 6) && pushLuaUserdataCallback(L, dt->selfRef, "progress"))
            {
                lua_pushinteger(L, recv);
                lua_pushinteger(L, total);
                if (dt->pushFromRef(L) && lua_pcall(L, 3, 0, 0))
                {
                    Logger::logger()->log(Logger::Extension, "[request]progress: " + QString(lua_tostring(L, -1)));
                }
            }
        });
    }
    lua_pushnil(L);
    return 2;
}

int AppNet::websocket(lua_State *L)
{
    if (lua_gettop(L) != 1 || lua_type(L, -1) != LUA_TTABLE)
    {
        lua_pushnil(L);
        lua_pushstring(L, "websocket: param error");
        return 2;
    }
    QVariantMap reqInfo = getValue(L, false, 2).toMap();

    QWebSocket *ws = new QWebSocket;
    WebSocketData *wsData = new WebSocketData(ws);
    wsData->push(L);  // table_param, ws

    lua_pushvalue(L, 1);  // table_parem, ws, table_param
    lua_setuservalue(L, -2);   // table_param, ws
    lua_pushstring(L, WebSocketData::selfKey);  // table_param, ws, key
    lua_pushvalue(L, -2);  // table_param, ws, key, ws
    lua_rawset(L, 1);  // table_param, ws

    if (reqInfo.contains("connected"))
    {
        QObject::connect(ws, &QWebSocket::connected, [wsData, L](){
            LuaStackGuard stackGuard(L);
            if (lua_checkstack(L, 4) && pushLuaUserdataCallback(L, wsData->selfRef, "connected"))
            {
                if (wsData->pushFromRef(L) && lua_pcall(L, 1, 0, 0))
                {
                    Logger::logger()->log(Logger::Extension, "[websocket]connected: " + QString(lua_tostring(L, -1)));
                }
            }
        });
    }
    QObject::connect(ws, &QWebSocket::disconnected, [wsData, L](){
        LuaStackGuard stackGuard(L);
        if (lua_checkstack(L, 4) && pushLuaUserdataCallback(L, wsData->selfRef, "disconnected"))
        {
            if (wsData->pushFromRef(L) && lua_pcall(L, 1, 0, 0))
            {
                Logger::logger()->log(Logger::Extension, "[websocket]disconnected: " + QString(lua_tostring(L, -1)));
            }
        }
        wsData->releaseRef(L);
    });
    if (reqInfo.contains("received"))
    {
        auto func = [wsData, L](bool isText, bool lastFrame, const QByteArray &message){
            LuaStackGuard stackGuard(L);
            if (lua_checkstack(L, 7) && pushLuaUserdataCallback(L, wsData->selfRef, "received"))
            {
                if (wsData->pushFromRef(L))
                {
                    lua_pushlstring(L, message.constData(), message.size());
                    lua_pushboolean(L, isText);
                    lua_pushboolean(L, lastFrame);
                    if (lua_pcall(L, 4, 0, 0))
                    {
                        Logger::logger()->log(Logger::Extension, "[websocket]received: " + QString(lua_tostring(L, -1)));
                    }
                }
            }
        };
        QObject::connect(ws, &QWebSocket::binaryMessageReceived, [=](const QByteArray &message){
            func(false, false, message);
        });
        QObject::connect(ws, &QWebSocket::binaryFrameReceived, [=](const QByteArray &message, bool isLastFrame){
            func(false, isLastFrame, message);
        });
        QObject::connect(ws, &QWebSocket::textMessageReceived, [=](const QString &message){
            func(true, false, message.toUtf8());
        });
        QObject::connect(ws, &QWebSocket::textFrameReceived, [=](const QString &message, bool isLastFrame){
            func(true, isLastFrame, message.toUtf8());
        });
    }
    if (reqInfo.contains("pong"))
    {
        QObject::connect(ws, &QWebSocket::pong, [wsData, L](quint64 elapsedTime, const QByteArray &payload){
            LuaStackGuard stackGuard(L);
            if (lua_checkstack(L, 6) && pushLuaUserdataCallback(L, wsData->selfRef, "pong"))
            {
                if (wsData->pushFromRef(L))
                {
                    lua_pushinteger(L, elapsedTime);
                    lua_pushlstring(L, payload.constData(), payload.size());
                    if (lua_pcall(L, 3, 0, 0))
                    {
                        Logger::logger()->log(Logger::Extension, "[websocket]pong: " + QString(lua_tostring(L, -1)));
                    }
                }
            }
        });
    }
    if (reqInfo.contains("state_changed"))
    {
        QObject::connect(ws, &QWebSocket::stateChanged, [wsData, L](QAbstractSocket::SocketState state){
            LuaStackGuard stackGuard(L);
            if (lua_checkstack(L, 5) && pushLuaUserdataCallback(L, wsData->selfRef, "state_changed"))
            {
                if (wsData->pushFromRef(L))
                {
                    lua_pushinteger(L, static_cast<int>(state));
                    if (lua_pcall(L, 2, 0, 0))
                    {
                        Logger::logger()->log(Logger::Extension, "[websocket]state_changed: " + QString(lua_tostring(L, -1)));
                    }
                }
            }
        });
    }
    lua_pushnil(L);
    return 2;
}

void AppNet::buildRequest(const QVariantMap &reqInfo, QNetworkRequest &req)
{
    QUrl queryUrl{reqInfo["url"].toString()};
    if (reqInfo.contains("query"))
    {
        QUrlQuery query;
        const QVariantMap queryMap = reqInfo["query"].toMap();
        for(auto iter = queryMap.constBegin(); iter != queryMap.constEnd(); ++iter)
        {
            query.addQueryItem(iter.key(),iter.value().toString());
        }
        queryUrl.setQuery(query);
    }
    req.setUrl(queryUrl);
    if (reqInfo.contains("header"))
    {
        const QVariantMap headerMap = reqInfo["header"].toMap();
        for(auto iter = headerMap.constBegin(); iter != headerMap.constEnd(); ++iter)
        {
            req.setRawHeader(iter.key().toUtf8(), iter.value().toByteArray());
        }
    }
    //req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    if (reqInfo.contains("redirect"))
    {
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, reqInfo.value("redirect", true));
        req.setMaximumRedirectsAllowed(reqInfo.value("max_redirect", 10).toInt());
    }
    if (reqInfo.contains("trans_timeout"))
    {
        bool ok = false;
        const int timeout = reqInfo["trans_timeout"].toInt(&ok);
        if (ok && timeout >= 0)
        {
            req.setTransferTimeout(timeout);
        }
    }
}

void RequestData::push(lua_State *L)
{
    RequestData **d = (RequestData **)lua_newuserdata(L, sizeof(RequestData *));
    luaL_getmetatable(L, RequestData::MetaName);
    lua_setmetatable(L, -2);
    *d = this;
}

bool RequestData::pushFromRef(lua_State *L)
{
    if (!isValidLuaRef(selfRef) || !lua_checkstack(L, 1)) return false;
    if (lua_rawgeti(L, LUA_REGISTRYINDEX, selfRef) != LUA_TUSERDATA)
    {
        lua_pop(L, 1);
        return false;
    }
    return true;
}

void RequestData::releaseRef(lua_State *L)
{
    if (!isValidLuaRef(selfRef)) return;
    luaL_unref(L, LUA_REGISTRYINDEX, selfRef);
    selfRef = LUA_NOREF;
}

RequestData *RequestData::checkItem(lua_State *L, int pos)
{
    void *ud = luaL_checkudata(L, pos, RequestData::MetaName);
    luaL_argcheck(L, ud != NULL, pos, "reqdata expected");
    return *(RequestData **)ud;
}

int RequestData::reqGC(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (d)
    {
        d->releaseRef(L);
        *(RequestData **)lua_touserdata(L, 1) = nullptr;
        delete d;
    }
    return 0;
}

int RequestData::status(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d || !d->reply)
    {
        lua_pushnil(L);
        return 1;
    }
    const int statusCode = d->reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    lua_pushinteger(L, statusCode);
    return 1;
}

int RequestData::header(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d || !d->reply)
    {
        lua_pushnil(L);
        return 1;
    }
    const auto &headers = d->reply->rawHeaderPairs();
    lua_newtable(L); // table
    for (auto &p : headers)
    {
        lua_pushlstring(L, p.first.constData(), p.first.size()); // table key
        lua_pushlstring(L, p.second.constData(), p.second.size()); //  table key value
        lua_rawset(L, -3); // table
    }
    return 1;
}

int RequestData::content(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d || !d->reply)
    {
        lua_pushnil(L);
        return 1;
    }
    const QByteArray content = d->reply->readAll();
    lua_pushlstring(L, content.constData(), content.size());
    return 1;
}

int RequestData::read(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d || !d->reply)
    {
        lua_pushnil(L);
        return 1;
    }
    qint64 maxSize = 1024;
    if (lua_gettop(L) > 1 || lua_type(L, 2) == LUA_TNUMBER)
    {
        maxSize = lua_tointeger(L, 2);
    }
    const QByteArray content = d->reply->read(maxSize);
    lua_pushlstring(L, content.constData(), content.size());
    return 1;
}

int RequestData::error(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d || !d->reply)
    {
        lua_pushnil(L);
        return 1;
    }
    const QString err = d->reply->errorString();
    lua_pushstring(L, err.toUtf8().constData());
    return 1;
}

int RequestData::extra(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_getuservalue(L, 1);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_pushnil(L);
        return 1;
    }
    lua_getfield(L, -1, "extra");
    lua_remove(L, -2);
    return 1;
}

int RequestData::abort(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d || !d->reply) return 0;
    d->reply->abort();
    return 0;
}

int RequestData::finished(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d || !d->reply) return 0;
    lua_pushboolean(L, d->reply->isFinished());
    return 1;
}

int RequestData::running(lua_State *L)
{
    RequestData *d = checkItem(L, 1);
    if (!d || !d->reply) return 0;
    lua_pushboolean(L, d->reply->isRunning());
    return 1;
}

void WebSocketData::push(lua_State *L)
{
    WebSocketData **d = (WebSocketData **)lua_newuserdata(L, sizeof(WebSocketData *));
    luaL_getmetatable(L, WebSocketData::MetaName);
    lua_setmetatable(L, -2);
    *d = this;
}

bool WebSocketData::pushFromRef(lua_State *L)
{
    if (!isValidLuaRef(selfRef) || !lua_checkstack(L, 1)) return false;
    if (lua_rawgeti(L, LUA_REGISTRYINDEX, selfRef) != LUA_TUSERDATA)
    {
        lua_pop(L, 1);
        return false;
    }
    return true;
}

void WebSocketData::retain(lua_State *L, int pos)
{
    if (isValidLuaRef(selfRef)) return;
    lua_pushvalue(L, pos);
    selfRef = luaL_ref(L, LUA_REGISTRYINDEX);
}

void WebSocketData::releaseRef(lua_State *L)
{
    if (!isValidLuaRef(selfRef)) return;
    luaL_unref(L, LUA_REGISTRYINDEX, selfRef);
    selfRef = LUA_NOREF;
}

WebSocketData *WebSocketData::checkItem(lua_State *L, int pos)
{
    void *ud = luaL_checkudata(L, pos, WebSocketData::MetaName);
    luaL_argcheck(L, ud != NULL, pos, "reqdata expected");
    return *(WebSocketData **)ud;
}

int WebSocketData::wsGC(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (d)
    {
        d->releaseRef(L);
        *(WebSocketData **)lua_touserdata(L, 1) = nullptr;
        delete d;
    }
    return 0;
}

int WebSocketData::open(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (!d || !d->websocket || lua_gettop(L) != 2) return 0;
    d->retain(L, 1);
    d->websocket->open(QUrl(lua_tostring(L, 2)));
    return 0;
}

int WebSocketData::close(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (!d || !d->websocket) return 0;
    d->websocket->close();
    return 0;
}

int WebSocketData::ping(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (!d || !d->websocket) return 0;
    QByteArray content;
    if (lua_gettop(L) > 1 && lua_type(L, 2) == LUA_TSTRING)
    {
        size_t len = 0;
        const char *c = lua_tolstring(L, 2, &len);
        content.append(c, len);
    }
    d->websocket->ping(content);
    return 0;
}

int WebSocketData::send(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (!d || !d->websocket || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING) return 0;
    size_t len = 0;
    const char *c = lua_tolstring(L, 2, &len);
    QByteArray content(c, len);
    bool isBinary = false;
    if (lua_gettop(L) > 2)
    {
        isBinary = lua_toboolean(L, 3);
    }
    if (isBinary)
    {
        d->websocket->sendBinaryMessage(content);
    }
    else
    {
        d->websocket->sendTextMessage(content);
    }
    return 0;
}

int WebSocketData::address(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (!d || !d->websocket) return 0;
    QVariantMap info = {
        {"peer_addr", d->websocket->peerAddress().toString()},
        {"peer_port", d->websocket->peerPort()},
        {"peer_name", d->websocket->peerName()},
        {"local_addr", d->websocket->localAddress().toString()},
        {"local_port", d->websocket->localPort()},
    };
    pushValue(L, info);
    return 1;
}

int WebSocketData::state(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (!d || !d->websocket) return 0;
    lua_pushinteger(L, static_cast<int>(d->websocket->state()));
    return 1;
}

int WebSocketData::error(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (!d || !d->websocket) return 0;
    const QString err = d->websocket->errorString();
    lua_pushstring(L, err.toUtf8().constData());
    return 1;
}

int WebSocketData::extra(lua_State *L)
{
    WebSocketData *d = checkItem(L, 1);
    if (!d || !d->websocket) return 0;
    lua_getuservalue(L, 1);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_pushnil(L);
        return 1;
    }
    lua_getfield(L, -1, "extra");
    lua_remove(L, -2);
    return 1;
}

}

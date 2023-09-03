#ifndef LUA_APPNET_H
#define LUA_APPNET_H

#include "modulebase.h"
#include <QVariant>
#include <QNetworkReply>
#include <QtWebSockets/QWebSocket>

class QNetworkRequest;
namespace  Extension
{
struct RequestData
{
    RequestData(QNetworkReply *r, int ref) : reply(r), reqInfoRef(ref) {}
    ~RequestData()
    {
        if (reply)
        {
            reply->deleteLater();
            reply = nullptr;
        }
    }
    RequestData &operator=(const RequestData&) = delete;
    RequestData(const RequestData&) = delete;
    QNetworkReply *reply = nullptr;
    int reqInfoRef = 0;

    void push(lua_State *L);
    void pushFromRef(lua_State *L);

    static const char *MetaName;
    static const char *selfKey;

    static RequestData *checkItem(lua_State *L, int pos);
    static int reqGC(lua_State *L);
    static int status(lua_State *L);
    static int header(lua_State *L);
    static int content(lua_State *L);
    static int read(lua_State *L);
    static int error(lua_State *L);
    static int extra(lua_State *L);
    static int abort(lua_State *L);
    static int finished(lua_State *L);
    static int running(lua_State *L);
};

struct WebSocketData
{
    WebSocketData(QWebSocket *s, int ref) : websocket(s), reqInfoRef(ref) {}
    ~WebSocketData()
    {
        if (websocket)
        {
            websocket->deleteLater();
            websocket = nullptr;
        }
    }
    WebSocketData &operator=(const WebSocketData&) = delete;
    WebSocketData(const WebSocketData&) = delete;

    QWebSocket *websocket = nullptr;
    int reqInfoRef = 0;

    void push(lua_State *L);
    void pushFromRef(lua_State *L);

    static const char *MetaName;
    static const char *selfKey;

    static WebSocketData *checkItem(lua_State *L, int pos);
    static int wsGC(lua_State *L);
    static int open(lua_State *L);
    static int close(lua_State *L);
    static int ping(lua_State *L);
    static int send(lua_State *L);
    static int address(lua_State *L);
    static int state(lua_State *L);
    static int error(lua_State *L);
    static int extra(lua_State *L);
};

class AppNet : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int request(lua_State *L);
    static int websocket(lua_State *L);
private:
    enum class RequestMethod
    {
        GET, POST, HEAD, PUT, DELETE, UNKNOWN
    };
    static void buildRequest(const QVariantMap &reqInfo, QNetworkRequest &req);
};

}
#endif // LUA_APPNET_H

#include "lua_net.h"
#include "Extension/Common/ext_common.h"

namespace Extension
{

void Net::setup()
{
    const luaL_Reg netFuncs[] = {
        {"httpget", httpGet},
        {"httpgetbatch", httpGetBatch},
        {"httppost", httpPost},
        {"httphead", httpHead},
        {"json2table", json2table},
        {"table2json", table2json},
        {nullptr, nullptr}
    };
    registerFuncs("kiko", netFuncs);
}

void Net::pushNetworkReply(lua_State *L, const Network::Reply &reply)
{
    lua_newtable(L); // table

    lua_pushstring(L, "statusCode"); // table key
    lua_pushnumber(L, reply.statusCode); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "hasError"); // table key
    lua_pushboolean(L, reply.hasError); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "errInfo"); // table key
    lua_pushstring(L, reply.errInfo.toStdString().c_str()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "content"); // table key
    lua_pushlstring(L, reply.content.constData(), reply.content.size()); // tabel key value
    lua_rawset(L, -3); //table

    lua_pushstring(L, "headers"); // table key
    lua_newtable(L); // table key table
    for(auto &p : reply.headers)
    {
        lua_pushlstring(L, p.first.constData(), p.first.size()); // table key table key
        lua_pushlstring(L, p.second.constData(), p.second.size()); // table key table key value
        lua_rawset(L, -3); //table key table
    }
    lua_rawset(L, -3); //table
}

void Net::setRequestOptions(const QVariantMap &options, QStringList &headers)
{
    if (options.value("set_dandan_header", false).toBool() && options.contains("dandan_path"))
    {
        const QString &ddPath = options.value("dandan_path", "").toString();
        qint64 ts = QDateTime::currentSecsSinceEpoch();
        QString secret = QString("%1%2%3%4").arg(Network::kDanDanAppId).arg(ts).arg(ddPath).arg(Network::kDanDanAppSecret);
        QByteArray hash = QCryptographicHash::hash(secret.toUtf8(), QCryptographicHash::Sha256);
        headers << "X-AppId" << Network::kDanDanAppId;
        headers << "X-Timestamp" << QString::number(ts);
        headers << "X-Signature" << hash.toBase64();
    }
}

int Net::httpGet(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //url <query> <header> <options>
        if(params==0 || params>4) break;
        if(lua_type(L, 1)!=LUA_TSTRING) break;
        const char *curl = luaL_checkstring(L,1);
        QUrlQuery query;
        QStringList headers;
        bool redirect = true;
        if(params > 1)  //has query
        {
            lua_pushvalue(L, 2);
            auto q = getValue(L, false);
            lua_pop(L, 1);
            if (q.type() != QVariant::Map && !(q.type() == QVariant::List && q.toList().size() == 0)) break;
            auto qmap = q.toMap();
            for(auto iter=qmap.constBegin(); iter!=qmap.constEnd(); ++iter)
            {
                query.addQueryItem(iter.key(),iter.value().toString());
            }
        }
        if(params > 2)  //has header
        {
            lua_pushvalue(L, 3);
            auto h = getValue(L, false);
            lua_pop(L, 1);
            if (h.type() != QVariant::Map && !(h.type() == QVariant::List && h.toList().size() == 0)) break;
            auto hmap = h.toMap();
            for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
            {
                headers<<iter.key()<<iter.value().toString();
            }
        }
        if (params > 3)
        {
            QVariantMap options = getValue(L, false).toMap();
            redirect = options.value("redirect", true).toBool();
            setRequestOptions(options, headers);
        }
        Network::Reply &&reply = Network::httpGet(curl,query,headers,redirect);
        if(!reply.hasError)
        {
            lua_pushnil(L);
            pushNetworkReply(L, reply);
        }
        else
        {
            lua_pushstring(L, reply.errInfo.toStdString().c_str());
            lua_pushnil(L);
        }
        return 2;
    }while(false);
    lua_pushstring(L, "httpget: param error, expect: url(string), <query(table)>, <header(table)>, <options(table)>");
    lua_pushnil(L);
    return 2;
}

int Net::httpHead(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //url <query> <header> <options>
        if(params==0 || params>4) break;
        if(lua_type(L, 1)!=LUA_TSTRING) break;
        const char *curl = luaL_checkstring(L,1);
        QUrlQuery query;
        QStringList headers;
        bool redirect = true;
        if(params > 1)  //has query
        {
            lua_pushvalue(L, 2);
            auto q = getValue(L, false);
            lua_pop(L, 1);
            if (q.type() != QVariant::Map && !(q.type() == QVariant::List && q.toList().size() == 0)) break;
            auto qmap = q.toMap();
            for(auto iter=qmap.constBegin(); iter!=qmap.constEnd(); ++iter)
            {
                query.addQueryItem(iter.key(),iter.value().toString());
            }
        }
        if(params > 2)  //has header
        {
            lua_pushvalue(L, 3);
            auto h = getValue(L, false);
            lua_pop(L, 1);
            if (h.type() != QVariant::Map && !(h.type() == QVariant::List && h.toList().size() == 0)) break;
            auto hmap = h.toMap();
            for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
            {
                headers<<iter.key()<<iter.value().toString();
            }
        }
        if(params > 3)
        {
            QVariantMap options = getValue(L, false).toMap();
            redirect = options.value("redirect", true).toBool();
            setRequestOptions(options, headers);
        }
        Network::Reply &&reply = Network::httpHead(curl,query,headers,redirect);
        if(!reply.hasError)
        {
            lua_pushnil(L);
            pushNetworkReply(L, reply);
        }
        else
        {
            lua_pushstring(L, reply.errInfo.toStdString().c_str());
            lua_pushnil(L);
        }
        return 2;
    }while(false);
    lua_pushstring(L, "httphead: param error, expect: url(string), <query(table)>, <header(table)>, <options(table)>");
    lua_pushnil(L);
    return 2;
}

int Net::httpPost(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //url <data> <header> <query> <options>
        if(params<2 || params>5) break;
        if(lua_type(L, 1)!=LUA_TSTRING) break;
        if(lua_type(L, 2)!=LUA_TSTRING) break;
        const char *curl = luaL_checkstring(L,1);
        size_t dataLength = 0;
        const char *data = lua_tolstring(L, 2, &dataLength);
        QByteArray cdata(data, dataLength);
        QStringList headers;
        if(params > 2)  //has header
        {
            lua_pushvalue(L, 3);
            auto h = getValue(L, false);
            lua_pop(L, 1);
            if (h.type() != QVariant::Map && !(h.type() == QVariant::List && h.toList().size() == 0)) break;
            auto hmap = h.toMap();
            for(auto iter=hmap.constBegin(); iter!=hmap.constEnd(); ++iter)
            {
                headers<<iter.key()<<iter.value().toString();
            }
        }
        QUrlQuery query;
        if(params > 3)  //has query
        {
            lua_pushvalue(L, 4);
            auto q = getValue(L, false);
            lua_pop(L, 1);
            if (q.type() != QVariant::Map && !(q.type() == QVariant::List && q.toList().size() == 0)) break;
            auto qmap = q.toMap();
            for(auto iter=qmap.constBegin(); iter!=qmap.constEnd(); ++iter)
            {
                query.addQueryItem(iter.key(), iter.value().toString());
            }
        }
        if (params > 4)  // has options
        {
            QVariantMap options = getValue(L, false).toMap();
            setRequestOptions(options, headers);
        }
        Network::Reply &&reply = Network::httpPost(curl, cdata, headers, query);
        if(!reply.hasError)
        {
            lua_pushnil(L);
            pushNetworkReply(L, reply);
        }
        else
        {
            lua_pushstring(L, reply.errInfo.toStdString().c_str());
            lua_pushnil(L);
        }
        return 2;
    }while(false);
    lua_pushstring(L, "httppost: param error, expect: url(string), <data(string)>, <header(table)>, <query(table)>, <options(table)>");
    lua_pushnil(L);
    return 2;
}

int Net::httpGetBatch(lua_State *L)
{
    do
    {
        int params = lua_gettop(L);  //urls([u1,u2,...]) <querys([{xx=xx,...},{xx=xx,...},...])> <headers([{xx=xx,..},{xx=xx,..},...])>, <options>
        if(params==0 || params>3) break;
        if(lua_type(L, 1)!=LUA_TTABLE) break;
        lua_pushvalue(L, 1);
        auto us = getValue(L, false);
        lua_pop(L, 1);
        if(!us.canConvert(QVariant::StringList)) break;
        auto urls = us.toStringList();
        QList<QUrlQuery> querys;
        QList<QStringList> headers;
        bool redirect = true;
        if(params > 1)  //has query
        {
            lua_pushvalue(L, 2);
            auto q = getValue(L, false);
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
            lua_pushvalue(L, 3);
            auto h = getValue(L, false);
            lua_pop(L, 1);
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
                headers.append(header);
            }
        }
        if(params > 3)
        {
            QVariantMap options = getValue(L, false).toMap();
            redirect = options.value("redirect", true).toBool();
            QStringList optHeaders;
            setRequestOptions(options, optHeaders);
            for (auto &h : headers)
            {
                h.append(optHeaders);
            }
        }
        QList<Network::Reply> &&content = Network::httpGetBatch(urls, querys, headers, redirect); //[[hasError, content], [], ...]
        lua_pushnil(L);
        lua_newtable(L); // table
        for(int i=0; i<content.size(); ++i)
        {
            pushNetworkReply(L, content[i]); //table reply
            lua_rawseti(L, -2, i+1); //table
        }
        return 2;
    }while(false);
    lua_pushstring(L, "httpgetbatch: param error, expect: urls(string array), <querys(table array)>, <headers(table array)>, <options>");
    lua_pushnil(L);
    return 2;
}

int Net::json2table(lua_State *L)
{
    int params = lua_gettop(L);  //jsonstr
    if(params!=1 || lua_type(L, 1)!=LUA_TSTRING)
    {
        lua_pushstring(L, "json2table: param error, expect: jsonstr(string)");
        lua_pushnil(L);
        return 2;
    }
    size_t dataLength = 0;
    const char *data = lua_tolstring(L, 1, &dataLength);
    QByteArray cdata(data, dataLength);
    try {
        auto jdoc = Network::toJson(cdata);
        lua_pushnil(L);
        if(jdoc.isArray())
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
    } catch (Network::NetworkError &err) {
        lua_pushstring(L,err.errorInfo.toStdString().c_str());
        lua_pushnil(L);
        return 2;
    }
}

int Net::table2json(lua_State *L)
{
    int params = lua_gettop(L);
    if(params<1 || lua_type(L, 1)!=LUA_TTABLE)
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


}

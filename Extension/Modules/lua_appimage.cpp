#include "lua_appimage.h"
#include "Extension/Common/ext_common.h"
#include <QImage>
#include <QBuffer>
#include <QImageReader>

namespace  Extension
{
const char *AppImage::MetaName = "meta.kiko.img";

void AppImage::setup()
{
    const luaL_Reg imgMembers[] = {
        {"size",  size},
        {"save",  save},
        {"tobytes",  tobytes},
        {"scale", scale},
        {"__gc",  imgGC},
        {nullptr, nullptr}
    };
    registerMemberFuncs(MetaName, imgMembers);
    const luaL_Reg funcs[] = {
        {"createimg", createimg},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "ui"}, funcs);
}

QImage *AppImage::checkImg(lua_State *L, int pos)
{
    void *ud = luaL_checkudata(L, pos, AppImage::MetaName);
    if (!ud) return nullptr;
    return *(QImage **)ud;
}

void AppImage::pushImg(lua_State *L, QImage *img)
{
    QImage **d = (QImage **)lua_newuserdata(L, sizeof(QImage *));
    luaL_getmetatable(L, AppImage::MetaName);
    lua_setmetatable(L, -2);
    *d = img;
}

int AppImage::createimg(lua_State *L)
{
    if (lua_gettop(L) < 1)
    {
        lua_pushnil(L);
        return 1;
    }
    const int type = lua_type(L, 1);
    if (type == LUA_TSTRING)
    {
        const QString path = lua_tostring(L, 1);
        QImage *img = new QImage(path);
        pushImg(L, img);
    }
    else if (type == LUA_TTABLE)
    {
        const QVariantMap param = getValue(L, false).toMap();
        int w = param.value("w", -1).toInt();
        int h = param.value("h", -1).toInt();
        QImage *img = nullptr;
        if (param.contains("data"))
        {
            QByteArray imgData = param["data"].toByteArray();
            QBuffer bufferImage(&imgData);
            bufferImage.open(QIODevice::ReadOnly);
            QImageReader reader(&bufferImage);
            if (w > 0 || h > 0)
            {
                QSize s = reader.size();
                if (w > 0) s.setWidth(w);
                if (h > 0) s.setHeight(h);
                reader.setScaledSize(s);
            }
            img = new QImage(reader.read());
        }
        else
        {
            int format = param.value("format", QImage::Format::Format_ARGB32).toInt();
            img = new QImage(w, h, static_cast<QImage::Format>(format));
        }
        pushImg(L, img);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

int AppImage::imgGC(lua_State *L)
{
    QImage *img = checkImg(L, 1);
    if(img) delete img;
    return 0;
}

int AppImage::size(lua_State *L)
{
    QImage *img = checkImg(L, 1);
    if (!img)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, img->width());
    lua_pushinteger(L, img->height());
    return 2;
}

int AppImage::save(lua_State *L)
{
    QImage *img = checkImg(L, 1);
    if (!img || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    lua_pushboolean(L, img->save(QString(lua_tostring(L, 2))));
    return 1;
}

int AppImage::tobytes(lua_State *L)
{
    QImage *img = checkImg(L, 1);
    if (!img || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    const char *format = "jpg";
    if (lua_gettop(L) > 1 && lua_type(L, 2) == LUA_TSTRING)
    {
        format = lua_tostring(L, 2);
    }
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    img->save(&buffer, format);
    lua_pushlstring(L, ba.constData(), ba.size());
    return 1;
}

int AppImage::scale(lua_State *L)
{
    QImage *img = checkImg(L, 1);
    if (!img || lua_gettop(L) < 3)
    {
        lua_pushnil(L);
        return 1;
    }
    int w = lua_tointeger(L, 2), h = lua_tointeger(L, 3);
    if (w == 0 || h == 0)
    {
        lua_pushnil(L);
        return 1;
    }
    Qt::AspectRatioMode mode = Qt::IgnoreAspectRatio;
    if (lua_gettop(L) > 3)
    {
        mode = static_cast<Qt::AspectRatioMode>(lua_tointeger(L, 4));
    }
    QImage *scaledImg = new QImage(img->scaled(w, h, mode, Qt::SmoothTransformation));
    pushImg(L, scaledImg);
    return 1;
}



}

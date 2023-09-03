#include "lua_clipboardinterface.h"
#include <QApplication>
#include <QClipboard>
#include "Extension/App/kapp.h"
#include "lua_appimage.h"
#include <QWidget>

namespace Extension
{

void ClipboardInterface::setup()
{
    const luaL_Reg funcs[] = {
        {"gettext", gettext},
        {"settext", settext},
        {"getimg", getimg},
        {"setimg", setimg},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "clipboard"}, funcs);
}

int ClipboardInterface::gettext(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app) return 0;
    QString text;
    QMetaObject::invokeMethod(app->window()->getWidget(), [&](){
        text = QApplication::clipboard()->text();
    }, Qt::BlockingQueuedConnection);
    lua_pushstring(L, text.toUtf8().constData());
    return 1;
}

int ClipboardInterface::settext(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (lua_gettop(L) != 1 || !app) return 0;
    const QString text = lua_tostring(L, 1);  
    QMetaObject::invokeMethod(app->window()->getWidget(), [=](){
        QApplication::clipboard()->setText(text);
    }, Qt::QueuedConnection);
    return 0;
}

int ClipboardInterface::getimg(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app) return 0;
    QImage *img = nullptr;
    QMetaObject::invokeMethod(app->window()->getWidget(), [&](){
        QImage clipboardImg = QApplication::clipboard()->image();
        if(!clipboardImg.isNull()) img = new QImage(clipboardImg);
    }, Qt::BlockingQueuedConnection);
    if (!img) return 0;
    AppImage::pushImg(L, img);
    return 1;
}

int ClipboardInterface::setimg(lua_State *L)
{
    if (lua_gettop(L) != 1) return 0;
    QImage *img = AppImage::checkImg(L, 1);
    KApp *app = KApp::getApp(L);
    if (!img || !app) return 0;
    QMetaObject::invokeMethod(app->window()->getWidget(), [=](){
        QApplication::clipboard()->setImage(*img);
    }, Qt::BlockingQueuedConnection);
    return 0;
}


}

#include "lua_appcommondialog.h"
#include <QFileDialog>
#include "Extension/App/kapp.h"
#include "UI/inputdialog.h"

namespace  Extension
{

void AppCommonDialog::setup()
{
    const luaL_Reg funcs[] = {
        {"openfile", openfile},
        {"savefile", savefile},
        {"selectdir", selectdir},
        {"dialog", dialog},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "dialog"}, funcs);
}

int AppCommonDialog::openfile(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) > 1)
    {
        lua_pushnil(L);
        return 1;
    }
    QVariantMap options = getValue(L, true, 2).toMap();
    const QString caption = options.value("title").toString();
    const QString dir = options.value("path").toString();
    const QString filter = options.value("filter").toString();
    const bool multiFiles = options.value("multi", false).toBool();

    if (multiFiles)
    {
        QStringList files;
        QMetaObject::invokeMethod(app->window()->getWidget(), [&](){
            files = QFileDialog::getOpenFileNames(app->window()->getWidget(), caption, dir, filter);
        }, Qt::BlockingQueuedConnection);
        if (!files.empty()) pushValue(L, files);
        else lua_pushnil(L);
    }
    else
    {
        QString file;
        QMetaObject::invokeMethod(app->window()->getWidget(), [&](){
            file = QFileDialog::getOpenFileName(app->window()->getWidget(), caption, dir, filter);
        }, Qt::BlockingQueuedConnection);
        if (!file.isEmpty()) lua_pushstring(L, file.toStdString().c_str());
        else lua_pushnil(L);
    }
    return 1;
}

int AppCommonDialog::savefile(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) > 1)
    {
        lua_pushnil(L);
        return 1;
    }
    QVariantMap options = getValue(L, true, 2).toMap();
    const QString caption = options.value("title").toString();
    const QString dir = options.value("path").toString();
    const QString filter = options.value("filter").toString();

    QString file;
    QMetaObject::invokeMethod(app->window()->getWidget(), [&](){
        file = QFileDialog::getSaveFileName(app->window()->getWidget(), caption, dir, filter);
    }, Qt::BlockingQueuedConnection);

    if (!file.isEmpty()) lua_pushstring(L, file.toStdString().c_str());
    else lua_pushnil(L);
    return 1;
}

int AppCommonDialog::selectdir(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) > 1)
    {
        lua_pushnil(L);
        return 1;
    }
    QVariantMap options = getValue(L, true, 2).toMap();
    const QString caption = options.value("title").toString();
    const QString dir = options.value("path").toString();

    QString directory;
    QMetaObject::invokeMethod(app->window()->getWidget(), [&](){
        directory = QFileDialog::getExistingDirectory(app->window()->getWidget(), caption, dir);
    }, Qt::BlockingQueuedConnection);

    if (!directory.isEmpty()) lua_pushstring(L, directory.toStdString().c_str());
    else lua_pushnil(L);
    return 1;
}

int AppCommonDialog::dialog(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) != 1)
    {
        lua_pushnil(L);
        return 1;
    }
    QVariantMap optMap = getValue(L, false, 2).toMap();
    QString title = optMap.value("title", "KikoPlay").toString();
    QString tip = optMap.value("tip").toString();
    bool hasText = optMap.contains("text"), hasImage = optMap.contains("image");

    QStringList retList;
    QMetaObject::invokeMethod(app->window()->getWidget(), [&](){
        if(!hasText && !hasImage)
        {
            InputDialog dialog(title, tip, app->window()->getWidget());
            int ret = dialog.exec();
            retList = QStringList{ret==QDialog::Accepted?"accept":"reject", ""};
        }
        else if(hasText && !hasImage)
        {
            InputDialog dialog(title, tip,  optMap.value("text").toString(), true, app->window()->getWidget());
            int ret = dialog.exec();
            retList = ret==QDialog::Accepted? QStringList({"accept", dialog.text}):QStringList({"reject", ""});
        }
        else if(!hasText && hasImage)
        {
            InputDialog dialog(optMap.value("image").toByteArray(), title, tip, app->window()->getWidget());
            int ret = dialog.exec();
            retList = QStringList{ret==QDialog::Accepted?"accept":"reject", ""};
        }
        else
        {
            InputDialog dialog(optMap.value("image").toByteArray(), title, tip,  optMap.value("text").toString(), app->window()->getWidget());
            int ret = dialog.exec();
            retList = ret==QDialog::Accepted? QStringList({"accept", dialog.text}):QStringList({"reject", ""});
        }
    }, Qt::BlockingQueuedConnection);
    lua_pushstring(L, retList[0].toStdString().c_str()); // accept/reject
    lua_pushstring(L, retList[1].toStdString().c_str());  //text
    return 2;
}


}


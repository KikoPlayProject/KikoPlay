#ifndef APPWINDOW_H
#define APPWINDOW_H

#include "appwidget.h"
namespace Extension
{
class AppWindow : public AppWidget, public AppWidgetRegister<AppWindow>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
public:
    static const char *MetaName;
    static int show(lua_State *L);
    static int raise(lua_State *L);
    static int message(lua_State *L);
public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
protected:
    explicit AppWindow(AppWidget *parent = nullptr);

    // AppWidget interface
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
    void updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params);
};
}
#endif // APPWINDOW_H

#ifndef APPLABEL_H
#define APPLABEL_H

#include "appwidget.h"
namespace Extension
{

class AppLabel : public AppWidget, public AppWidgetRegister<AppLabel>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *MetaName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);

    static int setimg(lua_State *L);
    static int getimg(lua_State *L);
    static int clear(lua_State *L);
public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
protected:
    explicit AppLabel(AppWidget *parent = nullptr);
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private slots:
    void onLinkClick(const QString &link);
};

}
#endif // APPLABEL_H

#ifndef APPGVIEW_H
#define APPGVIEW_H

#include "appview.h"
namespace Extension
{

class AppGView : public AppView, public AppWidgetRegister<AppGView>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
    static const char *MetaName;
public:
    explicit AppGView(AppWidget *parent = nullptr);
    const char *getMetaname() const { return MetaName; }
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    void updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params);
};

}

#endif // APPGVIEW_H

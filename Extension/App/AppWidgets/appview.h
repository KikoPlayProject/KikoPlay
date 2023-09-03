#ifndef APPVIEW_H
#define APPVIEW_H

#include "appwidget.h"
namespace Extension
{

class AppView : public AppWidget, public AppWidgetRegister<AppView>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
    static const char *MetaName;
protected:
    explicit AppView(AppWidget *parent = nullptr);
    const char *getMetaname() const { return MetaName; }
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
};

}
#endif // APPVIEW_H

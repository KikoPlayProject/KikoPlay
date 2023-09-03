#ifndef APPSVIEW_H
#define APPSVIEW_H

#include "appview.h"
namespace Extension
{

class AppSView : public AppView, public AppWidgetRegister<AppSView>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
    static const char *MetaName;
public:
    explicit AppSView(AppWidget *parent = nullptr);
    const char *getMetaname() const { return MetaName; }
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    void updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params);
};

}
#endif // APPSVIEW_H

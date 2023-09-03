#ifndef APPVVIEW_H
#define APPVVIEW_H

#include "appview.h"
namespace Extension
{

class AppVView : public AppView, public AppWidgetRegister<AppVView>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
    static const char *MetaName;
public:
    explicit AppVView(AppWidget *parent = nullptr);
    const char *getMetaname() const { return MetaName; }
protected:
    virtual void updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params);
};

}
#endif // APPVVIEW_H

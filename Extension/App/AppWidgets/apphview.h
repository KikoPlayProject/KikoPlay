#ifndef APPHVIEW_H
#define APPHVIEW_H

#include "appview.h"
namespace Extension
{

class AppHView : public AppView, public AppWidgetRegister<AppHView>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
    static const char *MetaName;
public:
    explicit AppHView(AppWidget *parent = nullptr);
    const char *getMetaname() const { return MetaName; }
protected:
    virtual void updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params);
};

}
#endif // APPHVIEW_H

#ifndef APPCHECKBOX_H
#define APPCHECKBOX_H

#include "appwidget.h"
namespace Extension
{

class AppCheckBox : public AppWidget, public AppWidgetRegister<AppCheckBox>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *MetaName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);

    static int click(lua_State *L);
public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
public:
    explicit AppCheckBox(AppWidget *parent = nullptr);
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private slots:
    void onStateChanged(int state);
};

}
#endif // APPCHECKBOX_H

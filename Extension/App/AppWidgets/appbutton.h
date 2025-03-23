#ifndef APPBUTTON_H
#define APPBUTTON_H

#include <UI/widgets/kpushbutton.h>
#include "appwidget.h"
class AppPushButton : public KPushButton
{
    Q_OBJECT
public:
    using KPushButton::KPushButton;

};

namespace Extension
{

class AppButton : public AppWidget, public AppWidgetRegister<AppButton>
{
    Q_OBJECT
public:
    static const char *AppWidgetName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
    static const char *MetaName;

    static int click(lua_State *L);
public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
protected:
    explicit AppButton(AppWidget *parent = nullptr);
    // AppWidget interface
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private slots:
    void onClick();
};

}
#endif // APPBUTTON_H

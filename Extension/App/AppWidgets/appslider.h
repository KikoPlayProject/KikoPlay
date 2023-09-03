#ifndef APPSLIDER_H
#define APPSLIDER_H

#include "appwidget.h"
namespace Extension
{
class AppSlider : public AppWidget, public AppWidgetRegister<AppSlider>
{
    Q_OBJECT
public:
    static const char *AppWidgetName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
    static const char *MetaName;

public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
protected:
    explicit AppSlider(AppWidget *parent = nullptr);
    // AppWidget interface
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private slots:
    void onValueChanged(int value);
};

}
#endif // APPSLIDER_H

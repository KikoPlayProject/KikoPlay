#ifndef APPRADIO_H
#define APPRADIO_H

#include "appwidget.h"
namespace Extension
{
class AppRadio : public AppWidget, public AppWidgetRegister<AppRadio>
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
    explicit AppRadio(AppWidget *parent = nullptr);
    // AppWidget interface
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private slots:
    void onToggled(bool checked);
};
}
#endif // APPRADIO_H

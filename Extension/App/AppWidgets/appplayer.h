#ifndef APPPLAYER_H
#define APPPLAYER_H

#include "appwidget.h"
class SimplePlayer;
namespace Extension
{

class AppPlayer : public AppWidget, public AppWidgetRegister<AppPlayer>
{
    Q_OBJECT
public:
    static const char *AppWidgetName;
    static const char *StyleClassName;
    static const char *MetaName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);

    static int property(lua_State *L);
    static int command(lua_State *L);

public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
private:
    SimplePlayer *smPlayer;
protected:
    explicit AppPlayer(AppWidget *parent = nullptr);
    ~AppPlayer();
    // AppWidget interface
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private slots:
    void onClick();
    void onPosChanged(double value);
    void onStateChanged(int state);
    void onDurationChanged(double duration);
};

}
#endif // APPPLAYER_H

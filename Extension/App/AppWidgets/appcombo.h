#ifndef APPCOMBO_H
#define APPCOMBO_H

#include "appwidget.h"
namespace Extension
{

class AppCombo : public AppWidget, public AppWidgetRegister<AppCombo>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *MetaName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);

    static int append(lua_State *L);
    static int insert(lua_State *L);
    static int item(lua_State *L);
    static int remove(lua_State *L);
    static int clear(lua_State *L);
public:
    explicit AppCombo(AppWidget *parent = nullptr);
public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private:
    int dataRef;
    static int getDataRef(lua_State *L, AppCombo *appCombo);
    static void removeDataRef(lua_State *L, AppCombo *appCombo, int ref);
    static void parseItems(lua_State *L, QVector<QPair<QString, QVariant>> &items, AppCombo *appCombo);
private slots:
    void onCurrentChanged(int index);
    void onTextChanged(const QString &text);
};

}
#endif // APPCOMBO_H

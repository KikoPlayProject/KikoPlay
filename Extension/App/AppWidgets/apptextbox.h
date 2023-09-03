#ifndef APPTEXTBOX_H
#define APPTEXTBOX_H

#include "appwidget.h"
namespace Extension
{

class AppTextBox : public AppWidget, public AppWidgetRegister<AppTextBox>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *MetaName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);

    static int append(lua_State *L);
    static int toend(lua_State *L);
public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
public:
    explicit AppTextBox(AppWidget *parent = nullptr);
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private slots:
    void onTextChanged();
};
}
#endif // APPTEXTBOX_H

#ifndef APPTEXTLINE_H
#define APPTEXTLINE_H

#include "appwidget.h"
namespace Extension
{

class AppTextLine : public AppWidget, public AppWidgetRegister<AppTextLine>
{
    Q_OBJECT
public:
    static const char* AppWidgetName;
    static const char *MetaName;
    static const char *StyleClassName;
    static AppWidget *create(AppWidget *parent, KApp *app);
    static void registEnv(lua_State *L);
public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
public:
    explicit AppTextLine(AppWidget *parent = nullptr);
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private slots:
    void onTextChanged(const QString &text);
    void onReturnPressed();
};

}
#endif // APPTEXTLINE_H

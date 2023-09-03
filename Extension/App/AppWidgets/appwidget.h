#ifndef APPWIDGET_H
#define APPWIDGET_H
#include <QObject>
#include <QVariant>
#include <QSharedPointer>
#include <QButtonGroup>
#include "apputil.h"
#include "Extension/Lua/lua.hpp"
#include "Extension/Common/ext_common.h"

class QWidget;
class QAction;
class QXmlStreamReader;
namespace Extension
{
class KApp;
class AppWidget: public QObject
{
    Q_OBJECT
public:
    using CreateFunc = std::function<AppWidget*(AppWidget *, KApp *)>;
    using EnvRegistFunc = std::function<void(lua_State *)>;
    static void registAppWidget(const QString &name, const CreateFunc &func, const QString &metaname, const EnvRegistFunc &regFunc);
    static void setStyleClass(const QString &name, const QString &styleClass);


    static AppWidget *create(const QString &name, AppWidget *parent, KApp *app);
    static AppWidget *parseFromXml(const QString &data,  KApp *app, const QString &specificRoot = "", QString *errInfo = nullptr, AppWidget *p = nullptr);
    static QVector<QAction *> parseMenu(QXmlStreamReader &reader,  AppWidget *parent);
    static AppWidget *parseFromXmlFile(const QString &filename,  KApp *app, const QString &specificRoot = "", QString *errInfo = nullptr, AppWidget *parent = nullptr);
public:
    static const char *MetaName;
    static AppWidget *checkWidget(lua_State *L, int pos, const char *metaName);
    static void registClassFuncs(lua_State *L, const char *metaName, const luaL_Reg *funcs, const char *pMetaname = nullptr);
    static void registEnv(lua_State *L);
    static int getopt(lua_State *L);
    static int setopt(lua_State *L);
    static int data(lua_State *L);
    static int setstyle(lua_State *L);
    static int addchild(lua_State *L);
    static int removechild(lua_State *L);
    static int getchild(lua_State *L);
    static int widgetparent(lua_State *L);
    static int onevent(lua_State *L);
    static int adjustsize(lua_State *L);
public:
    virtual ~AppWidget();
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
    void luaPush(lua_State *L);
    bool setOption(AppWidgetOption option, const QVariant &val);
    QVariant getOption(AppWidgetOption option, bool *succ = nullptr);
    QWidget *getWidget() { return widget; }
    KApp *getApp() { return app; }
    void setStyle(const QString &qss, const QVariantMap *extraVals = nullptr);
protected:
    AppWidget(QObject *parent = nullptr);
    virtual bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    virtual QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    virtual void updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params) {}
    virtual const char *getMetaname() const = 0;
    virtual void addActions(const QVector<QAction *> &actions);

    QWidget *widget;
    KApp *app;
    QHash<AppEvent, QString> bindEvents;
private:
    static QSharedPointer<QHash<QString, CreateFunc>> appCreateHash;
    static QSharedPointer<QHash<QString, EnvRegistFunc>> envRegistHash;
    static QSharedPointer<QHash<QString, QString>> styleClassHash;
};

template <typename T>
class AppWidgetRegister
{
public:
    AppWidgetRegister()
    {
        reg.ping();
    }
private:
    struct Register
    {
        Register()
        {
            AppWidget::registAppWidget(T::AppWidgetName, T::create, T::MetaName, T::registEnv);
            AppWidget::setStyleClass(T::AppWidgetName, T::StyleClassName);
        }
        void ping() {}
    };
    static Register reg;
};
template <typename T>
typename AppWidgetRegister<T>::Register AppWidgetRegister<T>::reg;

struct WindowRes : public AppRes
{
    WindowRes(AppWidget *w) : window(w) {}
    ~WindowRes() {  if (window)  window->deleteLater(); }
    AppWidget *window;
};

struct AppButtonGroupRes : public AppRes
{
    AppButtonGroupRes()  {}
    ~AppButtonGroupRes() { for (auto g : btnGroups) g->deleteLater();  }
    QButtonGroup *getButtonGroup(const QString &group)
    {
        if (!btnGroups.contains(group)) btnGroups[group] = new QButtonGroup;
        return btnGroups.value(group);
    }
    QMap<QString, QButtonGroup *> btnGroups;

    static const char *resKey;
    static AppButtonGroupRes *getButtonRes(KApp *app);
};

}
Q_DECLARE_METATYPE(Extension::AppWidget*);

#endif // APPWIDGET_H

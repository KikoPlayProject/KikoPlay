#ifndef APPLIST_H
#define APPLIST_H

#include "appwidget.h"
class QListWidgetItem;
namespace Extension
{

class AppList : public AppWidget, public AppWidgetRegister<AppList>
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
    static int remove(lua_State *L);
    static int clear(lua_State *L);
    static int item(lua_State *L);
    static int set(lua_State *L);
    static int setview(lua_State *L);
    static int getview(lua_State *L);
    static int selection(lua_State *L);
    static int scrollto(lua_State *L);

public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);

public:
    explicit AppList(AppWidget *parent = nullptr);
    ~AppList();

protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }

private:
    int dataRef;
    static int getDataRef(lua_State *L, AppList *appList);
    static void removeDataRef(lua_State *L, AppList *appList, int ref);
    static void parseItems(lua_State *L, QVector<QListWidgetItem *> &items, AppList *appList);
    static void setItem(QListWidgetItem *item, const QString &field, const QVariant &val);
    static QVariantMap itemToMap(QListWidgetItem *item);

private slots:
    void onItemClick(QListWidgetItem *item);
    void onItemDoubleClick(QListWidgetItem *item);
    void onScrollEdge(bool bottom);
};
}

#endif // APPLIST_H

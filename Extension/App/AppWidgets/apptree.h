#ifndef APPTREE_H
#define APPTREE_H

#include "appwidget.h"
class QTreeWidgetItem;
namespace Extension
{
class AppTree;
struct AppTreeItem
{
    static const char *MetaName;
    static QTreeWidgetItem *checkItem(lua_State *L, int pos);
    static AppTree *getAppTree(QTreeWidgetItem *item);
    static void pushItem(lua_State *L, QTreeWidgetItem *item);
    static void pushItems(lua_State *L, const QList<QTreeWidgetItem *> &items);
    static void pushItemCol(lua_State *L, QTreeWidgetItem *item, int col);
    static void pushItemCol(lua_State *L, QTreeWidgetItem *item, int col, const QString &field);
    static void setItemCol(QTreeWidgetItem *item, int col, const QString &field, const QVariant &val);

    static int append(lua_State *L);
    static int insert(lua_State *L);
    static int remove(lua_State *L);
    static int clear(lua_State *L);
    static int get(lua_State *L);
    static int set(lua_State *L);
    static int scrollto(lua_State *L);
    static int parent(lua_State *L);
    static int child(lua_State *L);
    static int childcount(lua_State *L);
    static int indexof(lua_State *L);
};

class AppTree : public AppWidget, public AppWidgetRegister<AppTree>
{
    Q_OBJECT
    friend struct AppTreeItem;
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
    static int indexof(lua_State *L);
    static int selection(lua_State *L);
    static int setheader(lua_State *L);
    static int headerwidth(lua_State *L);

public:
    explicit AppTree(AppWidget *parent = nullptr);
    ~AppTree();
public:
    virtual void bindEvent(AppEvent event, const QString &luaFunc);
protected:
    bool setWidgetOption(AppWidgetOption option, const QVariant &val);
    QVariant getWidgetOption(AppWidgetOption option, bool *succ = nullptr);
    const char *getMetaname() const { return MetaName; }
private:
    int dataRef;
    static int getDataRef(lua_State *L, AppTree *appTree);
    static void removeDataRef(lua_State *L, QTreeWidgetItem *item, bool onlyChild = false);
    static void parseItems(lua_State *L, QList<QTreeWidgetItem *> &items, AppTree *appTree);
private slots:
    void onItemClick(QTreeWidgetItem *item);
    void onItemDoubleClick(QTreeWidgetItem *item);
    void onScrollEdge(bool bottom);
};

}
#endif // APPTREE_H

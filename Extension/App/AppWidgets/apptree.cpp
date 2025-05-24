#include "apptree.h"
#include <QTreeWidget>
#include <QHeaderView>
#include <QFile>
#include <QBuffer>
#include <QPixmap>
#include <QIcon>
#include <QImageReader>
#include <QScrollBar>
#include "Extension/Common/ext_common.h"
#include "Extension/App/kapp.h"
#include "Extension/Modules/lua_appimage.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#define TreeItemDataRole Qt::UserRole + 1

namespace Extension
{
const char *AppTree::AppWidgetName = "tree";
const char *AppTree::MetaName = "meta.kiko.widget.tree";
const char *AppTree::StyleClassName = "QTreeWidget";
const char *AppTreeItem::MetaName = "meta.kiko.treeitem";

AppWidget *AppTree::create(AppWidget *parent, KApp *app)
{
    return new AppTree(parent);
}

void AppTree::registEnv(lua_State *L)
{
    const luaL_Reg itemMembers[] = {
        {"append", AppTreeItem::append},
        {"insert", AppTreeItem::insert},
        {"remove", AppTreeItem::remove},
        {"clear",  AppTreeItem::clear},
        {"get", AppTreeItem::get},
        {"set", AppTreeItem::set},
        {"scrollto", AppTreeItem::scrollto},
        {"parent", AppTreeItem::parent},
        {"child", AppTreeItem::child},
        {"childcount", AppTreeItem::childcount},
        {"indexof", AppTreeItem::indexof},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppTreeItem::MetaName, itemMembers);
    const luaL_Reg members[] = {
        {"append", append},
        {"insert", insert},
        {"remove", remove},
        {"clear", clear},
        {"item", item},
        {"indexof", indexof},
        {"selection", selection},
        {"setheader", setheader},
        {"headerwidth", headerwidth},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppTree::MetaName, members, AppWidget::MetaName);
}

int AppTree::append(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget || lua_gettop(L) < 2)
    {
        lua_pushnil(L);
        return 1;
    }
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    // append({item1, item2, ... })
    QList<QTreeWidgetItem *> items;
    parseItems(L, items, static_cast<AppTree *>(appWidget));
    if (items.empty())
    {
        lua_pushnil(L);
        return 1;
    }
    QMetaObject::invokeMethod(tree, [&](){
       tree->addTopLevelItems(items);
    }, Qt::BlockingQueuedConnection);
    AppTreeItem::pushItems(L, items);
    return 1;
}

int AppTree::insert(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget || lua_gettop(L) < 3 || lua_type(L, -2) != LUA_TNUMBER)
    {
        lua_pushnil(L);
        return 1;
    }
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    // insert(pos, {item1, item2, ... })
    const int row = lua_tointeger(L, 2) - 1;
    QList<QTreeWidgetItem *> items;
    parseItems(L, items, static_cast<AppTree *>(appWidget));
    if (items.empty())
    {
        lua_pushnil(L);
        return 1;
    }
    QMetaObject::invokeMethod(tree, [&items, row, tree](){
        tree->insertTopLevelItems(row, items);
    }, Qt::BlockingQueuedConnection);
    AppTreeItem::pushItems(L, items);
    return 1;
}

int AppTree::remove(lua_State *L)
{
    // remove(pos)
    // remove(item)
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget || lua_gettop(L) < 2) return 0;
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    int row = -1;
    if (lua_type(L, -1) == LUA_TNUMBER)
    {
        row = lua_tointeger(L, -1) - 1;
    }
    else
    {
        QTreeWidgetItem *item = AppTreeItem::checkItem(L, -1);
        row = tree->indexOfTopLevelItem(item);
    }
    if (row < 0) return 0;
    removeDataRef(L, tree->topLevelItem(row));
    QMetaObject::invokeMethod(tree, [row, tree](){
        QTreeWidgetItem *item = tree->takeTopLevelItem(row);
        if (item) delete item;
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppTree::clear(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget) return 0;
    AppTree *appTree = static_cast<AppTree *>(appWidget);
    if (appTree->dataRef > 0)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, appTree->dataRef);
        appTree->dataRef = -1;
    }
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    QMetaObject::invokeMethod(tree, "clear", Qt::BlockingQueuedConnection);
    return 0;
}

int AppTree::item(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget || lua_gettop(L) < 2 || lua_type(L, -1) != LUA_TNUMBER)
    {
        lua_pushnil(L);
        return 1;
    }
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    // item(pos)
    const int row = lua_tointeger(L, 2) - 1;
    QTreeWidgetItem *item = tree->topLevelItem(row);
    if (!item)
    {
        lua_pushnil(L);
        return 1;
    }
    AppTreeItem::pushItem(L, item);
    return 1;
}

int AppTree::indexof(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget || lua_gettop(L) < 2)
    {
        lua_pushinteger(L, -1);
        return 1;
    }
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    QTreeWidgetItem *item = AppTreeItem::checkItem(L, 2);
    if (!item)
    {
        lua_pushinteger(L, -1);
        return 1;
    }
    lua_pushinteger(L, tree->indexOfTopLevelItem(item) + 1);
    return 1;
}

int AppTree::selection(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget)
    {
        lua_pushnil(L);
        return 1;
    }
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    QList<QTreeWidgetItem *> selItems;
    QMetaObject::invokeMethod(tree, [&](){
        selItems = tree->selectedItems();
    }, Qt::BlockingQueuedConnection);
    lua_newtable(L);  // table
    for (int i = 0; i < selItems.size(); ++i)
    {
        AppTreeItem::pushItem(L, selItems[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

int AppTree::setheader(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget || lua_gettop(L) < 2 || lua_type(L, -1) != LUA_TTABLE) return 0;
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    // setheader({h1, h2, ...})
    const QVariant headers = getValue(L, true);
    if (!headers.canConvert(QMetaType::QStringList)) return 0;
    QMetaObject::invokeMethod(tree, [tree, &headers](){
       tree->setHeaderLabels(headers.toStringList());
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppTree::headerwidth(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTree::MetaName);
    if (!appWidget || lua_gettop(L) < 3 || lua_type(L, 2) != LUA_TSTRING || lua_type(L, 3) != LUA_TNUMBER) return 0;
    QTreeWidget *tree = static_cast<QTreeWidget *>(appWidget->getWidget());
    // headerwidth("get"/"set", col, width)
    int col = lua_tointeger(L, 3) - 1;
    if (QString(lua_tostring(L, 2)) == "get")
    {
        int w = 0;
        QMetaObject::invokeMethod(tree, [tree, col, &w](){
            w = tree->header()->sectionSize(col) * 96 / tree->header()->logicalDpiX();
        }, Qt::BlockingQueuedConnection);
        lua_pushinteger(L, w);
        return 1;
    }
    else
    {
        int w = lua_tointeger(L, 4);
        QMetaObject::invokeMethod(tree, [tree, col, &w](){
            tree->header()->resizeSection(col, w * tree->header()->logicalDpiX() / 96);
        }, Qt::BlockingQueuedConnection);
        return 0;
    }
}

AppTree::AppTree(AppWidget *parent) : AppWidget{parent}, dataRef(-1)
{
    QTreeWidget *tree = new QTreeWidget(parent? parent->getWidget() : nullptr);
    tree->setProperty("__appwidget", QVariant::fromValue((AppWidget *)this));
    tree->setAnimated(true);
    tree->setItemDelegate(new KTreeviewItemDelegate(tree));
    this->widget = tree;
}

AppTree::~AppTree()
{
    if (dataRef > 0 && app && app->getState())
    {
        luaL_unref(app->getState(), LUA_REGISTRYINDEX, dataRef);
    }
}

void AppTree::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QTreeWidget *tree = static_cast<QTreeWidget *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_ITEM_CLICK:
    {
        QObject::connect(tree, &QTreeWidget::itemClicked, this, &AppTree::onItemClick);
        break;
    }
    case AppEvent::EVENT_ITEM_DOUBLE_CLICK:
    {
        QObject::connect(tree, &QTreeWidget::itemDoubleClicked, this, &AppTree::onItemDoubleClick);
        break;
    }
    case AppEvent::EVENT_ITEM_CHANGED:
    {
        QObject::connect(tree, &QTreeWidget::itemChanged, this, &AppTree::onItemChanged);
        break;
    }
    case AppEvent::EVENT_SCROLL_EDGE:
    {
        QObject::connect(tree->verticalScrollBar(), &QScrollBar::valueChanged, [=](int val){
            if (val == tree->verticalScrollBar()->minimum())
            {
                QMetaObject::invokeMethod(this, "onScrollEdge", Qt::QueuedConnection, Q_ARG(bool, false));
            }
            else if (val == tree->verticalScrollBar()->maximum())
            {
                QMetaObject::invokeMethod(this, "onScrollEdge", Qt::QueuedConnection, Q_ARG(bool, true));
            }
        });
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

bool AppTree::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QTreeWidget *tree = static_cast<QTreeWidget *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_ROOT_DECORATED:
    {
        tree->setRootIsDecorated(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_ALTER_ROW_COLOR:
    {
        tree->setAlternatingRowColors(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_HEADER_VISIBLE:
    {
        tree->setHeaderHidden(!val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_SORTABLE:
    {
        tree->setSortingEnabled(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_LIST_SELECTION_MODE:
    {
        static const QHash<QString, QAbstractItemView::SelectionMode> modeHash = {
            {"none",   QAbstractItemView::SelectionMode::NoSelection},
            {"single", QAbstractItemView::SelectionMode::SingleSelection},
            {"multi",  QAbstractItemView::SelectionMode::MultiSelection},
        };
        if (modeHash.contains(val.toString()))
        {
            tree->setSelectionMode(modeHash[val.toString()]);
        }
        break;
    }
    case AppWidgetOption::OPTION_DISABLE_H_SCROLLBAR:
    {
        tree->setHorizontalScrollBarPolicy(val.toBool()? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
        break;
    }
    case AppWidgetOption::OPTION_DISABLE_V_SCROLLBAR:
    {
        tree->setVerticalScrollBarPolicy(val.toBool()? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
        break;
    }
    case AppWidgetOption::OPTION_WORD_WRAP:
    {
        tree->setWordWrap(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_ELIDEMODE:
    {
        static const QHash<QString, Qt::TextElideMode> mode {
            {"left",   Qt::TextElideMode::ElideLeft},
            {"right",  Qt::TextElideMode::ElideRight},
            {"middle", Qt::TextElideMode::ElideMiddle},
            {"none",   Qt::TextElideMode::ElideNone},
        };
        tree->setTextElideMode(mode.value(val.toString(), Qt::TextElideMode::ElideRight));
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppTree::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    QTreeWidget *tree = static_cast<QTreeWidget *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_COUNT:
    {
        return tree->topLevelItemCount();
    }
    case AppWidgetOption::OPTION_COLUMN_COUNT:
    {
        return tree->columnCount();
    }
    case AppWidgetOption::OPTION_HEADER_VISIBLE:
    {
        return !tree->isHeaderHidden();
    }
    case AppWidgetOption::OPTION_ROOT_DECORATED:
    {
        return tree->rootIsDecorated();
    }
    case AppWidgetOption::OPTION_ALTER_ROW_COLOR:
    {
        return tree->alternatingRowColors();
    }
    case AppWidgetOption::OPTION_SORTABLE:
    {
        return tree->isSortingEnabled();
    }
    case AppWidgetOption::OPTION_WORD_WRAP:
    {
        return tree->wordWrap();
    }
    case AppWidgetOption::OPTION_LIST_SELECTION_MODE:
    {
        QAbstractItemView::SelectionMode mode = tree->selectionMode();
        static const char *modeType[] = {"none", "single", "multi"};
        return modeType[mode];
    }
    case AppWidgetOption::OPTION_DISABLE_H_SCROLLBAR:
    {
        return tree->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
    }
    case AppWidgetOption::OPTION_DISABLE_V_SCROLLBAR:
    {
        return tree->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
    }
    case AppWidgetOption::OPTION_ELIDEMODE:
    {
        Qt::TextElideMode mode = tree->textElideMode();
        static const char *modeType[] = {"left", "right", "middle", "none"};
        return modeType[mode];
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

int AppTree::getDataRef(lua_State *L, AppTree *appTree)
{
    if (appTree->dataRef < 0)
    {
        lua_newtable(L);
        appTree->dataRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, appTree->dataRef);  // data data_tb
    lua_pushvalue(L, -2);  // data data_tb  data
    const int ref = luaL_ref(L, -2); // data data_tb
    lua_pop(L, 1);
    return ref;
}

void AppTree::removeDataRef(lua_State *L, QTreeWidgetItem *item, bool onlyChild)
{
    if (!item) return;
    AppTree *appTree = AppTreeItem::getAppTree(item);
    if (!appTree) return;
    QVector<QTreeWidgetItem *> items{item};
    if (onlyChild)
    {
        QTreeWidgetItem *currentItem = items.takeFirst();
        for (int i = 0; i < currentItem->childCount(); ++i)
        {
            items.append(currentItem->child(i));
        }
    }
    QVector<int> refs;
    while(!items.empty())
    {
        QTreeWidgetItem *currentItem = items.takeFirst();
        for (int i = 0; i < currentItem->childCount(); ++i)
        {
            items.append(currentItem->child(i));
        }
        for (int j = 0; j < currentItem->columnCount(); ++j)
        {
            const QVariant data = currentItem->data(j, TreeItemDataRole);
            if (data.isValid() && data.userType() == QMetaType::type("Extension::LuaItemRef"))
            {
                refs << data.value<Extension::LuaItemRef>().ref;
            }
        }
    }
    if (!refs.empty())
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, appTree->dataRef);  // data_tb
        for (int ref : refs)
        {
            luaL_unref(L, -1, ref);
        }
        lua_pop(L, 1);
    }
}

void AppTree::parseItems(lua_State *L, QList<QTreeWidgetItem *> &items, AppTree *appTree)
{
    // {col1, col2, col3, ...}
    // {{col1, col2, col3}, {col1, col2, col3}, ...}
    // {{{text=xx, tip=xx, align=xx, bg=xx, fg=xx, data=xx, check=xx(none, true, false)}, ...}, ...}
    auto modifer = [appTree](lua_State *L, QVariantMap &map, int level){
        if (appTree && level < 3 && map.contains("data"))
        {
            lua_pushstring(L, "data");  // t key
            const int type = lua_gettable(L, -2);
            if (type == LUA_TFUNCTION || type == LUA_TTABLE)
            {
                const int ref = appTree->getDataRef(L, appTree);
                map["data"] = QVariant::fromValue<Extension::LuaItemRef>({ref, appTree->dataRef});
            }
            lua_pop(L, 1);
        }
        if (appTree && level < 3 && map.contains("icon"))
        {
            lua_pushstring(L, "icon");  // t key
            const int type = lua_gettable(L, -2);
            if (type == LUA_TSTRING)
            {
                size_t len = 0;
                const char * s = lua_tolstring(L, -1, &len);
                if (QFile::exists(s))
                {
                    map["icon"] = QIcon(s);
                }
                else
                {
                    QByteArray imgBytes(s, len);
                    QBuffer bufferImage(&imgBytes);
                    bufferImage.open(QIODevice::ReadOnly);
                    QImageReader reader(&bufferImage);
                    map["icon"] = QIcon(QPixmap::fromImageReader(&reader));
                }
            }
            else if (type == LUA_TUSERDATA)
            {
                QImage *img = AppImage::checkImg(L, -1);
                if (img)
                {
                    map["icon"] = QIcon(QPixmap::fromImage(*img));
                }
            }
            lua_pop(L, 1);
        }
    };
    const QVariantList itemParam = getValue(L, true, modifer, 3).toList();
    QVariantList rawItems;
    bool isStringList = true;
    for (auto &item : itemParam)
    {
        if (!item.canConvert(QMetaType::QString))
        {
            isStringList = false;
            break;
        }
    }
    rawItems = isStringList ? QVariantList(std::initializer_list<QVariant>{itemParam}) : itemParam;
    if (rawItems.empty()) return;
    for (auto &item : rawItems)
    {
        if (item.typeId() == QMetaType::QVariantList)
        {
            const QVariantList itemCols = item.toList();
            int colIndex = 0;
            QTreeWidgetItem *treeItem = new QTreeWidgetItem;
            items.append(treeItem);
            for (auto &col : itemCols)
            {
                if (col.canConvert(QMetaType::QVariantMap))
                {
                    const QVariantMap colInfo = col.toMap();
                    for (auto iter = colInfo.begin(); iter != colInfo.end(); ++iter)
                    {
                        AppTreeItem::setItemCol(treeItem, colIndex, iter.key(), iter.value());
                    }
                }
                else
                {
                    treeItem->setText(colIndex, col.toString());
                }
                ++colIndex;
            }
        }
    }
}

void AppTree::onItemClick(QTreeWidgetItem *item, int col)
{
    if (app && item && bindEvents.contains(AppEvent::EVENT_ITEM_CLICK))
    {
        lua_newtable(app->getState());  // table
        lua_pushstring(app->getState(), "item");
        AppTreeItem::pushItem(app->getState(), item);  // table key item
        lua_rawset(app->getState(), -3);  // table

        lua_pushstring(app->getState(), "col");
        lua_pushinteger(app->getState(), col + 1);
        lua_rawset(app->getState(), -3);

        lua_pushstring(app->getState(), "srcId");
        lua_pushstring(app->getState(), this->objectName().toUtf8().constData());
        lua_rawset(app->getState(), -3);

        lua_pushstring(app->getState(), "src");
        this->luaPush(app->getState());
        lua_rawset(app->getState(), -3);

        app->eventCall(bindEvents[AppEvent::EVENT_ITEM_CLICK], 1);
    }
}

void AppTree::onItemDoubleClick(QTreeWidgetItem *item, int col)
{
    if (app && item && bindEvents.contains(AppEvent::EVENT_ITEM_DOUBLE_CLICK))
    {
        lua_newtable(app->getState());  // table
        lua_pushstring(app->getState(), "item");
        AppTreeItem::pushItem(app->getState(), item);  // table key item
        lua_rawset(app->getState(), -3);  // table

        lua_pushstring(app->getState(), "col");
        lua_pushinteger(app->getState(), col + 1);
        lua_rawset(app->getState(), -3);

        lua_pushstring(app->getState(), "srcId");
        lua_pushstring(app->getState(), this->objectName().toUtf8().constData());
        lua_rawset(app->getState(), -3);

        lua_pushstring(app->getState(), "src");
        this->luaPush(app->getState());
        lua_rawset(app->getState(), -3);

        app->eventCall(bindEvents[AppEvent::EVENT_ITEM_DOUBLE_CLICK], 1);
    }
}

void AppTree::onItemChanged(QTreeWidgetItem *item, int col)
{
    if (app && item && bindEvents.contains(AppEvent::EVENT_ITEM_CHANGED))
    {
        lua_newtable(app->getState());  // table
        lua_pushstring(app->getState(), "item");
        AppTreeItem::pushItem(app->getState(), item);  // table key item
        lua_rawset(app->getState(), -3);  // table

        lua_pushstring(app->getState(), "col");
        lua_pushinteger(app->getState(), col + 1);
        lua_rawset(app->getState(), -3);

        lua_pushstring(app->getState(), "srcId");
        lua_pushstring(app->getState(), this->objectName().toUtf8().constData());
        lua_rawset(app->getState(), -3);

        lua_pushstring(app->getState(), "src");
        this->luaPush(app->getState());
        lua_rawset(app->getState(), -3);

        app->eventCall(bindEvents[AppEvent::EVENT_ITEM_CHANGED], 1);
    }
}

void AppTree::onScrollEdge(bool bottom)
{
    if (app && bindEvents.contains(AppEvent::EVENT_SCROLL_EDGE))
    {
        const QVariantMap params = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"bottom", bottom},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_SCROLL_EDGE], params);
    }
}

QTreeWidgetItem *AppTreeItem::checkItem(lua_State *L, int pos)
{
    void *ud = luaL_checkudata(L, pos, AppTreeItem::MetaName);
    luaL_argcheck(L, ud != NULL, pos, "treeitem expected");
    return *(QTreeWidgetItem **)ud;
}

AppTree *AppTreeItem::getAppTree(QTreeWidgetItem *item)
{
    const QVariant v = item->treeWidget()->property("__appwidget");
    AppTree *appTree = nullptr;
    if (v.userType() == QMetaType::type("Extension::AppWidget*"))
    {
        appTree = static_cast<AppTree *>(v.value<Extension::AppWidget*>());
    }
    return appTree;
}

void AppTreeItem::pushItem(lua_State *L, QTreeWidgetItem *item)
{
    QTreeWidgetItem **treeItem = (QTreeWidgetItem **)lua_newuserdata(L, sizeof(QTreeWidgetItem *));
    luaL_getmetatable(L, AppTreeItem::MetaName);
    lua_setmetatable(L, -2);  // item meta
    *treeItem = item;  // item
}

void AppTreeItem::pushItems(lua_State *L, const QList<QTreeWidgetItem *> &items)
{
    if (items.size() == 1)
    {
        AppTreeItem::pushItem(L, items.back());
        return;
    }
    lua_newtable(L); // table
    for(int i = 0; i < items.size(); ++i)
    {
        AppTreeItem::pushItem(L, items[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

void AppTreeItem::pushItemCol(lua_State *L, QTreeWidgetItem *item, int col)
{
    static const char *fields[] = {
        "text", "tip", "child_size", "align", "bg", "fg", "data", "check", "collapse", "edit"
    };
    lua_newtable(L);  // table
    for (const char *field : fields)
    {
        lua_pushstring(L, field);  // table key
        pushItemCol(L, item, col, field);  // table key value
        lua_rawset(L, -3);
    }
}

void AppTreeItem::pushItemCol(lua_State *L, QTreeWidgetItem *item, int col, const QString &field)
{
    if (field == "text")
    {
        pushValue(L, item->text(col));
    }
    else if (field == "tip")
    {
        pushValue(L, item->toolTip(col));
    }
    else if (field == "child_size")
    {
        pushValue(L, item->childCount());
    }
    else if (field == "align")
    {
        pushValue(L, item->textAlignment(col));
    }
    else if (field == "bg")
    {
        pushValue(L, item->background(col).color().rgb());
    }
    else if (field == "fg")
    {
        pushValue(L, item->foreground(col).color().rgb());
    }
    else if (field == "data")
    {
        pushValue(L, item->data(col, TreeItemDataRole));
    }
    else if (field == "check")
    {
        if (item->flags() & Qt::ItemIsUserCheckable)
            pushValue(L, item->checkState(col) == Qt::Checked? "true" : "false");
        else
            pushValue(L, "none");
    }
    else if (field == "collapse")
    {
        pushValue(L, !item->isExpanded());
    }
    else if (field == "edit")
    {
        pushValue(L, item->flags().testFlag(Qt::ItemIsEditable));
    }
    else
    {
        lua_pushnil(L);
    }
}

void AppTreeItem::setItemCol(QTreeWidgetItem *item, int col, const QString &field, const QVariant &val)
{
    if (field == "text")
    {
        item->setText(col, val.toString());
    }
    else if (field == "tip")
    {
        item->setToolTip(col, val.toString());
    }
    else if (field == "align")
    {
        bool ok = false;
        int alignment = val.toInt(&ok);
        if (ok && alignment >= 0)
        {
            item->setTextAlignment(col, alignment);
        }
    }
    else if (field == "fg")
    {
        bool ok = false;
        const int fgColor = val.toInt(&ok);
        if (ok)
        {
            item->setForeground(col, QColor((fgColor >> 16)&0xff, (fgColor >> 8)&0xff, fgColor&0xff));
        }
    }
    else if (field == "bg")
    {
        bool ok = false;
        const int fgColor = val.toInt(&ok);
        if (ok)
        {
            item->setBackground(col, QColor((fgColor >> 16)&0xff, (fgColor >> 8)&0xff, fgColor&0xff));
        }
    }
    else if (field == "data")
    {
        item->setData(col, TreeItemDataRole, val);
    }
    else if (field == "check")
    {
        const QString checkInfo = val.toString().toLower();
        if (checkInfo != "none")
        {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(col, checkInfo == "true"? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        }
        else
        {
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        }
    }
    else if (field == "collapse")
    {
        item->setExpanded(!val.toBool());
    }
    else if (field == "icon")
    {
        item->setIcon(col, val.value<QIcon>());
    }
    else if (field == "edit")
    {
        item->setFlags(val.toBool()? (item->flags() | Qt::ItemIsEditable) : (item->flags() & ~Qt::ItemIsEditable));
    }
}

int AppTreeItem::append(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    if (!item || !item->treeWidget() || lua_gettop(L) < 2)
    {
        lua_pushnil(L);
        return 1;
    }
    // append({item1, item2, ... })
    QList<QTreeWidgetItem *> items;
    AppTree::parseItems(L, items, getAppTree(item));
    if (items.empty())
    {
        lua_pushnil(L);
        return 1;
    }
    QMetaObject::invokeMethod(item->treeWidget(), [&](){
       item->addChildren(items);
    }, Qt::BlockingQueuedConnection);
    AppTreeItem::pushItems(L, items);
    return 1;
}

int AppTreeItem::insert(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    if (!item || !item->treeWidget() || lua_gettop(L) < 2)
    {
        lua_pushnil(L);
        return 1;
    }
    // insert(pos, {item1, item2, ... })
    const int row = lua_tointeger(L, 2) - 1;
    QList<QTreeWidgetItem *> items;
    AppTree::parseItems(L, items, getAppTree(item));
    if (items.empty())
    {
        lua_pushnil(L);
        return 1;
    }
    QMetaObject::invokeMethod(item->treeWidget(), [&](){
        item->insertChildren(row, items);
    }, Qt::BlockingQueuedConnection);
    AppTreeItem::pushItems(L, items);
    return 1;
}

int AppTreeItem::remove(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    if (!item || !item->treeWidget() || lua_gettop(L) < 2) return 0;
    // remove(pos)
    // remove(child)
    int row = -1;
    if (lua_type(L, -1) == LUA_TNUMBER)
    {
        row = lua_tointeger(L, -1) - 1;
    }
    else
    {
        QTreeWidgetItem *rmItem = AppTreeItem::checkItem(L, -1);
        row = item->indexOfChild(rmItem);
    }
    if (row < 0) return 0;
    AppTree::removeDataRef(L, item->child(row));
    QMetaObject::invokeMethod(item->treeWidget(), [&](){
        QTreeWidgetItem *rmItem = item->takeChild(row);
        if (rmItem) delete rmItem;
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppTreeItem::clear(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    if (!item || !item->treeWidget()) return 0;
    AppTree::removeDataRef(L, item, true);
    QMetaObject::invokeMethod(item->treeWidget(), [&](){
        qDeleteAll(item->takeChildren());
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppTreeItem::get(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    if (!item || lua_gettop(L) < 2)
    {
        lua_pushnil(L);
        return 1;
    }
    const int col = lua_tointeger(L, 2) - 1;
    if (lua_gettop(L) == 2)  //  get(col)
    {
        pushItemCol(L, item, col);
        return 1;
    }
    else  //  get(col, field)
    {
        const QString field = lua_tostring(L, 3);
        pushItemCol(L, item, col, field);
        return 1;
    }
}

int AppTreeItem::set(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    // set(col, field, val)
    if (!item || !item->treeWidget() || lua_gettop(L) != 4) return 0;
    const int col = lua_tointeger(L, 2) - 1;
    const QString field = lua_tostring(L, 3);
    QVariant val;
    const int valType = lua_type(L, 4);
    if (valType != LUA_TFUNCTION && valType != LUA_TTABLE)
    {
        val = getValue(L, true);
    }
    else if (field == "data")
    {
        AppTree *appTree = getAppTree(item);
        val = QVariant::fromValue<Extension::LuaItemRef>({appTree->getDataRef(L, appTree), appTree->dataRef});
    }
    else if (field == "icon")
    {
        if (valType == LUA_TSTRING)
        {
            size_t len = 0;
            const char * s = lua_tolstring(L, -1, &len);
            if (QFile::exists(s))
            {
                val = QIcon(s);
            }
            else
            {
                QByteArray imgBytes(s, len);
                QBuffer bufferImage(&imgBytes);
                bufferImage.open(QIODevice::ReadOnly);
                QImageReader reader(&bufferImage);
                val = QIcon(QPixmap::fromImageReader(&reader));
            }
        }
        else if (valType == LUA_TUSERDATA)
        {
            QImage *img = AppImage::checkImg(L, -1);
            if (img) val = QIcon(QPixmap::fromImage(*img));
        }
    }
    QMetaObject::invokeMethod(item->treeWidget(), [&](){
        setItemCol(item, col, field, val);
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppTreeItem::scrollto(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    if (!item || !item->treeWidget()) return 0;
    QMetaObject::invokeMethod(item->treeWidget(), [&](){
        item->treeWidget()->scrollToItem(item);
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppTreeItem::parent(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    if (!item || !item->treeWidget() || !item->parent())
    {
        lua_pushnil(L);
        return 1;
    }
    AppTreeItem::pushItem(L, item->parent());
    return 1;
}

int AppTreeItem::child(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    // chile(index)
    if (!item || lua_gettop(L) < 2)
    {
        lua_pushnil(L);
        return 1;
    }
    const int index = lua_tointeger(L, 2) - 1;
    if (index < 0 || index >= item->childCount())
    {
        lua_pushnil(L);
        return 1;
    }
    AppTreeItem::pushItem(L, item->child(index));
    return 1;
}

int AppTreeItem::childcount(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    if (!item)
    {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_pushinteger(L, item->childCount());
    return 1;
}

int AppTreeItem::indexof(lua_State *L)
{
    QTreeWidgetItem *item = checkItem(L, 1);
    // indexof(child)
    if (!item || lua_gettop(L) < 2)
    {
        lua_pushinteger(L, -1);
        return 1;
    }
    QTreeWidgetItem *child = AppTreeItem::checkItem(L, 2);
    if (!child)
    {
        lua_pushinteger(L, -1);
        return 1;
    }
    lua_pushinteger(L, item->indexOfChild(child) + 1);
    return 1;
}


}

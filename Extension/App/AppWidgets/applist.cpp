#include "applist.h"
#include <QListWidget>
#include <QScrollBar>
#include <QFile>
#include <QBuffer>
#include <QPixmap>
#include <QIcon>
#include <QImageReader>
#include "Common/logger.h"
#include "Extension/Common/ext_common.h"
#include "Extension/App/kapp.h"
#include "Extension/Modules/lua_appimage.h"

#define ListItemDataRole Qt::UserRole + 1
namespace  Extension
{
const char *AppList::AppWidgetName = "list";
const char *AppList::MetaName = "meta.kiko.widget.list";
const char *AppList::StyleClassName = "QListWidget";

AppWidget *AppList::create(AppWidget *parent, KApp *app)
{
    return new AppList(parent);
}

void AppList::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"append", append},
        {"insert", insert},
        {"remove", remove},
        {"clear", clear},
        {"item", item},
        {"set", set},
        {"setview", setview},
        {"getview", getview},
        {"selection", selection},
        {"scrollto", scrollto},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppList::MetaName, members, AppWidget::MetaName);
}

int AppList::append(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget || lua_gettop(L) != 2) return 0;
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    // append({item1, item2, ... })
    QVector<QListWidgetItem *> items;
    parseItems(L, items, static_cast<AppList *>(appWidget));
    if (items.empty()) return 0;
    QMetaObject::invokeMethod(listWidget, [&](){
        for (QListWidgetItem *item : qAsConst(items))
        {
            listWidget->addItem(item);
        }
    }, Qt::BlockingQueuedConnection);
    lua_pushinteger(L, listWidget->count());
    return 1;
}

int AppList::insert(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget || lua_gettop(L) != 3 || lua_type(L, -2) != LUA_TNUMBER) return 0;
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    // insert(pos, {item1, item2, ... })
    const int row = lua_tointeger(L, 2) - 1;
    QVector<QListWidgetItem *> items;
    parseItems(L, items, static_cast<AppList *>(appWidget));
    if (items.empty()) return 0;
    QMetaObject::invokeMethod(listWidget, [&items, row, listWidget](){
        int r = row;
        for (QListWidgetItem *item : qAsConst(items))
        {
            listWidget->insertItem(r++, item);
        }
    }, Qt::BlockingQueuedConnection);
    lua_pushinteger(L, listWidget->count());
    return 1;
}

int AppList::remove(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget || lua_gettop(L) < 2 || lua_type(L, -1) != LUA_TNUMBER) return 0;
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    // remove(pos)
    const int row = lua_tointeger(L, -1) - 1;
    int dataRef = -1;
    QMetaObject::invokeMethod(listWidget, [row, listWidget, &dataRef](){
       QListWidgetItem *item = listWidget->takeItem(row);
       if (item)
       {
           QVariant data = item->data(ListItemDataRole);
           if (data.userType() == QMetaType::type("Extension::LuaItemRef"))
           {
               dataRef = data.value<Extension::LuaItemRef>().ref;
           }
           delete item;
       }
    }, Qt::BlockingQueuedConnection);
    if (dataRef != -1)
    {
        removeDataRef(L, static_cast<AppList *>(appWidget), dataRef);
    }
    lua_pushinteger(L, listWidget->count());
    return 1;
}

int AppList::clear(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget) return 0;
    AppList *appList = static_cast<AppList *>(appWidget);
    if (appList->dataRef > 0)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, appList->dataRef);
        appList->dataRef = -1;
    }
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    QMetaObject::invokeMethod(listWidget, "clear", Qt::BlockingQueuedConnection);
    return 0;
}

int AppList::item(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget || lua_gettop(L) != 2 || lua_type(L, -1) != LUA_TNUMBER)
    {
        lua_pushnil(L);
        return 1;
    }
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    // item(pos)
    const int row = lua_tointeger(L, 2) - 1;
    QListWidgetItem *item = listWidget->item(row);
    if (!item)
    {
        lua_pushnil(L);
        return 1;
    }
    pushValue(L, itemToMap(item));
    return 1;
}

int AppList::set(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget || lua_gettop(L) != 4 || lua_type(L, 2) != LUA_TNUMBER)
    {
        lua_pushnil(L);
        return 1;
    }
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    // set(pos, key, val)
    const int row = lua_tointeger(L, 2) - 1;
    QListWidgetItem *item = listWidget->item(row);
    if (!item) return 0;
    const QString key = lua_tostring(L, 3);
    QVariant val;
    const int valType = lua_type(L, 4);
    if (valType != LUA_TFUNCTION && valType != LUA_TTABLE && valType != LUA_TUSERDATA)
    {
        val = getValue(L, true);
    }
    else if (key == "data")
    {
        AppList *appList = static_cast<AppList *>(appWidget);
        val = QVariant::fromValue<Extension::LuaItemRef>({getDataRef(L, appList), appList->dataRef});
    }
    else if (key == "icon")
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
    QMetaObject::invokeMethod(listWidget, [&](){
       setItem(item, key, val);
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppList::setview(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget || lua_gettop(L) != 3 || lua_type(L, 2) != LUA_TNUMBER)
    {
        lua_pushnil(L);
        return 1;
    }
    // setview(pos, xml)
    const int row = lua_tointeger(L, 2) - 1;
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    QListWidgetItem *item = listWidget->item(row);
    if (!item)
    {
        lua_pushnil(L);
        return 1;
    }
    if (lua_type(L, 3) == LUA_TNIL)
    {
        QMetaObject::invokeMethod(listWidget, [&](){
            listWidget->removeItemWidget(item);
        }, Qt::BlockingQueuedConnection);
        return 0;
    }
    QString content = lua_tostring(L, 3);
    if (content.isEmpty()) return 0;
    if (QFile::exists(content))  // from file
    {
        QFile xmlFile(content);
        if (!xmlFile.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            Logger::logger()->log(Logger::Extension, QString("[setview]file error: %1").arg(content));
            lua_pushnil(L);
            return 1;
        }
        content = xmlFile.readAll();
    }
    KApp *app = KApp::getApp(L);
    AppWidget *viewWidget = nullptr;
    QString errInfo;
    QMetaObject::invokeMethod(listWidget, [&](){
        viewWidget = AppWidget::parseFromXml(content, app, "", &errInfo);
        if (viewWidget)
        {
            viewWidget->moveToThread(app->getThread());
            viewWidget->getWidget()->setProperty("__appwidget", QVariant::fromValue((AppWidget *)viewWidget));
            listWidget->setItemWidget(item, viewWidget->getWidget());
            item->setSizeHint(viewWidget->getWidget()->sizeHint());
        }
    }, Qt::BlockingQueuedConnection);
    if (!viewWidget)
    {
        Logger::logger()->log(Logger::Extension, QString("[setview]parse error: %1").arg(errInfo));
        lua_pushnil(L);
    }
    else
    {
        QObject::connect(viewWidget->getWidget(), &QObject::destroyed, viewWidget, &QObject::deleteLater);
        viewWidget->setParent(appWidget);
        viewWidget->luaPush(L);
    }
    return 1;
}

int AppList::getview(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget || lua_gettop(L) != 2 || lua_type(L, -1) != LUA_TNUMBER)
    {
        lua_pushnil(L);
        return 1;
    }
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    // getview(pos)
    const int row = lua_tointeger(L, 2) - 1;
    QListWidgetItem *item = listWidget->item(row);
    do
    {
        if (!item) break;
        QWidget *w = listWidget->itemWidget(item);
        if (!w) break;
        QVariant v = w->property("__appwidget");
        if (v.userType() != QMetaType::type("Extension::AppWidget*")) break;
        v.value<Extension::AppWidget*>()->luaPush(L);
        return 1;
    } while (false);
    lua_pushnil(L);
    return 1;
}

int AppList::selection(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget)
    {
        lua_pushnil(L);
        return 1;
    }
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    QList<QListWidgetItem *> selItems;
    QMetaObject::invokeMethod(listWidget, [&](){
        selItems = listWidget->selectedItems();
    }, Qt::BlockingQueuedConnection);
    QVariantList selList;
    for (QListWidgetItem *item : qAsConst(selItems))
    {
        selList.append(itemToMap(item));
    }
    pushValue(L, selList);
    return 1;
}

int AppList::scrollto(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppList::MetaName);
    if (!appWidget || lua_gettop(L) != 2 || lua_type(L, -1) != LUA_TNUMBER) return 0;
    QListWidget *listWidget = static_cast<QListWidget *>(appWidget->getWidget());
    // scrollto(pos)
    const int row = lua_tointeger(L, -1) - 1;
    QMetaObject::invokeMethod(listWidget, [row, listWidget](){
       listWidget->scrollToItem(listWidget->item(row));
    }, Qt::BlockingQueuedConnection);
    return 0;
}

void AppList::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QListWidget *listWidget = static_cast<QListWidget *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_ITEM_CLICK:
    {
        QObject::connect(listWidget, &QListWidget::itemClicked, this, &AppList::onItemClick);
        break;
    }
    case AppEvent::EVENT_ITEM_DOUBLE_CLICK:
    {
        QObject::connect(listWidget, &QListWidget::itemDoubleClicked, this, &AppList::onItemDoubleClick);
        break;
    }
    case AppEvent::EVENT_ITEM_CHANGED:
    {
        QObject::connect(listWidget, &QListWidget::itemChanged, this, &AppList::onItemChanged);
        break;
    }
    case AppEvent::EVENT_SCROLL_EDGE:
    {
        QObject::connect(listWidget->verticalScrollBar(), &QScrollBar::valueChanged, [=](int val){
            if (val == listWidget->verticalScrollBar()->minimum())
            {
                QMetaObject::invokeMethod(this, "onScrollEdge", Qt::QueuedConnection, Q_ARG(bool, false));
            }
            else if (val == listWidget->verticalScrollBar()->maximum())
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

AppList::AppList(AppWidget *parent)
    : AppWidget{parent}, dataRef(-1)
{
    QListWidget *listWidget = new QListWidget(parent? parent->getWidget() : nullptr);
    listWidget->setResizeMode(QListView::Adjust);
    this->widget = listWidget;
}

AppList::~AppList()
{
    if (dataRef > 0 && app && app->getState())
    {
        luaL_unref(app->getState(), LUA_REGISTRYINDEX, dataRef);
    }
}

bool AppList::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QListWidget *listWidget = static_cast<QListWidget *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_LIST_SELECTION_MODE:
    {
        static const QHash<QString, QAbstractItemView::SelectionMode> modeHash = {
            {"none",   QAbstractItemView::SelectionMode::NoSelection},
            {"single", QAbstractItemView::SelectionMode::SingleSelection},
            {"multi",  QAbstractItemView::SelectionMode::MultiSelection},
        };
        if (modeHash.contains(val.toString()))
        {
            listWidget->setSelectionMode(modeHash[val.toString()]);
        }
        break;
    }
    case AppWidgetOption::OPTION_CURRENT_INDEX:
    {
        bool ok = false;
        int row = val.toInt(&ok) - 1;
        if (ok)
        {
            listWidget->setCurrentRow(row);
        }
        break;
    }
    case AppWidgetOption::OPTION_ITEMS:
    {
        listWidget->clear();
        listWidget->addItems(val.toString().split(","));
        break;
    }
    case AppWidgetOption::OPTION_DISABLE_H_SCROLLBAR:
    {
        listWidget->setHorizontalScrollBarPolicy(val.toBool()? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
        break;
    }
    case AppWidgetOption::OPTION_DISABLE_V_SCROLLBAR:
    {
        listWidget->setVerticalScrollBarPolicy(val.toBool()? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
        break;
    }
    case AppWidgetOption::OPTION_WORD_WRAP:
    {
        listWidget->setWordWrap(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_ALTER_ROW_COLOR:
    {
        listWidget->setAlternatingRowColors(val.toBool());
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
        listWidget->setTextElideMode(mode.value(val.toString(), Qt::TextElideMode::ElideRight));
        break;
    }
    case AppWidgetOption::OPTON_VIEW_MODE:
    {
        listWidget->setViewMode(val.toString().toLower() == "icon"? QListView::IconMode : QListView::ListMode);
        listWidget->setMovement(QListView::Static);
        break;
    }
    case AppWidgetOption::OPTION_ICON_SIZE:
    case AppWidgetOption::OPTION_GRID_SIZE:
    {
        const QStringList size = val.toString().split(",", Qt::SkipEmptyParts);
        QSize sz;
        if (size.size() == 2)
        {
            bool wValid = false, hValid = false;
            int w = size[0].toInt(&wValid), h = size[1].toInt(&hValid);
            if (wValid && hValid && w >= 0 && h >= 0)
            {
                sz = QSize(w * listWidget->logicalDpiX() / 96, h  * listWidget->logicalDpiY() / 96);
            }
        }
        if (option == AppWidgetOption::OPTION_ICON_SIZE)
        {
            if (!sz.isEmpty()) listWidget->setIconSize(sz);
        }
        else
        {
            listWidget->setGridSize(sz);
        }
        break;
    }
    case AppWidgetOption::OPTION_IS_UNIFORM_ITEM_SIZE:
    {
        listWidget->setUniformItemSizes(val.toBool());
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppList::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    QListWidget *listWidget = static_cast<QListWidget *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_COUNT:
    {
        return listWidget->count();
    }
    case AppWidgetOption::OPTION_LIST_SELECTION_MODE:
    {
        QAbstractItemView::SelectionMode mode = listWidget->selectionMode();
        static const char *modeType[] = {"none", "single", "multi"};
        return modeType[mode];
    }
    case AppWidgetOption::OPTION_CURRENT_INDEX:
    {
        return listWidget->currentRow() + 1;
    }
    case AppWidgetOption::OPTION_DISABLE_H_SCROLLBAR:
    {
        return listWidget->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
    }
    case AppWidgetOption::OPTION_DISABLE_V_SCROLLBAR:
    {
        return listWidget->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
    }
    case AppWidgetOption::OPTION_H_SCROLLBAR_VISIBLE:
    {
        return listWidget->horizontalScrollBar()->isVisible();
    }
    case AppWidgetOption::OPTION_V_SCROLLBAR_VISIBLE:
    {
        return listWidget->verticalScrollBar()->isVisible();
    }
    case AppWidgetOption::OPTION_WORD_WRAP:
    {
        return listWidget->wordWrap();
    }
    case AppWidgetOption::OPTION_ALTER_ROW_COLOR:
    {
        return listWidget->alternatingRowColors();
    }
    case AppWidgetOption::OPTION_ELIDEMODE:
    {
        Qt::TextElideMode mode = listWidget->textElideMode();
        static const char *modeType[] = {"left", "right", "middle", "none"};
        return modeType[mode];
    }
    case AppWidgetOption::OPTON_VIEW_MODE:
    {
        return listWidget->viewMode() == QListView::IconMode? "icon" : "list";
    }
    case AppWidgetOption::OPTION_ICON_SIZE:
    case AppWidgetOption::OPTION_GRID_SIZE:
    {
        const QSize s = option == AppWidgetOption::OPTION_ICON_SIZE? listWidget->iconSize() : listWidget->gridSize();
        if (s.isEmpty()) return "";
        return QString("%1,%2").arg(s.width() * 96 / listWidget->logicalDpiX()).arg(s.height() * 96 / listWidget->logicalDpiY());
    }
    case AppWidgetOption::OPTION_IS_UNIFORM_ITEM_SIZE:
    {
        return listWidget->uniformItemSizes();
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

int AppList::getDataRef(lua_State *L, AppList *appList)
{
    if (appList->dataRef < 0)
    {
        lua_newtable(L);
        appList->dataRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, appList->dataRef);  // data data_tb
    lua_pushvalue(L, -2);  // data data_tb  data
    const int ref = luaL_ref(L, -2); // data data_tb
    lua_pop(L, 1);
    return ref;
}

void AppList::removeDataRef(lua_State *L, AppList *appList, int ref)
{
    if (appList->dataRef < 0) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, appList->dataRef);  // data_tb
    luaL_unref(L, -1, ref);
    lua_pop(L, 1);
}

void AppList::parseItems(lua_State *L, QVector<QListWidgetItem *> &items, AppList *appList)
{
    auto modifer = [appList](lua_State *L, QVariantMap &map, int level){
        if (level < 2)
        {
            if (map.contains("data"))
            {
                lua_pushstring(L, "data");  // t key
                const int type = lua_gettable(L, -2);
                if (type == LUA_TFUNCTION || type == LUA_TTABLE)
                {
                    const int ref = appList->getDataRef(L, appList);
                    map["data"] = QVariant::fromValue<Extension::LuaItemRef>({ref, appList->dataRef});
                }
                lua_pop(L, 1);
            }
            if (map.contains("icon"))
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
        }
    };
    const QVariant itemParam = getValue(L, true, modifer, 2);
    QVariantList rawItems;
    if (itemParam.typeId() == QMetaType::QVariantList)
    {
        rawItems = itemParam.toList();
    }
    else
    {
        rawItems.append(itemParam);
    }
    if (rawItems.empty()) return;
    for (auto &item : rawItems)
    {
        if (item.canConvert(QMetaType::QVariantMap))
        {
            //  {["text"]=xx, ["tip"]=xx, ["fg"]=0x12345, ["bg"]=0x2222, ["align"]=xx, ["data"]=xx}
            QVariantMap itemInfo = item.toMap();
            QListWidgetItem *listItem = new QListWidgetItem(itemInfo.value("text").toString());
            items.append(listItem);
            for (auto iter = itemInfo.begin(); iter != itemInfo.end(); ++iter)
            {
                setItem(listItem, iter.key(), iter.value());
            }
        }
        else
        {
            QListWidgetItem *listItem = new QListWidgetItem(item.toString());
            items.append(listItem);
        }
    }
}

void AppList::setItem(QListWidgetItem *item, const QString &field, const QVariant &val)
{
    if (field == "text")  {
        item->setText(val.toString());
    } else if (field == "tip") {
        item->setToolTip(val.toString());
    } else if (field == "align") {
        bool ok = false;
        int alignment = val.toInt(&ok);
        if (ok && alignment >= 0)
        {
            item->setTextAlignment(alignment);
        }
    } else if (field == "bg") {
        bool ok = false;
        const uint bgColor = val.toUInt(&ok);
        if (ok)
        {
            item->setBackground(QColor((bgColor >> 16)&0xff, (bgColor >> 8)&0xff, bgColor&0xff, bgColor >> 24));
        }
    } else if (field == "fg") {
        bool ok = false;
        const uint fgColor = val.toUInt(&ok);
        if (ok)
        {
            item->setForeground(QColor((fgColor >> 16)&0xff, (fgColor >> 8)&0xff, fgColor&0xff, fgColor >> 24));
        }
    } else if (field == "check") {
        if (val.toString().toLower() == "none")
        {
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        }
        else
        {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(val.toBool()? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
        }
    } else if (field == "data") {
        item->setData(ListItemDataRole, val);
    } else if (field == "icon") {
        item->setIcon(val.value<QIcon>());
    } else if (field == "edit") {
        item->setFlags(val.toBool()? (item->flags() | Qt::ItemIsEditable) : (item->flags() & ~Qt::ItemIsEditable));
    }
}

QVariantMap AppList::itemToMap(QListWidgetItem *item)
{
    QVariantMap itemMap = {
        {"text", item->text()},
        {"tip", item->toolTip()},
        {"index", item->listWidget()? item->listWidget()->row(item) + 1 : 0},
        {"edit", item->flags().testFlag(Qt::ItemIsEditable)},
    };
    if (item->data(Qt::BackgroundRole).isValid())
    {
        itemMap["bg"] = item->background().color().rgba();
    }
    if (item->data(Qt::ForegroundRole).isValid())
    {
        itemMap["fg"] = item->foreground().color().rgba();
    }
    if (item->data(Qt::CheckStateRole).isValid())
    {
        itemMap["check"] = item->checkState() == Qt::Checked;
    }
    if (item->data(Qt::DecorationRole).isValid())
    {
        QIcon icon = item->icon();
        itemMap["icon"] = icon.pixmap(icon.availableSizes()[0]).toImage();
    }
    if (item->data(ListItemDataRole).isValid())
    {
        itemMap["data"] = item->data(ListItemDataRole);
    }
    if (item->data(Qt::TextAlignmentRole).isValid())
    {
        itemMap["align"] = item->textAlignment();
    }
    return itemMap;
}

void AppList::onItemClick(QListWidgetItem *item)
{
    if (app && item && bindEvents.contains(AppEvent::EVENT_ITEM_CLICK))
    {
        const QVariantMap params = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"item", itemToMap(item)},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_ITEM_CLICK], params);
    }
}

void AppList::onItemDoubleClick(QListWidgetItem *item)
{
    if (app && item && bindEvents.contains(AppEvent::EVENT_ITEM_DOUBLE_CLICK))
    {
        const QVariantMap params = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"item", itemToMap(item)},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_ITEM_DOUBLE_CLICK], params);
    }
}

void AppList::onItemChanged(QListWidgetItem *item)
{
    if (app && item && bindEvents.contains(AppEvent::EVENT_ITEM_CHANGED))
    {
        const QVariantMap params = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"item", itemToMap(item)},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_ITEM_CHANGED], params);
    }
}

void AppList::onScrollEdge(bool bottom)
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

}

#include "appcombo.h"
#include <QComboBox>
#include "Extension/Common/ext_common.h"
#include "Extension/App/kapp.h"
#include "UI/ela/ElaComboBox.h"

namespace Extension
{
const char *AppCombo::AppWidgetName = "combo";
const char *AppCombo::MetaName = "meta.kiko.widget.combo";
const char *AppCombo::StyleClassName = "QComboBox";

AppWidget *AppCombo::create(AppWidget *parent, KApp *app)
{
    return new AppCombo(parent);
}

void AppCombo::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"append", append},
        {"insert", insert},
        {"item", item},
        {"remove", remove},
        {"clear", clear},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppCombo::MetaName, members, AppWidget::MetaName);
}

int AppCombo::append(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppCombo::MetaName);
    if (!appWidget || lua_gettop(L) != 2) return 0;
    QComboBox *combo = static_cast<QComboBox *>(appWidget->getWidget());
    // append({item1, item2, ... })
    QVector<QPair<QString, QVariant>> items;
    parseItems(L, items, static_cast<AppCombo *>(appWidget));
    if (items.empty()) return 0;
    QMetaObject::invokeMethod(combo, [&](){
        for (auto &item : qAsConst(items))
        {
            combo->addItem(item.first, item.second);
        }
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppCombo::insert(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppCombo::MetaName);
    if (!appWidget || lua_gettop(L) != 3 || lua_type(L, -2) != LUA_TNUMBER) return 0;
    QComboBox *combo = static_cast<QComboBox *>(appWidget->getWidget());
    // insert(pos, {item1, item2, ... })
    const int row = lua_tointeger(L, 2) - 1;
    QVector<QPair<QString, QVariant>> items;
    parseItems(L, items, static_cast<AppCombo *>(appWidget));
    if (items.empty()) return 0;
    QMetaObject::invokeMethod(combo, [&items, row, combo](){
        int r = row;
        for (auto &item : qAsConst(items))
        {
            combo->insertItem(r++, item.first, item.second);
        }
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppCombo::item(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppCombo::MetaName);
    if (!appWidget || lua_gettop(L) != 2 || lua_type(L, -1) != LUA_TNUMBER) return 0;
    QComboBox *combo = static_cast<QComboBox *>(appWidget->getWidget());
    // item(index)
    const int index = lua_tointeger(L, -1) - 1;
    const QVariantMap params = {
        {"text", combo->itemText(index)},
        {"data", combo->itemData(index)},
    };
    pushValue(L, params);
    return 1;
}

int AppCombo::remove(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppCombo::MetaName);
    if (!appWidget || lua_gettop(L) != 2 || lua_type(L, -1) != LUA_TNUMBER) return 0;
    QComboBox *combo = static_cast<QComboBox *>(appWidget->getWidget());
    // remove(index)
    const int index = lua_tointeger(L, -1) - 1;
    QVariant data = combo->itemData(index);
    if (data.userType() == QMetaType::type("Extension::LuaItemRef"))
    {
        int ref = data.value<Extension::LuaItemRef>().ref;
        removeDataRef(L, static_cast<AppCombo *>(appWidget), ref);
    }
    QMetaObject::invokeMethod(combo, [index, combo](){
        combo->removeItem(index);
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppCombo::clear(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    if (!appWidget) return 0;
    AppCombo *appCombo = static_cast<AppCombo *>(appWidget);
    if (appCombo->dataRef > 0)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, appCombo->dataRef);
        appCombo->dataRef = -1;
    }
    QComboBox *combo = static_cast<QComboBox *>(appWidget->getWidget());
    QMetaObject::invokeMethod(combo, "clear", Qt::BlockingQueuedConnection);
    return 0;
}

AppCombo::AppCombo(AppWidget *parent) : AppWidget{parent}
{
    widget = new ElaComboBox(parent? parent->getWidget() : nullptr);
}

void AppCombo::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QComboBox *combo = static_cast<QComboBox *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_CURRENT_CHANGED:
    {
        QObject::connect(combo, (void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, this, &AppCombo::onCurrentChanged);
        break;
    }
    case AppEvent::EVENT_TEXT_CHANGED:
    {
        QObject::connect(combo, &QComboBox::editTextChanged, this, &AppCombo::onTextChanged);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

bool AppCombo::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QComboBox *combo = static_cast<QComboBox *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TEXT:
    {
        combo->setCurrentText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_ITEMS:
    {
        combo->clear();
        combo->addItems(val.toString().split(","));
        break;
    }
    case AppWidgetOption::OPTION_CURRENT_INDEX:
    {
        bool ok = false;
        int index = val.toInt(&ok) - 1;
        if (ok)
        {
            combo->setCurrentIndex(index);
        }
        break;
    }
    case AppWidgetOption::OPTION_EDITABLE:
    {
        combo->setEditable(val.toBool());
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppCombo::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ) *succ = true;
    QComboBox *combo = static_cast<QComboBox *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_COUNT:
    {
        return combo->count();
    }
    case AppWidgetOption::OPTION_TEXT:
    {
        return combo->currentText();
    }
    case AppWidgetOption::OPTION_CURRENT_INDEX:
    {
        return combo->currentIndex() + 1;
    }
    case AppWidgetOption::OPTION_EDITABLE:
    {
        return combo->isEditable();
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

int AppCombo::getDataRef(lua_State *L, AppCombo *appCombo)
{
    if (appCombo->dataRef < 0)
    {
        lua_newtable(L);
        appCombo->dataRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, appCombo->dataRef);  // data data_tb
    lua_pushvalue(L, -2);  // data data_tb  data
    const int ref = luaL_ref(L, -2); // data data_tb
    lua_pop(L, 1);
    return ref;
}

void AppCombo::removeDataRef(lua_State *L, AppCombo *appCombo, int ref)
{
    if (appCombo->dataRef < 0) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, appCombo->dataRef);  // data_tb
    luaL_unref(L, -1, ref);
    lua_pop(L, 1);
}

void AppCombo::parseItems(lua_State *L, QVector<QPair<QString, QVariant> > &items, AppCombo *appCombo)
{
    auto modifer = [appCombo](lua_State *L, QVariantMap &map, int level){
        if (level < 2 && map.contains("data"))
        {
            lua_pushstring(L, "data");  // t key
            const int type = lua_gettable(L, -2);
            if (type == LUA_TFUNCTION || type == LUA_TTABLE)
            {
                const int ref = appCombo->getDataRef(L, appCombo);
                map["data"] = QVariant::fromValue<Extension::LuaItemRef>({ref, appCombo->dataRef});
            }
            lua_pop(L, 1);
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
            //  {["text"]=xx,  ["data"]=xx}
            QVariantMap itemInfo = item.toMap();
            QPair<QString, QVariant> comboItem;
            comboItem.first = itemInfo.value("text").toString();
            if (itemInfo.contains("data"))
            {
                comboItem.second = itemInfo["data"];
            }
            items.append(std::move(comboItem));
        }
        else
        {
            items.append({item.toString(), QVariant()});
        }
    }
}

void AppCombo::onCurrentChanged(int index)
{
    if (app && bindEvents.contains(AppEvent::EVENT_CURRENT_CHANGED))
    {
        QComboBox *combo = static_cast<QComboBox *>(this->widget);
        const QVariantMap params = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"index", index + 1},
            {"text", combo->itemText(index)},
            {"data", combo->itemData(index)},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_CURRENT_CHANGED], params);
    }
}

void AppCombo::onTextChanged(const QString &text)
{
    if (app && bindEvents.contains(AppEvent::EVENT_TEXT_CHANGED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"text", text}
        };
        app->eventCall(bindEvents[AppEvent::EVENT_TEXT_CHANGED], param);
    }
}

}

#include "appwidget.h"
#include <QFile>
#include <QThread>
#include <QWidget>
#include <QVariant>
#include <QXmlStreamReader>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStackedLayout>
#include <QAction>
#include "Extension/Common/ext_common.h"
#include "Common/logger.h"
#include "Extension/App/kapp.h"
#include "UI/ela/ElaMenu.h"
#include "UI/stylemanager.h"

namespace Extension
{

QSharedPointer<QHash<QString, AppWidget::CreateFunc>> AppWidget::appCreateHash;
QSharedPointer<QHash<QString, AppWidget::EnvRegistFunc>> AppWidget::envRegistHash;
QSharedPointer<QHash<QString, QString>> AppWidget::styleClassHash;

const char *AppWidget::MetaName = "meta.kiko.widget";
const char *AppButtonGroupRes::resKey = "buttonGroup";

void AppWidget::registAppWidget(const QString &name, const CreateFunc &func, const QString &metaname,  const EnvRegistFunc &regFunc)
{
    if (!appCreateHash)
    {
        appCreateHash = QSharedPointer<QHash<QString, AppWidget::CreateFunc>>::create();
    }
    if (!envRegistHash)
    {
        envRegistHash = QSharedPointer<QHash<QString, AppWidget::EnvRegistFunc>>::create();
    }
    appCreateHash->insert(name, func);
    envRegistHash->insert(metaname, regFunc);
}

void AppWidget::setStyleClass(const QString &name, const QString &styleClass)
{
    if (!styleClassHash)
    {
        styleClassHash = QSharedPointer<QHash<QString, QString>>::create();
    }
    if (!styleClass.isEmpty())
    {
        styleClassHash->insert(name, styleClass);
    }
}

AppWidget *AppWidget::create(const QString &name, AppWidget *parent, KApp *app)
{
    auto iter = appCreateHash->find(name);
    if (iter == appCreateHash->end()) return nullptr;
    AppWidget *w = iter.value()(parent, app);
    w->app = app;
    return w;
}

AppWidget *AppWidget::parseFromXml(const QString &data, KApp *app, const QString &specificRoot, QString *errInfo, AppWidget *p)
{
    QXmlStreamReader reader(data);
    QVector<AppWidget *> parents;
    AppWidget *rootWidget = nullptr;
    QHash<AppWidgetLayoutDependOption, QVariant> layoutDependParams;
    bool hasError = false;
    reader.addExtraNamespaceDeclarations({
        {"view-depend", "http://www.kikoplay.fun/view-depend"},
        {"data", "http://www.kikoplay.fun/data"},
        {"event", "http://www.kikoplay.fun/event"}
    });
    while (!reader.atEnd())
    {
        if (reader.isStartElement())
        {
            if (parents.isEmpty())
            {
                if (!specificRoot.isEmpty() && reader.name() != specificRoot)
                {
                    return nullptr;
                }
            }
            layoutDependParams.clear();
            AppWidget *parent = parents.isEmpty()? (p? p : nullptr) : parents.last();
            AppWidget *currentWidget = nullptr;
            bool isInclude = false;
            const QXmlStreamAttributes attributes = reader.attributes();
            if (reader.name() == QLatin1StringView("menu"))
            {
                QVector<QAction *> actions = parseMenu(reader, parent);
                if (parent) parent->addActions(actions);
                continue;
            }
            else if (reader.name() == QLatin1StringView("include"))
            {
                const QString includeFile = reader.readElementText().trimmed();
                QFileInfo info(app->path(), includeFile);
                if (!info.exists())
                {
                    if (errInfo) *errInfo = QString("include file not exist: %1").arg(info.absoluteFilePath());
                    hasError = true;
                    break;
                }
                isInclude = true;
                currentWidget = AppWidget::parseFromXmlFile(info.absoluteFilePath(), app, "", errInfo, parent);
            }
            else
            {
                currentWidget = AppWidget::create(reader.name().toString(), parent, app);
            }
            if (!currentWidget)
            {
                hasError = true;
                break;
            }
            for (auto &attr : attributes)
            {
                if (attr.prefix() == QLatin1StringView("view-depend"))
                {
                    AppWidgetLayoutDependOption layouDependtOpt = getWidgetLayoutDependOptionType(attr.name().toString());
                    if (layouDependtOpt != AppWidgetLayoutDependOption::LAYOUT_DEPEND_UNKNOWN)
                    {
                        layoutDependParams.insert(layouDependtOpt, attr.value().toString());
                    }
                }
                else if (attr.prefix() == QLatin1StringView("event"))
                {
                    AppEvent event = getWidgetEventType(attr.name().toString());
                    if (event != AppEvent::EVENT_UNKNOWN)
                    {
                        currentWidget->bindEvent(event, attr.value().toString());
                    }
                }
                else if (attr.prefix() == QLatin1StringView("data"))
                {
                    currentWidget->setProperty(attr.name().toString().toUtf8().constData(), attr.value().toString());
                }
                else
                {
                    AppWidgetOption option = getWidgetOptionType(attr.name().toString());
                    if (option != AppWidgetOption::OPTION_UNKNOWN)
                    {
                        currentWidget->setOption(option, QVariant(attr.value().toString()));
                    }
                }
            }
            if (parents.isEmpty())
            {
                rootWidget = currentWidget;
            }
            if (parent)
            {
                parent->updateChildLayout(currentWidget, layoutDependParams);
            }
            if (!isInclude)  // include node has reached end_element
            {
                parents.append(currentWidget);
            }
        }
        else if (reader.isEndElement())
        {
            if (parents.isEmpty())
            {
                hasError = true;
                break;
            }
            parents.pop_back();
        }
        reader.readNext();
    }
    if (reader.hasError())
    {
        if (errInfo) *errInfo = QString("Line: %1, Col: %2, Error: %3").arg(reader.lineNumber()).arg(reader.columnNumber()).arg(reader.errorString());
        hasError = true;
    }
    if (hasError)
    {
        if (rootWidget) delete rootWidget;
        return nullptr;
    }
    return rootWidget;
}

QVector<QAction *> AppWidget::parseMenu(QXmlStreamReader &reader, AppWidget *parent)
{
    QVector<QAction *> actions;
    while (!reader.atEnd())
    {
        if (reader.isStartElement() && reader.name() == QLatin1StringView("item"))
        {
            QString itemTitle, itemId, itemClickCb;
            bool isSep = false;
            QVariantMap itemData;
            const QXmlStreamAttributes attributes = reader.attributes();
            for (auto &attr : attributes)
            {
                if (attr.prefix() == QLatin1StringView("event"))
                {
                    if (attr.name() == QLatin1StringView("click"))
                    {
                        itemClickCb = attr.value().toString();
                    }
                }
                else if (attr.prefix() == QLatin1StringView("data"))
                {
                    itemData[attr.name().toString()] = attr.value().toString();
                }
                else
                {
                    if (attr.name() == QLatin1StringView("title"))
                    {
                        itemTitle = attr.value().toString();
                    }
                    else if (attr.name() == QLatin1StringView("id"))
                    {
                        itemId = attr.value().toString();
                    }
                    else if (attr.name() == QLatin1StringView("is_sep"))
                    {
                        isSep = (attr.value() == QLatin1StringView("true"));
                    }
                }
            }
            if (parent && parent->widget)
            {
                QAction *menuItemAction = new QAction(itemTitle, parent->widget);
                QObject::connect(menuItemAction, &QAction::triggered, parent, [=](){
                    const QVariantMap param = {
                        {"id", itemId},
                        {"data", itemData},
                        {"srcId", parent->objectName()},
                        {"src", QVariant::fromValue((AppWidget *)parent)}
                    };
                    parent->app->eventCall(itemClickCb, param);
                });
                menuItemAction->setSeparator(isSep);
                actions.append(menuItemAction);
            }
        }
        else if (reader.isEndElement())
        {
            if (reader.name() == QLatin1StringView("menu"))
            {
                reader.readNext();
                break;
            }
        }
        reader.readNext();
    }
    return actions;
}

AppWidget *AppWidget::parseFromXmlFile(const QString &filename, KApp *app, const QString &specificRoot, QString *errInfo, AppWidget *parent)
{
    QFile uiFile(filename);
    bool ret = uiFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if(!ret)
    {
        if (errInfo) *errInfo = QString("open file failed: %1").arg(filename);
        return nullptr;
    }
    return parseFromXml(uiFile.readAll(), app, specificRoot, errInfo, parent);
}

AppWidget *AppWidget::checkWidget(lua_State *L, int pos, const char *metaName)
{
    void *p = lua_touserdata(L, pos);
    if (!p) return nullptr;
    const int objType = luaL_getmetafield(L, pos, "__name");
    if (objType == LUA_TNIL) return nullptr;
    bool isChild = false;
    if (objType == LUA_TSTRING)
    {
        const QString objMeta = lua_tostring(L, -1);
        isChild = objMeta.startsWith(metaName, Qt::CaseInsensitive);
    }
    lua_pop(L, 1);
    return isChild? *static_cast<AppWidget **>(p) : nullptr;
}

void AppWidget::registClassFuncs(lua_State *L, const char *metaName, const luaL_Reg *funcs, const char *pMetaname)
{
    if(!L) return;
    luaL_newmetatable(L, metaName);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); // pushes the metatable
    lua_rawset(L, -3); // metatable.__index = metatable
    if (pMetaname)
    {
        luaL_getmetatable(L, pMetaname);
        lua_setmetatable(L, -2);  // reader meta
    }
    luaL_setfuncs(L, funcs, 0);
    lua_pop(L, 1);
}

void AppWidget::registEnv(lua_State *L)
{
    const luaL_Reg widgetMembers[] = {
        {"getopt", getopt},
        {"setopt", setopt},
        {"data", data},
        {"setstyle", setstyle},
        {"addchild", addchild},
        {"getchild", getchild},
        {"removechild", removechild},
        {"parent", widgetparent},
        {"onevent", onevent},
        {"adjustsize", adjustsize},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppWidget::MetaName, widgetMembers);

    QVector<QPair<QString, EnvRegistFunc>> widgetRegistFuns;
    for (auto iter = envRegistHash->begin(); iter != envRegistHash->end(); ++iter)
    {
        widgetRegistFuns.append({iter.key(), iter.value()});
    }
    using FuncType = QPair<QString, EnvRegistFunc>;
    std::sort(widgetRegistFuns.begin(), widgetRegistFuns.end(), [](const FuncType &f1, const FuncType &f2){return f1.first < f2.first;});
    for (auto &f : widgetRegistFuns)
    {
        f.second(L);
    }
}

int AppWidget::getopt(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    do
    {
        if(!appWidget || lua_gettop(L) == 1 || lua_type(L, 2) != LUA_TSTRING) break;
        const char *optName = lua_tostring(L, 2);
        AppWidgetOption option = getWidgetOptionType(optName);
        if (option == AppWidgetOption::OPTION_UNKNOWN) break;
        bool succ = false;
        QVariant opt = appWidget->getOption(option, &succ);
        if (!succ) break;
        pushValue(L, opt);
        return 1;
    } while (false);
    lua_pushnil(L);
    return 1;
}

int AppWidget::setopt(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    do
    {
        if(!appWidget || lua_gettop(L) < 3 || lua_type(L, 2) != LUA_TSTRING) break;
        const char *optName = lua_tostring(L, 2);
        AppWidgetOption option = getWidgetOptionType(optName);
        if (option == AppWidgetOption::OPTION_UNKNOWN) break;
        QVariant optVal = getValue(L, false);
        bool ret = appWidget->setOption(option, optVal);
        lua_pushboolean(L, ret);
        return 1;
    } while (false);
    lua_pushboolean(L, 0);
    return 1;
}

int AppWidget::data(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    if(!appWidget || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    const QVariant d = appWidget->property(lua_tostring(L, 2));
    if (d.isValid())
    {
        pushValue(L, d);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

int AppWidget::setstyle(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    KApp *app = KApp::getApp(L);
    if(!app || !appWidget || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING) return 0;
    QString qss = lua_tostring(L, 2);
    if (QFile::exists(qss))
    {
        QFile qssFile(qss);
        if (!qssFile.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            return 0;
        }
        qss = qssFile.readAll();
    }
    QVariantMap extraVals {
        {"AppPath", app->path()},
        {"AppDataPath", app->dataPath()},
        //{"StyleBGMode", StyleManager::getStyleManager()->currentMode() == StyleManager::StyleMode::BG_COLOR},
    };
    if (lua_gettop(L) == 3 || lua_type(L, 3) == LUA_TTABLE)
    {
        extraVals.insert(getValue(L, true, 2).toMap());
    }
    QMetaObject::invokeMethod(appWidget->widget, [appWidget, qss, extraVals](){
        appWidget->setStyle(qss, &extraVals);
    }, Qt::QueuedConnection);
    return 0;
}

int AppWidget::addchild(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    if(!appWidget || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING) return 0;
    QString content = lua_tostring(L, 2);
    if (lua_gettop(L) > 2 &&lua_type(L, 3) != LUA_TBOOLEAN)  // from file
    {
        if (lua_toboolean(L, 3))
        {
            QFile xmlFile(content);
            if (!xmlFile.open(QIODevice::ReadOnly|QIODevice::Text))
            {
                Logger::logger()->log(Logger::Extension, QString("[addchild]file error: %1").arg(content));
                lua_pushnil(L);
                return 1;
            }
            content = xmlFile.readAll();
        }
    }
    KApp *app = KApp::getApp(L);
    AppWidget *childWidget = nullptr;
    QString errInfo;
    QMetaObject::invokeMethod(appWidget->widget, [app, appWidget, &content, &childWidget, &errInfo](){
        childWidget = AppWidget::parseFromXml(content, app, "", &errInfo, appWidget);
        if (childWidget)
        {
            childWidget->moveToThread(app->getThread());
        }
    }, Qt::BlockingQueuedConnection);
    if (!childWidget)
    {
        Logger::logger()->log(Logger::Extension, QString("[addchild]parse error: %1").arg(errInfo));
        lua_pushnil(L);
    }
    else
    {
        childWidget->setParent(appWidget);
        childWidget->luaPush(L);
    }
    return 1;
}

int AppWidget::removechild(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    if(!appWidget || lua_gettop(L) < 2) return 0;
    AppWidget *child = nullptr;
    if (lua_type(L, 2) == LUA_TSTRING)
    {
        const QString childName = lua_tostring(L, 2);
        child = appWidget->findChild<AppWidget *>(childName);
    }
    else
    {
        child = checkWidget(L, 2, AppWidget::MetaName);
        if (child)
        {
            QObject *tmp = child->parent();
            while(tmp && tmp != appWidget)
            {
                tmp = tmp->parent();
            }
            if (tmp != appWidget)
            {
                child = nullptr;
            }
        }
        if (child)
        {
            lua_pushnil(L);
            lua_setmetatable(L, 2);
        }
    }
    if (child)
    {
        child->deleteLater();
        if (child->widget) child->widget->deleteLater();
    }
    return 0;
}

int AppWidget::getchild(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    if(!appWidget || lua_gettop(L) < 2)
    {
        lua_pushnil(L);
        return 1;
    }
    const QString name = lua_tostring(L, 2);
    AppWidget *child = appWidget->findChild<AppWidget *>(name);
    if (child) child->luaPush(L);
    else lua_pushnil(L);
    return 1;
}

int AppWidget::widgetparent(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    if(!appWidget || lua_gettop(L) > 1) return 0;
    AppWidget *p = dynamic_cast<AppWidget *>(appWidget->parent());
    if (!p) lua_pushnil(L);
    else p->luaPush(L);
    return 1;
}

int AppWidget::onevent(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    if(!appWidget || lua_gettop(L) != 3 || lua_type(L, 2) != LUA_TSTRING) return 0;
    QString event = lua_tostring(L, 2);
    if (event.startsWith("event:")) event = event.mid(6);
    AppEvent e = getWidgetEventType(event);
    if (e != AppEvent::EVENT_UNKNOWN)
    {
        QString eventCb;
        if (lua_type(L, 3) == LUA_TFUNCTION)
        {
            eventCb = QString::number(luaL_ref(L, LUA_REGISTRYINDEX));
        }
        else
        {
            eventCb = lua_tostring(L, 3);
        }
        appWidget->bindEvent(e, eventCb);
    }
    return 0;
}

int AppWidget::adjustsize(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWidget::MetaName);
    if(!appWidget || !appWidget->widget) return 0;
    QMetaObject::invokeMethod(appWidget->widget, [appWidget](){
        appWidget->widget->adjustSize();
    }, Qt::QueuedConnection);
    return 0;
}

AppWidget::~AppWidget()
{
    for (const QString &e : bindEvents)
    {
        bool isRef = false;
        int ref = e.toInt(&isRef);
        if (isRef && app && app->getState())
        {
            luaL_unref(app->getState(), LUA_REGISTRYINDEX, ref);
        }
    }
}

void AppWidget::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (event != AppEvent::EVENT_UNKNOWN)
    {
        if (bindEvents.contains(event))
        {
            bool isRef = false;
            const int ref = bindEvents[event].toInt(&isRef);
            if (isRef && app && app->getState())
            {
                luaL_unref(app->getState(), LUA_REGISTRYINDEX, ref);
            }
        }
        bindEvents.insert(event, luaFunc);
    }
}

void AppWidget::luaPush(lua_State *L)
{
    AppWidget **appWidget = (AppWidget **)lua_newuserdata(L, sizeof(AppWidget *));
    luaL_getmetatable(L, this->getMetaname());
    lua_setmetatable(L, -2);  // widget meta
    *appWidget = this;  // widget
}

bool AppWidget::setOption(AppWidgetOption option, const QVariant &val)
{
    if (!widget) return false;
    if (QThread::currentThread() == widget->thread())
    {
        return setWidgetOption(option, val);
    }
    else
    {
        QMetaObject::invokeMethod(widget, [val, option, this](){
            this->setWidgetOption(option, val);
        }, Qt::QueuedConnection);
        return true;
    }
}

QVariant AppWidget::getOption(AppWidgetOption option, bool *succ)
{
    if (!widget)
    {
        if (succ) *succ = false;
        return QVariant();
    }
    if (QThread::currentThread() == widget->thread())
    {
        return getWidgetOption(option, succ);
    }
    else
    {
        QVariant ret;
        QMetaObject::invokeMethod(widget, [&ret, succ, option, this](){
            ret = this->getWidgetOption(option, succ);
        }, Qt::BlockingQueuedConnection);
        return ret;
    }
}

void AppWidget::setStyle(const QString &qss, const QVariantMap *extraVals)
{
    if (!widget) return;
    // remove comments
    QString nQss;
    int state = 0;  // 0: out of comment  1: in comment  2:  /   3:  *
    for (const QChar c : qss)
    {
        if (state == 0)
        {
            if (c == '/') state = 2;
            else nQss.append(c);
        }
        else if (state == 1)
        {
            if (c == '*') state = 3;
        }
        else if (state == 2)
        {
            if (c == '*') state = 1;
            else if (c != '/')
            {
                state = 0;
                nQss.append('/');
                nQss.append(c);
            }
        }
        else if (state == 3)
        {
            if (c == '/') state = 0;
            else if (c != '*') state = 1;
        }
    }
    nQss = StyleManager::getStyleManager()->setQSS(nQss, extraVals);
    bool inSelector = true;
    QString curClass, newQss;
    int selectorState = 1;   //  0: default  1: in class selector  2: property
    for(const QChar c: nQss)
    {
        if (inSelector)
        {
            switch (selectorState)
            {
            case 0:
                if (c.isSpace() || c == '.' || c == ',')
                {
                    selectorState = 1;
                }
                else if (c == '{')
                {
                    inSelector = false;
                }
                else if (c == '[')
                {
                    selectorState = 2;
                }
                newQss.append(c);
                break;
            case 1:
                if (c.isSpace() || c == '#' || c == '[' || c == ',' || c == '>' || c == ':' || c == '{')
                {
                    if (!curClass.isEmpty())
                    {
                        auto iter = styleClassHash->find(curClass);
                        newQss.append(iter == styleClassHash->end()? curClass : iter.value());
                        curClass.clear();
                    }
                    if (c == '#' || c == ':')
                    {
                        selectorState = 0;
                    }
                    if (c == '[')
                    {
                        selectorState = 2;
                    }
                    if ( c == '{')
                    {
                        inSelector = false;
                    }
                    newQss.append(c);
                }
                else if (c == '.')
                {
                    newQss.append(c);
                }
                else
                {
                    curClass.append(c);
                }
                break;
            case 2:
                if (c == ']')
                {
                    selectorState = 1;
                }
                newQss.append(c);
            default:
                break;
            }
        }
        else
        {
            newQss.append(c);
            if (c == '}')
            {
                inSelector = true;
            }
        }
    }
    widget->setStyleSheet(newQss);
}

AppWidget::AppWidget(QObject *parent) : QObject(parent), widget(nullptr), app(nullptr)
{

}

bool AppWidget::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    switch (option)
    {
    case AppWidgetOption::OPTION_ID:
    {
        this->widget->setObjectName(val.toString());
        this->setObjectName(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_TOOLTIP:
    {
        this->widget->setToolTip(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_X:
    {
        bool ok = false;
        int x = val.toInt(&ok);
        if (ok)
        {
            this->widget->move(x, this->widget->y());
        }
        break;
    }
    case AppWidgetOption::OPTION_Y:
    {
        bool ok = false;
        int y = val.toInt(&ok);
        if (ok)
        {
            this->widget->move(this->widget->x(), y);
        }
        break;
    }
    case AppWidgetOption::OPTION_WIDTH:
    {
        bool ok = false;
        int w = val.toInt(&ok);
        if (ok)
        {
            this->widget->resize(w * widget->logicalDpiX() / 96, this->widget->height());
        }
        break;
    }
    case AppWidgetOption::OPTION_HEIGHT:
    {
        bool ok = false;
        int h = val.toInt(&ok);
        if (ok)
        {
            this->widget->resize(this->widget->width(), h * widget->logicalDpiY() / 96);
        }
        break;
    }
    case AppWidgetOption::OPTION_MAX_WIDTH:
    {
        bool ok = false;
        int w = val.toInt(&ok);
        if (ok)
        {
            this->widget->setMaximumWidth(w * widget->logicalDpiX() / 96);
        }
        break;
    }
    case AppWidgetOption::OPTION_MIN_WIDTH:
    {
        bool ok = false;
        int w = val.toInt(&ok);
        if (ok)
        {
            this->widget->setMinimumWidth(w * widget->logicalDpiX() / 96);
        }
        break;
    }
    case AppWidgetOption::OPTION_MAX_HEIGHT:
    {
        bool ok = false;
        int h = val.toInt(&ok);
        if (ok)
        {
            this->widget->setMaximumHeight(h * widget->logicalDpiY() / 96);
        }
        break;
    }
    case AppWidgetOption::OPTION_MIN_HEIGHT:
    {
        bool ok = false;
        int h = val.toInt(&ok);
        if (ok)
        {
            this->widget->setMinimumHeight(h * widget->logicalDpiY() / 96);
        }
        break;
    }
    case AppWidgetOption::OPTION_VISIBLE:
    {
        bool visible = val.toBool();
        this->widget->setVisible(visible);
        break;
    }
    case AppWidgetOption::OPTION_ENABLE:
    {
        bool enable = val.toBool();
        this->widget->setEnabled(enable);
        break;
    }
    case AppWidgetOption::OPTION_H_SIZE_POLICY:
    case AppWidgetOption::OPTION_V_SIZE_POLICY:
    {
        static const QHash<QString, QSizePolicy::Policy> policyHash = {
            {"fix", QSizePolicy::Policy::Fixed},
            {"min", QSizePolicy::Policy::Minimum},
            {"max", QSizePolicy::Policy::Maximum},
            {"prefer", QSizePolicy::Policy::Preferred},
            {"expand", QSizePolicy::Policy::Expanding},
            {"min_expand", QSizePolicy::Policy::MinimumExpanding},
            {"ignore", QSizePolicy::Policy::Ignored},
        };
        if (policyHash.contains(val.toString()))
        {
            QSizePolicy policy = this->widget->sizePolicy();
            if (option == AppWidgetOption::OPTION_H_SIZE_POLICY)
            {
                policy.setHorizontalPolicy(policyHash[val.toString()]);
            }
            else
            {
                policy.setVerticalPolicy(policyHash[val.toString()]);
            }
            this->widget->setSizePolicy(policy);
        }
        break;
    }
    default:
        return false;
    }

    return true;
}

QVariant AppWidget::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    switch (option)
    {
    case AppWidgetOption::OPTION_ID:
    {
        return this->widget->objectName();
    }
    case AppWidgetOption::OPTION_TOOLTIP:
    {
        return this->widget->toolTip();
    }
    case AppWidgetOption::OPTION_X:
    {
        return this->widget->x();
    }
    case AppWidgetOption::OPTION_Y:
    {
        return this->widget->y();
    }
    case AppWidgetOption::OPTION_WIDTH:
    {
        return this->widget->width() * 96 / widget->logicalDpiX();
    }
    case AppWidgetOption::OPTION_HEIGHT:
    {
        return this->widget->height() * 96 / widget->logicalDpiY();
    }
    case AppWidgetOption::OPTION_MAX_WIDTH:
    {
        return this->widget->maximumWidth() * 96 / widget->logicalDpiX();
    }
    case AppWidgetOption::OPTION_MIN_WIDTH:
    {
        return this->widget->minimumWidth() * 96 / widget->logicalDpiX();
    }
    case AppWidgetOption::OPTION_MAX_HEIGHT:
    {
        return this->widget->maximumHeight() * 96 / widget->logicalDpiY();
    }
    case AppWidgetOption::OPTION_MIN_HEIGHT:
    {
        return this->widget->minimumHeight() * 96 / widget->logicalDpiY();
    }
    case AppWidgetOption::OPTION_VISIBLE:
    {
        return this->widget->isVisible();
    }
    case AppWidgetOption::OPTION_ENABLE:
    {
        return this->widget->isEnabled();
    }
    case AppWidgetOption::OPTION_H_SIZE_POLICY:
    case AppWidgetOption::OPTION_V_SIZE_POLICY:
    {
        static const QHash<QSizePolicy::Policy, QString> policyHash = {
            {QSizePolicy::Policy::Fixed, "fix"},
            {QSizePolicy::Policy::Minimum, "min"},
            {QSizePolicy::Policy::Maximum, "max"},
            {QSizePolicy::Policy::Preferred, "prefer"},
            {QSizePolicy::Policy::Expanding, "expand"},
            {QSizePolicy::Policy::MinimumExpanding, "min_expand"},
            {QSizePolicy::Policy::Ignored, "ignore"},
        };
        const QSizePolicy policy = this->widget->sizePolicy();
        return policyHash[option == AppWidgetOption::OPTION_H_SIZE_POLICY ? policy.horizontalPolicy() : policy.verticalPolicy()];
    }
    default:
        break;
    }
    if (succ)
    {
        *succ = false;
    }
    return QVariant();
}

void AppWidget::addActions(const QVector<QAction *> &actions)
{
    if (!widget) return;
    widget->setContextMenuPolicy(Qt::CustomContextMenu);

    ElaMenu *menu = new ElaMenu(widget);
    menu->addActions(actions);

    QObject::connect(widget, &QWidget::customContextMenuRequested, widget, [=](const QPoint &pos){
        menu->exec(QCursor::pos());
    });
}

AppButtonGroupRes *AppButtonGroupRes::getButtonRes(KApp *app)
{
    AppRes *res = app->getRes(AppButtonGroupRes::resKey);
    AppButtonGroupRes *s = nullptr;
    if (!res)
    {
        s = new AppButtonGroupRes();
        app->addRes(AppButtonGroupRes::resKey, s);
    }
    else
    {
        s = static_cast<AppButtonGroupRes *>(res);
    }
    return s;
}

}

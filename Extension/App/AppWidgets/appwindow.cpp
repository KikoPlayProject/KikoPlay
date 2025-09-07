#include "appwindow.h"
#include "Extension/App/appframelessdialog.h"
#include "Extension/App/kapp.h"
#include <QVBoxLayout>
#include <QIcon>
#include "Common/notifier.h"

namespace Extension
{

const char *AppWindow::AppWidgetName = "window";
const char *AppWindow::MetaName = "meta.kiko.widget.window";
const char *AppWindow::StyleClassName = "AppFramelessDialog";

AppWidget *AppWindow::create(AppWidget *parent, KApp *app)
{
    AppWindow *w = new AppWindow(parent);
    static_cast<AppFramelessDialog *>(w->getWidget())->setWindowIcon(app->icon());
    return w;
}

void AppWindow::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"show", show},
        {"raise", raise},
        {"message", message},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppWindow::MetaName, members, AppWidget::MetaName);
}

int AppWindow::show(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWindow::MetaName);
    if (!appWidget) return 0;
    AppFramelessDialog *w = static_cast<AppFramelessDialog *>(appWidget->getWidget());
    QMetaObject::invokeMethod(w, "show", Qt::QueuedConnection);
    return 0;
}

int AppWindow::raise(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWindow::MetaName);
    if (!appWidget) return 0;
    AppFramelessDialog *w = static_cast<AppFramelessDialog *>(appWidget->getWidget());
    QMetaObject::invokeMethod(w, "raise", Qt::QueuedConnection);
    return 0;
}

int AppWindow::message(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppWindow::MetaName);
    if (!appWidget) return 0;
    AppFramelessDialog *w = static_cast<AppFramelessDialog *>(appWidget->getWidget());
    const int params = lua_gettop(L);
    if(params < 2 || lua_type(L, 2) != LUA_TSTRING) return 0;
    const QString msg(lua_tostring(L, 2));
    int flags = NotifyMessageFlag::NM_HIDE;
    if(params > 2 && lua_type(L, 3) == LUA_TNUMBER)
    {
        flags = lua_tonumber(L, 3);
    }
    QMetaObject::invokeMethod(w, [=](){
        w->showMessage(msg, flags);
    }, Qt::QueuedConnection);
    return 0;
}

void AppWindow::bindEvent(AppEvent event, const QString &luaFunc)
{

}

AppWindow::AppWindow(AppWidget *parent)
    : AppWidget{parent}
{
    widget = new AppFramelessDialog("App", parent? parent->getWidget() : nullptr);
    widget->setLayout(new QVBoxLayout);
}

bool AppWindow::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    AppFramelessDialog *w = static_cast<AppFramelessDialog *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        w->setTitle(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_PINNED:
    {
        w->setPin(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_CONTENT_MARGIN:
    {
        const QStringList marginStrs = val.toString().split(",");
        if (marginStrs.size() == 4)
        {
            QMargins margins = widget->contentsMargins();
            bool ok = false;
            if (!marginStrs[0].isEmpty())
            {
                const int left = marginStrs[0].toInt(&ok);
                if (ok) margins.setLeft(left * widget->logicalDpiX() / 96);
            }
            if (!marginStrs[1].isEmpty())
            {
                const int top = marginStrs[1].toInt(&ok);
                if (ok) margins.setTop(top * widget->logicalDpiY() / 96);
            }
            if (!marginStrs[2].isEmpty())
            {
                const int right = marginStrs[2].toInt(&ok);
                if (ok) margins.setRight(right * widget->logicalDpiX() / 96);
            }
            if (!marginStrs[3].isEmpty())
            {
                const int bottom = marginStrs[3].toInt(&ok);
                if (ok) margins.setBottom(bottom * widget->logicalDpiY() / 96);
            }
            widget->layout()->setContentsMargins(margins);
        }
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppWindow::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    AppFramelessDialog *w = static_cast<AppFramelessDialog *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        return w->windowTitle();
    }
    case AppWidgetOption::OPTION_CONTENT_MARGIN:
    {
        const QMargins margins = widget->layout()->contentsMargins();
        return QString("%1,%2,%3,%4").arg(margins.left() * 96 / widget->logicalDpiX()).
                arg(margins.top() * 96 / widget->logicalDpiY()).
                arg(margins.right() * 96 / widget->logicalDpiX()).
                arg(margins.bottom() * 96 / widget->logicalDpiY());
    }
    case AppWidgetOption::OPTION_PINNED:
    {
        return w->isPinned();
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppWindow::updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params)
{
    widget->layout()->addWidget(child->getWidget());
}

}

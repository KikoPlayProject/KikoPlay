#include "appbutton.h"
#include <QPushButton>
#include "Extension/App/kapp.h"
#include "UI/ela/ElaTheme.h"
#include "qpainter.h"

namespace  Extension
{
const char *AppButton::AppWidgetName = "button";
const char *AppButton::MetaName = "meta.kiko.widget.button";
const char *AppButton::StyleClassName = "AppPushButton";

AppWidget *AppButton::create(AppWidget *parent, KApp *app)
{
    return new AppButton(parent);
}

void AppButton::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"click", click},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppButton::MetaName, members, AppWidget::MetaName);
}

int AppButton::click(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppButton::MetaName);
    if (!appWidget)
    {
        return 0;
    }
    QPushButton *btn = static_cast<QPushButton *>(appWidget->getWidget());
    QMetaObject::invokeMethod(btn, "click", Qt::QueuedConnection);
    return 0;
}

void AppButton::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QPushButton *btn = static_cast<QPushButton *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_CLICK:
    {
        QObject::connect(btn, &QPushButton::clicked, this, &AppButton::onClick);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppButton::AppButton(AppWidget *parent)
    : AppWidget{parent}
{
    widget = new AppPushButton(parent? parent->getWidget() : nullptr);
}

bool AppButton::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QPushButton *btn = static_cast<QPushButton *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        btn->setText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_CHECKABLE:
    {
        btn->setCheckable(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_CHECKED:
    {
        btn->setChecked(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_BUTTON_GROUP:
    {
        AppButtonGroupRes *res = AppButtonGroupRes::getButtonRes(app);
        const QString groupKey = val.toString().trimmed();
        if (groupKey.isEmpty()) break;
        QButtonGroup *group = res->getButtonGroup(val.toString());
        group->addButton(btn);
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppButton::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    QPushButton *btn = static_cast<QPushButton *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        return btn->text();
    }
    case AppWidgetOption::OPTION_CHECKABLE:
    {
        return btn->isCheckable();
    }
    case AppWidgetOption::OPTION_CHECKED:
    {
        return btn->isChecked();
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppButton::onClick()
{
    if (app && bindEvents.contains(AppEvent::EVENT_CLICK))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)}
        };
        app->eventCall(bindEvents[AppEvent::EVENT_CLICK], param);
    }
}

}

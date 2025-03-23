#include "appradio.h"
#include <QRadioButton>
#include "Extension/App/kapp.h"
#include "UI/ela/ElaRadioButton.h"

namespace Extension
{
const char *AppRadio::AppWidgetName = "radio";
const char *AppRadio::MetaName = "meta.kiko.widget.radio";
const char *AppRadio::StyleClassName = "QRadioButton";


AppWidget *AppRadio::create(AppWidget *parent, KApp *app)
{
    return new AppRadio(parent);
}

void AppRadio::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"click", click},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppRadio::MetaName, members, AppWidget::MetaName);
}

int AppRadio::click(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppRadio::MetaName);
    if (!appWidget)
    {
        return 0;
    }
    QRadioButton *btn = static_cast<QRadioButton *>(appWidget->getWidget());
    QMetaObject::invokeMethod(btn, "click", Qt::QueuedConnection);
    return 0;
}

void AppRadio::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QRadioButton *radioBtn = static_cast<QRadioButton *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_TOGGLED:
    {
        QObject::connect(radioBtn, &QRadioButton::toggled, this, &AppRadio::onToggled);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppRadio::AppRadio(AppWidget *parent) : AppWidget{parent}
{
    widget = new ElaRadioButton(parent? parent->getWidget() : nullptr);
}

bool AppRadio::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QRadioButton *radioBtn = static_cast<QRadioButton *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        radioBtn->setText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_CHECKED:
    {
        radioBtn->setChecked(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_BUTTON_GROUP:
    {
        AppButtonGroupRes *res = AppButtonGroupRes::getButtonRes(app);
        const QString groupKey = val.toString().trimmed();
        if (groupKey.isEmpty()) break;
        QButtonGroup *group = res->getButtonGroup(val.toString());
        group->addButton(radioBtn);
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppRadio::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ) *succ = true;
    QRadioButton *radioBtn = static_cast<QRadioButton *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        return radioBtn->text();
    }
    case AppWidgetOption::OPTION_CHECKED:
    {
        return radioBtn->isChecked();
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppRadio::onToggled(bool checked)
{
    if (app && bindEvents.contains(AppEvent::EVENT_TOGGLED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"checked", checked}
        };
        app->eventCall(bindEvents[AppEvent::EVENT_TOGGLED], param);
    }
}

}

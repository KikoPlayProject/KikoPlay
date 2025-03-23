#include "appcheckbox.h"
#include <QCheckBox>
#include "Extension/App/kapp.h"
#include "UI/ela/ElaCheckBox.h"

namespace Extension
{
const char *AppCheckBox::AppWidgetName = "checkbox";
const char *AppCheckBox::MetaName = "meta.kiko.widget.checkbox";
const char *AppCheckBox::StyleClassName = "QCheckBox";


AppWidget *AppCheckBox::create(AppWidget *parent, KApp *app)
{
    return new AppCheckBox(parent);
}

void AppCheckBox::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"click", click},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppCheckBox::MetaName, members, AppWidget::MetaName);
}

int AppCheckBox::click(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppCheckBox::MetaName);
    if (!appWidget)
    {
        return 0;
    }
    QCheckBox *btn = static_cast<QCheckBox *>(appWidget->getWidget());
    QMetaObject::invokeMethod(btn, "click", Qt::QueuedConnection);
    return 0;
}

void AppCheckBox::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QCheckBox *checkBox = static_cast<QCheckBox *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_CHECK_STATE_CHANGED:
    {
        QObject::connect(checkBox, &QCheckBox::stateChanged, this, &AppCheckBox::onStateChanged);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppCheckBox::AppCheckBox(AppWidget *parent)
    : AppWidget{parent}
{
    widget = new ElaCheckBox(parent? parent->getWidget() : nullptr);
}

bool AppCheckBox::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QCheckBox *checkBox = static_cast<QCheckBox *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        checkBox->setText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_CHECK_STATE:
    {
        bool ok = false;
        int state = qMax(val.toInt(&ok), 0);
        if (ok)
        {
            checkBox->setCheckState(static_cast<Qt::CheckState>(state));
        }
        break;
    }
    case AppWidgetOption::OPTION_CHECKED:
    {
        checkBox->setChecked(val.toBool());
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppCheckBox::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    QCheckBox *checkBox = static_cast<QCheckBox *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        return checkBox->text();
    }
    case AppWidgetOption::OPTION_CHECK_STATE:
    {
        return checkBox->checkState();
    }
    case AppWidgetOption::OPTION_CHECKED:
    {
        return checkBox->isChecked();
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppCheckBox::onStateChanged(int state)
{
    if (app && bindEvents.contains(AppEvent::EVENT_CHECK_STATE_CHANGED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"state", state}
        };
        app->eventCall(bindEvents[AppEvent::EVENT_CHECK_STATE_CHANGED], param);
    }
}

}

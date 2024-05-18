#include "appslider.h"
#include <QSlider>
#include "Extension/App/kapp.h"


namespace Extension
{
const char *AppSlider::AppWidgetName = "slider";
const char *AppSlider::MetaName = "meta.kiko.widget.slider";
const char *AppSlider::StyleClassName = "QSlider";


AppWidget *AppSlider::create(AppWidget *parent, KApp *app)
{
    return new AppSlider(parent);
}

void AppSlider::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppSlider::MetaName, members, AppWidget::MetaName);
}

void AppSlider::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QSlider *slider = static_cast<QSlider *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_VALUE_CHANGED:
    {
        QObject::connect(slider, &QSlider::valueChanged, this, &AppSlider::onValueChanged);
        break;
    }
    case AppEvent::EVENT_SLIDER_MOVED:
    {
        QObject::connect(slider, &QSlider::sliderMoved, this, &AppSlider::onSliderMoved);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppSlider::AppSlider(AppWidget *parent) : AppWidget{parent}
{
    widget = new QSlider(Qt::Horizontal, parent? parent->getWidget() : nullptr);
}

bool AppSlider::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QSlider *slider = static_cast<QSlider *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_MIN:
    {
        slider->setMinimum(val.toInt());
        break;
    }
    case AppWidgetOption::OPTION_MAX:
    {
        slider->setMaximum(val.toInt());
        break;
    }
    case AppWidgetOption::OPTION_VALUE:
    {
        slider->setValue(val.toInt());
        break;
    }
    case AppWidgetOption::OPTION_SINGLE_STEP:
    {
        slider->setSingleStep(val.toInt());
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppSlider::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ) *succ = true;
    QSlider *slider = static_cast<QSlider *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_MIN:
    {
        return slider->minimum();
    }
    case AppWidgetOption::OPTION_MAX:
    {
        return slider->maximum();
    }
    case AppWidgetOption::OPTION_VALUE:
    {
        return slider->value();
    }
    case AppWidgetOption::OPTION_SINGLE_STEP:
    {
        return slider->singleStep();
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppSlider::onValueChanged(int value)
{
    if (app && bindEvents.contains(AppEvent::EVENT_VALUE_CHANGED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"value", value}
        };
        app->eventCall(bindEvents[AppEvent::EVENT_VALUE_CHANGED], param);
    }
}

void AppSlider::onSliderMoved(int value)
{
    if (app && bindEvents.contains(AppEvent::EVENT_SLIDER_MOVED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"value", value}
        };
        app->eventCall(bindEvents[AppEvent::EVENT_SLIDER_MOVED], param);
    }
}

}

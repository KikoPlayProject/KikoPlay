#include "appprogress.h"
#include <QProgressBar>

namespace Extension
{
const char *AppProgress::AppWidgetName = "progress";
const char *AppProgress::MetaName = "meta.kiko.widget.progress";
const char *AppProgress::StyleClassName = "QProgressBar";

AppWidget *AppProgress::create(AppWidget *parent, KApp *app)
{
    return new AppProgress(parent);
}

void AppProgress::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppProgress::MetaName, members, AppWidget::MetaName);
}

void AppProgress::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QProgressBar *progress = static_cast<QProgressBar *>(this->widget);
    switch (event)
    {
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppProgress::AppProgress(AppWidget *parent) : AppWidget{parent}
{
    widget = new QProgressBar(parent? parent->getWidget() : nullptr);
}

bool AppProgress::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QProgressBar *progress = static_cast<QProgressBar *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        progress->setFormat(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_MIN:
    {
        progress->setMinimum(val.toInt());
        break;
    }
    case AppWidgetOption::OPTION_MAX:
    {
        progress->setMaximum(val.toInt());
        break;
    }
    case AppWidgetOption::OPTION_VALUE:
    {
        progress->setValue(val.toInt());
        break;
    }
    case AppWidgetOption::OPTION_TEXT_VISIBLE:
    {
        progress->setTextVisible(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_ALIGN:
    {
        bool ok = false;
        int alignment = val.toInt(&ok);
        if (ok && alignment >= 0)
        {
            progress->setAlignment(static_cast<Qt::Alignment>(alignment));
        }
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppProgress::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ) *succ = true;
    QProgressBar *progress = static_cast<QProgressBar *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        return progress->format();
    }
    case AppWidgetOption::OPTION_MIN:
    {
        return progress->minimum();
    }
    case AppWidgetOption::OPTION_MAX:
    {
        return progress->maximum();
    }
    case AppWidgetOption::OPTION_VALUE:
    {
        return progress->value();
    }
    case AppWidgetOption::OPTION_TEXT_VISIBLE:
    {
        return progress->isTextVisible();
    }
    case AppWidgetOption::OPTION_ALIGN:
    {
        return static_cast<int>(progress->alignment());
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}



}

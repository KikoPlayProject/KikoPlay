#include "appview.h"
#include <QWidget>
#include <QLayout>

namespace Extension
{
const char *AppView::AppWidgetName = "view";
const char *AppView::MetaName = "meta.kiko.widget.view";
const char *AppView::StyleClassName = "QWidget";

AppWidget *AppView::create(AppWidget *parent, KApp *app)
{
    return new AppView(parent);
}

void AppView::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppView::MetaName, members, AppWidget::MetaName);
}

AppView::AppView(AppWidget *parent)
    : AppWidget{parent}
{
    widget = new QWidget(parent? parent->getWidget() : nullptr);
    widget->setContentsMargins(0, 0, 0, 0);
}

bool AppView::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    switch (option)
    {
    case AppWidgetOption::OPTION_SPACING:
    {
        bool ok = false;
        int spacing = val.toInt(&ok);
        if (ok && spacing >= 0)
        {
            widget->layout()->setSpacing(spacing * widget->logicalDpiX() / 96);
        }
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
            if (widget->layout())
            {
                widget->layout()->setContentsMargins(margins);
            }
            else
            {
                widget->setContentsMargins(margins);
            }
        }
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppView::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    switch (option)
    {
    case AppWidgetOption::OPTION_SPACING:
    {
        return widget->layout()->spacing() * 96 / widget->logicalDpiX();
    }
    case AppWidgetOption::OPTION_CONTENT_MARGIN:
    {
        const QMargins margins = widget->layout() ? widget->layout()->contentsMargins() : widget->contentsMargins();
        return QString("%1,%2,%3,%4").arg(margins.left() * 96 / widget->logicalDpiX()).
                arg(margins.top() * 96 / widget->logicalDpiY()).
                arg(margins.right() * 96 / widget->logicalDpiX()).
                arg(margins.bottom() * 96 / widget->logicalDpiY());
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

}

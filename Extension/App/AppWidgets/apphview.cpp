#include "apphview.h"
#include <QWidget>
#include <QHBoxLayout>

namespace Extension
{
const char *AppHView::AppWidgetName = "hview";
const char *AppHView::MetaName = "meta.kiko.widget.view.h";
const char *AppHView::StyleClassName = "QWidget";

AppWidget *AppHView::create(AppWidget *parent, KApp *app)
{
    return new AppHView(parent);
}

void AppHView::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppHView::MetaName, members, AppView::MetaName);
}

AppHView::AppHView(AppWidget *parent) : AppView{parent}
{
    widget->setLayout(new QHBoxLayout());
}

void AppHView::updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params)
{
    QHBoxLayout *hBoxLayout = static_cast<QHBoxLayout *>(widget->layout());
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_SPACING))
    {
        bool ok = false;
        int spacing = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_SPACING].toInt(&ok);
        if (ok && spacing >= 0)
        {
            hBoxLayout->addSpacing(spacing * widget->logicalDpiX() / 96);
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_STRETCH))
    {
        bool ok = false;
        int stretch = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_STRETCH].toInt(&ok);
        if (ok && stretch >= 0)
        {
            hBoxLayout->addStretch(stretch);
        }
    }
    int widgetStretch = 0, widgetAlignment = 0;
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_STRETCH))
    {
        bool ok = false;
        int stretch = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_STRETCH].toInt(&ok);
        if (ok && stretch >= 0)
        {
            widgetStretch = stretch;
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_ALIGNMENT))
    {
        bool ok = false;
        int alignment = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_ALIGNMENT].toInt(&ok);
        if (ok && alignment >= 0)
        {
            widgetAlignment = alignment;
        }
    }
    hBoxLayout->addWidget(child->getWidget(), widgetStretch, static_cast<Qt::Alignment>(widgetAlignment));
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_SPACING))
    {
        bool ok = false;
        int spacing = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_SPACING].toInt(&ok);
        if (ok && spacing >= 0)
        {
            hBoxLayout->addSpacing(spacing * widget->logicalDpiX() / 96);
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_STRETCH))
    {
        bool ok = false;
        int stretch = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_STRETCH].toInt(&ok);
        if (ok && stretch >= 0)
        {
            hBoxLayout->addStretch(stretch);
        }
    }
}

}

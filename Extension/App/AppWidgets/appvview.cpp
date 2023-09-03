#include "appvview.h"
#include <QWidget>
#include <QVBoxLayout>

namespace Extension
{
const char *AppVView::AppWidgetName = "vview";
const char *AppVView::MetaName = "meta.kiko.widget.view.v";
const char *AppVView::StyleClassName = "QWidget";

AppWidget *AppVView::create(AppWidget *parent, KApp *app)
{
    return new AppVView(parent);
}

void AppVView::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppVView::MetaName, members, AppView::MetaName);
}

AppVView::AppVView(AppWidget *parent) : AppView{parent}
{
    widget->setLayout(new QVBoxLayout());
}

void AppVView::updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params)
{
    QVBoxLayout *vBoxLayout = static_cast<QVBoxLayout *>(widget->layout());
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_SPACING))
    {
        bool ok = false;
        int spacing = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_SPACING].toInt(&ok);
        if (ok && spacing >= 0)
        {
            vBoxLayout->addSpacing(spacing * widget->logicalDpiX() / 96);
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_STRETCH))
    {
        bool ok = false;
        int stretch = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_LEADING_STRETCH].toInt(&ok);
        if (ok && stretch >= 0)
        {
            vBoxLayout->addStretch(stretch);
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
    vBoxLayout->addWidget(child->getWidget(), widgetStretch, static_cast<Qt::Alignment>(widgetAlignment));
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_SPACING))
    {
        bool ok = false;
        int spacing = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_SPACING].toInt(&ok);
        if (ok && spacing >= 0)
        {
            vBoxLayout->addSpacing(spacing * widget->logicalDpiX() / 96);
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_STRETCH))
    {
        bool ok = false;
        int stretch = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_TRAILING_STRETCH].toInt(&ok);
        if (ok && stretch >= 0)
        {
            vBoxLayout->addStretch(stretch);
        }
    }
}


}

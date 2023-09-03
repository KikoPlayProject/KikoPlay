#include "appgview.h"
#include <QWidget>
#include <QGridLayout>

namespace Extension
{
const char *AppGView::AppWidgetName = "gview";
const char *AppGView::MetaName = "meta.kiko.widget.view.g";
const char *AppGView::StyleClassName = "QWidget";

AppWidget *AppGView::create(AppWidget *parent, KApp *app)
{
    return new AppGView(parent);
}

void AppGView::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppGView::MetaName, members, AppView::MetaName);
}

AppGView::AppGView(AppWidget *parent) : AppView{parent}
{
    widget->setLayout(new QGridLayout());
}

bool AppGView::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    switch (option)
    {
    case AppWidgetOption::OPTION_H_SPACING:
    {
        QGridLayout *gLayout = static_cast<QGridLayout *>(widget->layout());
        bool ok = false;
        int spacing = val.toInt(&ok);
        if (ok && spacing >= 0)
        {
            gLayout->setHorizontalSpacing(spacing * widget->logicalDpiX() / 96);
        }
        break;
    }
    case AppWidgetOption::OPTION_V_SPACING:
    {
        QGridLayout *gLayout = static_cast<QGridLayout *>(widget->layout());
        bool ok = false;
        int spacing = val.toInt(&ok);
        if (ok && spacing >= 0)
        {
            gLayout->setVerticalSpacing(spacing * widget->logicalDpiY() / 96);
        }
        break;
    }
    case AppWidgetOption::OPTION_ROW_STRETCH:
    case AppWidgetOption::OPTION_COL_STRETCH:
    {
        QGridLayout *gLayout = static_cast<QGridLayout *>(widget->layout());
        const QStringList rStretch = val.toString().split(";", Qt::SkipEmptyParts);
        for (auto &s : rStretch)
        {
            const int p = s.indexOf(":");
            if (p == -1) continue;
            bool ok = false;
            const int rc = s.leftRef(p).toInt(&ok);
            if (!ok || rc <= 0) continue;
            const int stretch = s.midRef(p + 1).toInt(&ok);
            if (!ok) continue;
            if (option == AppWidgetOption::OPTION_ROW_STRETCH)
            {
                gLayout->setRowStretch(rc - 1, stretch);
            }
            else
            {
                gLayout->setColumnStretch(rc - 1, stretch);
            }
        }
        break;
    }
    default:
        return AppView::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppGView::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    switch (option)
    {
    case AppWidgetOption::OPTION_H_SPACING:
    {
        return static_cast<QGridLayout *>(widget->layout())->horizontalSpacing();
    }
    case AppWidgetOption::OPTION_V_SPACING:
    {
        return static_cast<QGridLayout *>(widget->layout())->verticalSpacing();
    }
    default:
        return AppView::getWidgetOption(option, succ);
    }
}

void AppGView::updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params)
{
    QGridLayout *gLayout = static_cast<QGridLayout *>(widget->layout());
    int row = 0, col = 0, row_span = 1, col_span = 1, alignment = 0;
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW))
    {
        bool ok = false;
        int r = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW].toInt(&ok);
        if (ok && r > 0)
        {
            row = r - 1;
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL))
    {
        bool ok = false;
        int c = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL].toInt(&ok);
        if (ok && c > 0)
        {
            col = c - 1;
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW_SPAN))
    {
        bool ok = false;
        int rs = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW_SPAN].toInt(&ok);
        if (ok && rs >= 1)
        {
            row_span = rs;
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL_SPAN))
    {
        bool ok = false;
        int cs = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL_SPAN].toInt(&ok);
        if (ok && cs >= 1)
        {
            col_span = cs;
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_ALIGNMENT))
    {
        bool ok = false;
        int align = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_ALIGNMENT].toInt(&ok);
        if (ok && align >= 0)
        {
            alignment = align;
        }
    }
    gLayout->addWidget(child->getWidget(), row, col, row_span, col_span, static_cast<Qt::Alignment>(alignment));
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW_STRETCH))
    {
        bool ok = false;
        int row_stretch = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_ROW_STRETCH].toInt(&ok);
        if (ok && row_stretch >= 0)
        {
            gLayout->setRowStretch(row, row_stretch);
        }
    }
    if (params.contains(AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL_STRETCH))
    {
        bool ok = false;
        int col_stretch = params[AppWidgetLayoutDependOption::LAYOUT_DEPEND_COL_STRETCH].toInt(&ok);
        if (ok && col_stretch >= 0)
        {
            gLayout->setColumnStretch(col, col_stretch);
        }
    }
}


}

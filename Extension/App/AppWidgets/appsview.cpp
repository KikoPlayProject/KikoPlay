#include "appsview.h"
#include <QWidget>
#include <QStackedLayout>

namespace Extension
{
const char *AppSView::AppWidgetName = "sview";
const char *AppSView::MetaName = "meta.kiko.widget.view.s";
const char *AppSView::StyleClassName = "QWidget";

AppWidget *AppSView::create(AppWidget *parent, KApp *app)
{
    return new AppSView(parent);
}

void AppSView::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppSView::MetaName, members, AppView::MetaName);
}

AppSView::AppSView(AppWidget *parent) : AppView{parent}
{
    widget->setLayout(new QStackedLayout());
}

bool AppSView::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QStackedLayout *sLayout = static_cast<QStackedLayout *>(widget->layout());
    switch (option)
    {
    case AppWidgetOption::OPTION_CURRENT_INDEX:
    {
        bool ok = false;
        int index = val.toInt(&ok) - 1;
        if (ok)
        {
            sLayout->setCurrentIndex(index);
        }
        break;
    }
    case AppWidgetOption::OPTION_STACK_MODE:
    {
        sLayout->setStackingMode(val.toString().toLower() == "all"? QStackedLayout::StackAll : QStackedLayout::StackOne);
        break;
    }
    default:
        return AppView::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppSView::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    QStackedLayout *sLayout = static_cast<QStackedLayout *>(widget->layout());
    switch (option)
    {
    case AppWidgetOption::OPTION_COUNT:
    {
        return sLayout->count();
    }
    case AppWidgetOption::OPTION_CURRENT_INDEX:
    {
        return sLayout->currentIndex() + 1;
    }
    case AppWidgetOption::OPTION_STACK_MODE:
    {
        static const char *mode[] = {"one", "all"};
        return mode[sLayout->stackingMode()];
    }
    default:
        return AppView::getWidgetOption(option, succ);
    }
}

void AppSView::updateChildLayout(AppWidget *child, const QHash<AppWidgetLayoutDependOption, QVariant> &params)
{
    QStackedLayout *sLayout = static_cast<QStackedLayout *>(widget->layout());
    sLayout->addWidget(child->getWidget());
}


}

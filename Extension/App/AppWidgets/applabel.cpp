#include "applabel.h"
#include <QLabel>
#include "Extension/Modules/lua_appimage.h"
#include "Extension/App/kapp.h"

namespace Extension
{
const char *AppLabel::AppWidgetName = "label";
const char *AppLabel::MetaName = "meta.kiko.widget.label";
const char *AppLabel::StyleClassName = "QLabel";

AppWidget *AppLabel::create(AppWidget *parent, KApp *app)
{
    return new AppLabel(parent);
}

void AppLabel::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"setimg", setimg},
        {"getimg", getimg},
        {"clear", clear},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppLabel::MetaName, members, AppWidget::MetaName);
}

int AppLabel::setimg(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppLabel::MetaName);
    if (!appWidget || lua_gettop(L) < 2)
    {
        return 0;
    }
    QLabel *label = static_cast<QLabel *>(appWidget->getWidget());
    if (lua_isstring(L, 2))
    {
        const QPixmap img(lua_tostring(L, 2));
        if (img.isNull()) return 0;
        QMetaObject::invokeMethod(label, [label, &img](){
            label->setPixmap(img);
        }, Qt::BlockingQueuedConnection);
    }
    else if (lua_isnil(L, 2))
    {
        QMetaObject::invokeMethod(label, [label](){
            label->setPixmap(QPixmap());
        }, Qt::BlockingQueuedConnection);
    }
    else
    {
        QImage *img = AppImage::checkImg(L, 2);
        if (!img) return 0;
        QMetaObject::invokeMethod(label, [label, img](){
            label->setPixmap(QPixmap::fromImage(*img));
        }, Qt::BlockingQueuedConnection);
    }
    return 0;
}

int AppLabel::getimg(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppLabel::MetaName);
    if (!appWidget)
    {
        return 0;
    }
    QLabel *label = static_cast<QLabel *>(appWidget->getWidget());
    QImage *img = nullptr;
    QMetaObject::invokeMethod(label, [label, &img](){
        auto pixmap = label->pixmap(Qt::ReturnByValue);
        if (!pixmap.isNull())
        {
            img = new QImage(pixmap.toImage());
        }
    }, Qt::BlockingQueuedConnection);
    if (!img) return 0;
    AppImage::pushImg(L, img);
    return 1;

}

int AppLabel::clear(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppLabel::MetaName);
    if (!appWidget) return 0;
    QLabel *label = static_cast<QLabel *>(appWidget->getWidget());
    QMetaObject::invokeMethod(label, "clear", Qt::BlockingQueuedConnection);
    return 0;
}

void AppLabel::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QLabel *label = static_cast<QLabel *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_LINK_CLICK:
    {
        QObject::connect(label, &QLabel::linkActivated, this, &AppLabel::onLinkClick);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppLabel::AppLabel(AppWidget *parent)
    : AppWidget{parent}
{
    widget = new QLabel(parent? parent->getWidget() : nullptr);
}

bool AppLabel::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QLabel *label = static_cast<QLabel *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        label->setText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_SCALE_CONTENT:
    {
        label->setScaledContents(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_WORD_WRAP:
    {
        label->setWordWrap(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_OPEN_LINK:
    {
        label->setOpenExternalLinks(val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_TEXT_SELECTABLE:
    {
        label->setTextInteractionFlags(label->textInteractionFlags() | Qt::TextSelectableByMouse);
        break;
    }
    case AppWidgetOption::OPTION_ALIGN:
    {
        bool ok = false;
        int alignment = val.toInt(&ok);
        if (ok && alignment >= 0)
        {
            label->setAlignment(static_cast<Qt::Alignment>(alignment));
        }
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppLabel::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    QLabel *label = static_cast<QLabel *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TITLE:
    {
        return label->text();
    }
    case AppWidgetOption::OPTION_SCALE_CONTENT:
    {
        return label->hasScaledContents();
    }
    case AppWidgetOption::OPTION_WORD_WRAP:
    {
        return label->wordWrap();
    }
    case AppWidgetOption::OPTION_OPEN_LINK:
    {
        return label->openExternalLinks();
    }
    case AppWidgetOption::OPTION_TEXT_SELECTABLE:
    {
        return (bool)(label->textInteractionFlags() & Qt::TextSelectableByMouse);
    }
    case AppWidgetOption::OPTION_ALIGN:
    {
        return static_cast<int>(label->alignment());
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppLabel::onLinkClick(const QString &link)
{
    if (app && bindEvents.contains(AppEvent::EVENT_LINK_CLICK))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"link", link},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_LINK_CLICK], param);
    }
}

}

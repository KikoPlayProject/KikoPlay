#include "apptextbox.h"
#include <QPlainTextEdit>
#include "Extension/App/kapp.h"
#include "UI/widgets/kplaintextedit.h"

namespace Extension
{
const char *AppTextBox::AppWidgetName = "textbox";
const char *AppTextBox::MetaName = "meta.kiko.widget.textbox";
const char *AppTextBox::StyleClassName = "QPlainTextEdit";

AppWidget *AppTextBox::create(AppWidget *parent, KApp *app)
{
    return new AppTextBox(parent);
}

void AppTextBox::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"append", append},
        {"toend", toend},
        {"clear", clear},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppTextBox::MetaName, members, AppWidget::MetaName);
}

int AppTextBox::append(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTextBox::MetaName);
    if (!appWidget || lua_gettop(L) < 2) return 0;
    QPlainTextEdit *textBox = static_cast<QPlainTextEdit *>(appWidget->getWidget());
    bool isHtml = false;
    if(lua_gettop(L) == 3 && lua_type(L, 3) == LUA_TBOOLEAN)
    {
        isHtml = lua_toboolean(L, 3);
    }
    const QString text = lua_tostring(L, 2);
    QMetaObject::invokeMethod(textBox, [&](){
        if (isHtml) textBox->appendHtml(text);
        else  textBox->appendPlainText(text);
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppTextBox::toend(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTextBox::MetaName);
    if (!appWidget) return 0;
    QPlainTextEdit *textBox = static_cast<QPlainTextEdit *>(appWidget->getWidget());
    QMetaObject::invokeMethod(textBox, [&](){
        QTextCursor cursor = textBox->textCursor();
        cursor.movePosition(QTextCursor::End);
        textBox->setTextCursor(cursor);
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int AppTextBox::clear(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppTextBox::MetaName);
    if (!appWidget) return 0;
    QPlainTextEdit *textBox = static_cast<QPlainTextEdit *>(appWidget->getWidget());
    QMetaObject::invokeMethod(textBox, "clear", Qt::BlockingQueuedConnection);
    return 0;
}

void AppTextBox::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    QPlainTextEdit *textBox = static_cast<QPlainTextEdit *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_TEXT_CHANGED:
    {
        QObject::connect(textBox, &QPlainTextEdit::textChanged, this, &AppTextBox::onTextChanged);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppTextBox::AppTextBox(AppWidget *parent)
    : AppWidget{parent}
{
    widget = new KPlainTextEdit(parent? parent->getWidget() : nullptr, false);
}

bool AppTextBox::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    QPlainTextEdit *textBox = static_cast<QPlainTextEdit *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TEXT:
    {
        textBox->setPlainText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_PLACEHOLDER_TEXT:
    {
        textBox->setPlaceholderText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_EDITABLE:
    {
        textBox->setReadOnly(!val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_OPEN_LINK:
    {
        textBox->setTextInteractionFlags(textBox->textInteractionFlags() | Qt::LinksAccessibleByMouse);
        break;
    }
    case AppWidgetOption::OPTION_MAX_LINE:
    {
        textBox->setMaximumBlockCount(val.toInt());
        break;
    }
    case AppWidgetOption::OPTION_WORD_WRAP:
    {
        textBox->setWordWrapMode(val.toBool()? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
        break;
    }
    case AppWidgetOption::OPTION_DISABLE_H_SCROLLBAR:
    {
        textBox->setHorizontalScrollBarPolicy(val.toBool()? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
        break;
    }
    case AppWidgetOption::OPTION_DISABLE_V_SCROLLBAR:
    {
        textBox->setVerticalScrollBarPolicy(val.toBool()? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppTextBox::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    QPlainTextEdit *textBox = static_cast<QPlainTextEdit *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TEXT:
    {
        return textBox->toPlainText();
    }
    case AppWidgetOption::OPTION_PLACEHOLDER_TEXT:
    {
        return textBox->placeholderText();
    }
    case AppWidgetOption::OPTION_EDITABLE:
    {
        return !textBox->isReadOnly();
    }
    case AppWidgetOption::OPTION_OPEN_LINK:
    {
        return (bool)(textBox->textInteractionFlags() & Qt::LinksAccessibleByMouse);
    }
    case AppWidgetOption::OPTION_MAX_LINE:
    {
        return textBox->maximumBlockCount();
    }
    case AppWidgetOption::OPTION_WORD_WRAP:
    {
        return textBox->wordWrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere;
    }
    case AppWidgetOption::OPTION_DISABLE_H_SCROLLBAR:
    {
        return textBox->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
    }
    case AppWidgetOption::OPTION_DISABLE_V_SCROLLBAR:
    {
        return textBox->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff;
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppTextBox::onTextChanged()
{
    if (app && bindEvents.contains(AppEvent::EVENT_TEXT_CHANGED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_TEXT_CHANGED], param);
    }
}

}

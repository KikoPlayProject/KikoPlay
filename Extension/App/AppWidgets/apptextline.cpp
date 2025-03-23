#include "apptextline.h"
#include <QLineEdit>
#include "Extension/App/kapp.h"
#include "UI/ela/ElaLineEdit.h"

namespace  Extension
{
const char *AppTextLine::AppWidgetName = "textline";
const char *AppTextLine::MetaName = "meta.kiko.widget.textline";
const char *AppTextLine::StyleClassName = "ElaLineEdit";

AppWidget *AppTextLine::create(AppWidget *parent, KApp *app)
{
    return new AppTextLine(parent);
}

void AppTextLine::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppTextLine::MetaName, members, AppWidget::MetaName);
}

void AppTextLine::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    ElaLineEdit *lineEdit = static_cast<ElaLineEdit *>(this->widget);
    switch (event)
    {
    case AppEvent::EVENT_TEXT_CHANGED:
    {
        QObject::connect(lineEdit, &ElaLineEdit::textChanged, this, &AppTextLine::onTextChanged);
        break;
    }
    case AppEvent::EVENT_RETURN_PRESSED:
    {
        QObject::connect(lineEdit, &ElaLineEdit::returnPressed, this, &AppTextLine::onReturnPressed);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppTextLine::AppTextLine(AppWidget *parent)
    : AppWidget{parent}
{
    widget = new ElaLineEdit(parent? parent->getWidget() : nullptr);
}

bool AppTextLine::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    ElaLineEdit *lineEdit = static_cast<ElaLineEdit *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TEXT:
    {
        lineEdit->setText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_PLACEHOLDER_TEXT:
    {
        lineEdit->setPlaceholderText(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_EDITABLE:
    {
        lineEdit->setReadOnly(!val.toBool());
        break;
    }
    case AppWidgetOption::OPTION_INPUT_MASK:
    {
        lineEdit->setInputMask(val.toString());
        break;
    }
    case AppWidgetOption::OPTION_ECHO_MODE:
    {
        bool ok = false;
        int mode = qBound(0, val.toInt(&ok), 3);
        if (ok) lineEdit->setEchoMode(ElaLineEdit::EchoMode(mode));
        break;
    }
    case AppWidgetOption::OPTION_SHOW_CLEAR_BUTTON:
    {
        lineEdit->setClearButtonEnabled(val.toBool());
        break;
    }
    default:
        return AppWidget::setWidgetOption(option, val);
    }
    return true;
}

QVariant AppTextLine::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    ElaLineEdit *lineEdit = static_cast<ElaLineEdit *>(this->widget);
    switch (option)
    {
    case AppWidgetOption::OPTION_TEXT:
    {
        return lineEdit->text();
    }
    case AppWidgetOption::OPTION_PLACEHOLDER_TEXT:
    {
        return lineEdit->placeholderText();
    }
    case AppWidgetOption::OPTION_EDITABLE:
    {
        return !lineEdit->isReadOnly();
    }
    case AppWidgetOption::OPTION_INPUT_MASK:
    {
        return lineEdit->inputMask();
    }
    case AppWidgetOption::OPTION_ECHO_MODE:
    {
        return static_cast<int>(lineEdit->echoMode());
    }
    case AppWidgetOption::OPTION_SHOW_CLEAR_BUTTON:
    {
        return lineEdit->isClearButtonEnabled();
    }
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppTextLine::onTextChanged(const QString &text)
{
    if (app && bindEvents.contains(AppEvent::EVENT_TEXT_CHANGED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"text", text}
        };
        app->eventCall(bindEvents[AppEvent::EVENT_TEXT_CHANGED], param);
    }
}

void AppTextLine::onReturnPressed()
{
    if (app && bindEvents.contains(AppEvent::EVENT_RETURN_PRESSED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_RETURN_PRESSED], param);
    }
}

}

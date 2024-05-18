#include "appplayer.h"
#include "Extension/App/kapp.h"
#include "Play/Video/simpleplayer.h"
#include <QWindow>

namespace Extension
{
const char *AppPlayer::AppWidgetName = "player";
const char *AppPlayer::MetaName = "meta.kiko.widget.player";
const char *AppPlayer::StyleClassName = "QOpenGLWidget";

AppWidget *AppPlayer::create(AppWidget *parent, KApp *app)
{
    return new AppPlayer(parent);
}

void AppPlayer::registEnv(lua_State *L)
{
    const luaL_Reg members[] = {
        {"property", property},
        {"command", command},
        {nullptr, nullptr}
    };
    registClassFuncs(L, AppPlayer::MetaName, members, AppWidget::MetaName);
}

int AppPlayer::property(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppPlayer::MetaName);
    if (!appWidget || lua_gettop(L) < 2 || lua_type(L, 2) != LUA_TSTRING) return 0;
    const QString property = QString(lua_tostring(L, 2)).trimmed();
    if(property.isEmpty())
    {
        return 0;
    }
    int errCode = 0;
    const QVariant val = static_cast<AppPlayer *>(appWidget)->smPlayer->getMPVPropertyVariant(property, errCode);
    lua_pushnumber(L, errCode);
    pushValue(L, val);
    return 2;
}

int AppPlayer::command(lua_State *L)
{
    AppWidget *appWidget = checkWidget(L, 1, AppPlayer::MetaName);
    if (!appWidget || lua_gettop(L) < 2) return 0;
    const QVariant param = getValue(L, true, 10);
    int ret = static_cast<AppPlayer *>(appWidget)->smPlayer->setMPVCommand(param);
    lua_pushinteger(L, ret);
    return 1;
}

void AppPlayer::bindEvent(AppEvent event, const QString &luaFunc)
{
    if (!widget) return;
    switch (event)
    {
    case AppEvent::EVENT_CLICK:
    {
        QObject::connect(smPlayer, &SimplePlayer::mouseClick, this, &AppPlayer::onClick);
        break;
    }
    case AppEvent::EVENT_PLAYER_POS_CHANGED:
    {
        QObject::connect(smPlayer, &SimplePlayer::positionChanged, this, &AppPlayer::onPosChanged);
        break;
    }
    case AppEvent::EVENT_PLAYER_STATE_CHANGED:
    {
        QObject::connect(smPlayer, &SimplePlayer::stateChanged, this, &AppPlayer::onStateChanged);
        break;
    }
    case AppEvent::EVENT_PLAYER_DURATION_CHANGED:
    {
        QObject::connect(smPlayer, &SimplePlayer::durationChanged, this, &AppPlayer::onDurationChanged);
        break;
    }
    default:
        break;
    }
    AppWidget::bindEvent(event, luaFunc);
}

AppPlayer::AppPlayer(AppWidget *parent) : AppWidget{parent}
{
    smPlayer = new SimplePlayer;
    smPlayer->setWindowFlags(Qt::FramelessWindowHint);
    QWindow *native_wnd = QWindow::fromWinId(smPlayer->winId());
    QWidget *playerWindowWidget = QWidget::createWindowContainer(native_wnd);
    playerWindowWidget->setMouseTracking(true);
    playerWindowWidget->setParent(parent? parent->getWidget() : nullptr);
    smPlayer->show();
    widget = playerWindowWidget;
}

AppPlayer::~AppPlayer()
{
    smPlayer->deleteLater();
}

bool AppPlayer::setWidgetOption(AppWidgetOption option, const QVariant &val)
{
    return AppWidget::setWidgetOption(option, val);
}

QVariant AppPlayer::getWidgetOption(AppWidgetOption option, bool *succ)
{
    if (succ)
    {
        *succ = true;
    }
    switch (option)
    {
    default:
        return AppWidget::getWidgetOption(option, succ);
    }
}

void AppPlayer::onClick()
{
    if (app && bindEvents.contains(AppEvent::EVENT_CLICK))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)}
        };
        app->eventCall(bindEvents[AppEvent::EVENT_CLICK], param);
    }
}

void AppPlayer::onPosChanged(double value)
{
    if (app && bindEvents.contains(AppEvent::EVENT_PLAYER_POS_CHANGED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"pos", value},
            {"duration", smPlayer->getDuration()},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_PLAYER_POS_CHANGED], param);
    }
}

void AppPlayer::onStateChanged(int state)
{
    if (app && bindEvents.contains(AppEvent::EVENT_PLAYER_STATE_CHANGED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"state", state},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_PLAYER_STATE_CHANGED], param);
    }
}

void AppPlayer::onDurationChanged(double duration)
{
    if (app && bindEvents.contains(AppEvent::EVENT_PLAYER_DURATION_CHANGED))
    {
        const QVariantMap param = {
            {"srcId", this->objectName()},
            {"src", QVariant::fromValue((AppWidget *)this)},
            {"duration", duration},
        };
        app->eventCall(bindEvents[AppEvent::EVENT_PLAYER_DURATION_CHANGED], param);
    }
}

}

#include "keyaction.h"
#include "Play/Playlist/playlist.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "Play/playcontext.h"
#include "Play/Danmu/danmupool.h"
#include "Common/notifier.h"

namespace
{
    void switchItem(bool prev, const QString &nullMsg)
    {
        while(true)
        {
            const PlayListItem *item = GlobalObjects::playlist->playPrevOrNext(prev);
            if (item)
            {
                if(item->type == PlayListItem::ItemType::LOCAL_FILE)
                {
                    QFileInfo fi(item->path);
                    if(!fi.exists())
                    {
                        Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, QObject::tr("File not exist: %0").arg(item->title));
                        continue;
                    }
                }
                PlayContext::context()->playItem(item);
                break;
            }
            else
            {
                Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, nullMsg);
                break;
            }
        }
    }
}
void KeyAction::serialize(QDataStream &out)
{
    for (const ActionParam &param : actParams)
    {
        switch (param.type)
        {
        case ParamType::PARAM_INT:
            out << param.val.toInt();
            break;
        case ParamType::PARAM_STR:
            out << param.val.toString();
            break;
        default:
            break;
        }
    }
}

void KeyAction::deserialize(QDataStream &in)
{
    for (ActionParam &param : actParams)
    {
        switch (param.type)
        {
        case ParamType::PARAM_INT:
        {
            int val = 0;
            in >> val;
            param.val = val;
            break;
        }
        case ParamType::PARAM_STR:
        {
            QString val;
            in >> val;
            param.val = val;
            break;
        }
        default:
            break;
        }
    }
}

KeyAction *KeyAction::createAction(ActionType actType)
{
    switch (actType)
    {
    case KeyAction::ACT_PLAYPAUSE:
        return new KeyActionPlayPause;
    case KeyAction::ACT_SEEK_FORWARD:
        return new KeyActionSeekForward;
    case KeyAction::ACT_SEEK_BACKWARD:
        return new KeyActionSeekBackward;
    case KeyAction::ACT_FRAME_FORWARD:
        return new KeyActionFrameForward;
    case KeyAction::ACT_FRAME_BACKWARD:
        return new KeyActionFrameBackward;
    case KeyAction::ACT_PLAY_PREVIOUS:
        return new KeyActionPlayPrev;
    case KeyAction::ACT_PLAY_NEXT:
        return new KeyActionPlayNext;
    case KeyAction::ACT_VOLUME_UP:
        return new KeyActionVolumeUp;
    case KeyAction::ACT_VOLUME_DOWN:
        return new KeyActionVolumeUp;
    case KeyAction::ACT_FULLSCREEN:
        return new KeyActionFullScreen;
    case KeyAction::ACT_MINIMODE:
        return new KeyActionMiniMode;
    case KeyAction::ACT_REFRESH_DANMU:
        return new KeyActionRefreshDanmu;
    case KeyAction::ACT_ADD_DANMU:
        return new KeyActionAddDanmu;
    case KeyAction::ACT_EDIT_POOL:
        return new KeyActionEditPool;
    case KeyAction::ACT_EDIT_BLOCK_RULES:
        return new KeyActionEditBlockRules;
    case KeyAction::ACT_MPV_COMMAND:
        return new KeyActionMPVCommand;
    case KeyAction::ACT_NONE:
        break;
    }
    return nullptr;
}

QString KeyAction::getActionName(ActionType actType)
{
    if (actType == ActionType::ACT_NONE) return "";
    static QString actNames[ActionType::ACT_NONE] = {
        QObject::tr("Play/Pause"),
        QObject::tr("Seek Forward"),
        QObject::tr("Seek Backward"),
        QObject::tr("Frame Step Forward"),
        QObject::tr("Frame Step Backward"),
        QObject::tr("Play Previous Item"),
        QObject::tr("Play Next Item"),
        QObject::tr("Increase Volume"),
        QObject::tr("Decrease Volume"),
        QObject::tr("Toggle Fullscreen"),
        QObject::tr("Toggle Mini Mode"),
        QObject::tr("Refresh Danmu Pool"),
        QObject::tr("Add Danmu"),
        QObject::tr("Edit Danmu Pool"),
        QObject::tr("Edit Block Rules"),
        QObject::tr("MPV Command"),
    };
    return actNames[actType];
}

void KeyActionPlayPause::trigger()
{
    MPVPlayer::PlayState state = GlobalObjects::mpvplayer->getState();
    switch (state)
    {
    case MPVPlayer::Play:
        GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
        break;
    case MPVPlayer::EndReached:
    {
        GlobalObjects::mpvplayer->seek(0);
        GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
        break;
    }
    case MPVPlayer::Pause:
        GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
        break;
    case MPVPlayer::Stop:
        break;
    }
}

KeyActionSeekForward::KeyActionSeekForward(int seekStep) : KeyAction(KeyAction::ACT_SEEK_FORWARD)
{
    ActionParam param;
    param.desc = QObject::tr("Seek Step(s)");
    param.type = ParamType::PARAM_INT;
    param.val = seekStep;
    actParams.append(param);
}

void KeyActionSeekForward::trigger()
{
    const int seekStep = actParams[0].val.toInt();
    GlobalObjects::mpvplayer->seek(seekStep, true);
}

QString KeyActionSeekForward::getDesc() const
{
    const int seekStep = actParams[0].val.toInt();
    return QObject::tr("Seek Forward %1s").arg(seekStep);
}

KeyActionSeekBackward::KeyActionSeekBackward(int seekStep) : KeyAction(KeyAction::ACT_SEEK_BACKWARD)
{
    ActionParam param;
    param.desc = QObject::tr("Seek Step(s)");
    param.type = ParamType::PARAM_INT;
    param.val = seekStep;
    actParams.append(param);
}

void KeyActionSeekBackward::trigger()
{
    const int seekStep = actParams[0].val.toInt();
    GlobalObjects::mpvplayer->seek(-seekStep, true);
}

QString KeyActionSeekBackward::getDesc() const
{
    const int seekStep = actParams[0].val.toInt();
    return QObject::tr("Seek Backward %1s").arg(seekStep);
}

void KeyActionFrameForward::trigger()
{
    GlobalObjects::mpvplayer->frameStep();
    Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, QObject::tr("Frame Step: Forward"), 0, QVariantMap({{"type", "playerInfo"}}));
}

void KeyActionFrameBackward::trigger()
{
    GlobalObjects::mpvplayer->frameStep(false);
    Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, QObject::tr("Frame Step: Backward"), 0, QVariantMap({{"type", "playerInfo"}}));
}

void KeyActionPlayPrev::trigger()
{
    switchItem(true, QObject::tr("No Previous Item"));
}

void KeyActionPlayNext::trigger()
{
    switchItem(false, QObject::tr("No Next Item"));
}

void KeyActionVolumeUp::trigger()
{
    GlobalObjects::mpvplayer->setVolume(GlobalObjects::mpvplayer->getVolume() + 1);
    Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, QObject::tr("Volume: %0").arg(GlobalObjects::mpvplayer->getVolume()), 0, QVariantMap({{"type", "playerInfo"}}));
}

void KeyActionVolumeDown::trigger()
{
    GlobalObjects::mpvplayer->setVolume(GlobalObjects::mpvplayer->getVolume() - 1);
    Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, QObject::tr("Volume: %0").arg(GlobalObjects::mpvplayer->getVolume()), 0, QVariantMap({{"type", "playerInfo"}}));
}

bool KeyActionFullScreen::isValid(QObject *env)
{
    if (GlobalObjects::context()->curMainPage != 0) return false;
    if (env->inherits("MainWindow") && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
    {
        return true;
    }
    return env && env->inherits("PlayerWindow");
}

void KeyActionFullScreen::trigger()
{
    emit GlobalObjects::mpvplayer->toggleFullScreen();
}

bool KeyActionMiniMode::isValid(QObject *env)
{
    if (GlobalObjects::context()->curMainPage != 0) return false;
    if (env->inherits("MainWindow") && !GlobalObjects::mpvplayer->getCurrentFile().isEmpty())
    {
        return true;
    }
    return env && env->inherits("PlayerWindow");
}

void KeyActionMiniMode::trigger()
{
    emit GlobalObjects::mpvplayer->toggleMiniMode();
}

bool KeyActionRefreshDanmu::isValid(QObject *env)
{
    return GlobalObjects::context()->curMainPage == 0;
}

void KeyActionRefreshDanmu::trigger()
{
    emit GlobalObjects::danmuPool->triggerUpdate();
}

bool KeyActionAddDanmu::isValid(QObject *env)
{
    return GlobalObjects::context()->curMainPage == 0;
}

void KeyActionAddDanmu::trigger()
{
    emit GlobalObjects::danmuPool->triggerAdd();
}

bool KeyActionEditPool::isValid(QObject *env)
{
    return GlobalObjects::context()->curMainPage == 0;
}

void KeyActionEditPool::trigger()
{
    emit GlobalObjects::danmuPool->triggerEditPool();
}

bool KeyActionEditBlockRules::isValid(QObject *env)
{
    return GlobalObjects::context()->curMainPage == 0;
}

void KeyActionEditBlockRules::trigger()
{
    emit GlobalObjects::danmuPool->triggerEditBlockRules();
}

KeyActionMPVCommand::KeyActionMPVCommand() : KeyAction(KeyAction::ACT_MPV_COMMAND)
{
    ActionParam param1;
    param1.desc = QObject::tr("Command");
    param1.type = ParamType::PARAM_STR;
    actParams.append(param1);

    ActionParam param2;
    param2.desc = QObject::tr("Description");
    param2.type = ParamType::PARAM_STR;
    actParams.append(param2);
}

void KeyActionMPVCommand::trigger()
{
    if (runCommands.isEmpty())
    {
        parseCommand();
    }
    int ret = 0;
    for (const auto &command : runCommands)
    {
        ret = GlobalObjects::mpvplayer->runCommand(command);
    }
    const QString tip = ret == 0 ? getDesc() : QString("%1: %2").arg(getDesc()).arg(ret);
    Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, tip, 0, QVariantMap({{"type", "playerInfo"}}));
}

QString KeyActionMPVCommand::getDesc() const
{
    QString desc = actParams[1].val.toString();
    if (!desc.isEmpty()) return desc;
    return actParams[0].val.toString();
}

void KeyActionMPVCommand::parseCommand()
{
    const QString command = actParams[0].val.toString();
    runCommands.clear();
    QStringList commandStrs = command.split(';', Qt::SkipEmptyParts);
    for (const QString &cStr : commandStrs)
    {
        QStringList commandParts;
        QString curPart;
        int state = 0;
        bool escape = false;
        for (QChar c : cStr)
        {
            if (state == 0) {
                if (escape) {
                    curPart.append('\\');
                    curPart.append(c);
                    escape = false;
                } else if (c.isSpace()) {
                    if(!curPart.isEmpty())
                        commandParts.append(curPart);
                    state = 1;
                    curPart.clear();
                } else if (c == '\'') {
                    state = 2;
                } else if (c == '"') {
                    state = 3;
                } else {
                    curPart.append(c);
                    escape = (c == '\\');
                }
            } else if (state == 1) {
                if (!c.isSpace()) {
                    curPart.append(c);
                    state = 0;
                }
            } else if (state == 2) {
                if (escape) {
                    curPart.append('\\');
                    curPart.append(c);
                    escape = false;
                } else if (c == '\'') {
                    state = 0;
                } else {
                    curPart.append(c);
                    escape = (c == '\\');
                }
            } else if (state == 3) {
                if (escape) {
                    curPart.append('\\');
                    curPart.append(c);
                    escape = false;
                } else if (c == '"') {
                    state = 0;
                } else {
                    curPart.append(c);
                    escape = (c=='\\');
                }
            }
        }
        if (!curPart.isEmpty()) commandParts.append(curPart);
        runCommands.append(commandParts);
    }
}


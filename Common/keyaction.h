#ifndef KEYACTION_H
#define KEYACTION_H
#include <QString>
#include <QVariant>
#include <QVector>
#include <QObject>
#include <QDataStream>

struct KeyAction
{
    enum ActionType
    {
        ACT_PLAYPAUSE,
        ACT_STOP,
        ACT_SEEK_FORWARD,
        ACT_SEEK_BACKWARD,
        ACT_FRAME_FORWARD,
        ACT_FRAME_BACKWARD,
        ACT_PLAY_PREVIOUS,
        ACT_PLAY_NEXT,
        ACT_VOLUME_UP,
        ACT_VOLUME_DOWN,
        ACT_FULLSCREEN,
        ACT_MINIMODE,
        ACT_REFRESH_DANMU,
        ACT_ADD_DANMU,
        ACT_EDIT_POOL,
        ACT_EDIT_BLOCK_RULES,
        ACT_MPV_COMMAND,
        ACT_NONE,
    };

    enum ParamType
    {
        PARAM_INT,
        PARAM_STR,
    };

    struct ActionParam
    {
        ParamType type;
        QString desc;
        QVariant val;
    };

    static KeyAction *createAction(ActionType actType);
    static QString getActionName(ActionType actType);


    ActionType actType;
    QVector<ActionParam> actParams;

    KeyAction(ActionType type) : actType(type) {}
    virtual ~KeyAction() {}
    virtual QString getDesc() const { return getActionName(actType); }
    virtual bool isValid(QObject *env) { return false; }
    virtual void trigger() { }

    virtual void serialize(QDataStream &out);
    virtual void deserialize(QDataStream &in);

};

struct KeyActionPlayPause : public KeyAction
{
    KeyActionPlayPause() : KeyAction(KeyAction::ACT_PLAYPAUSE) {}
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
};

struct KeyActionStop : public KeyAction
{
    KeyActionStop() : KeyAction(KeyAction::ACT_STOP) {}
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
};

struct KeyActionSeekForward : public KeyAction
{
    KeyActionSeekForward(int seekStep = 5);
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
    QString getDesc() const override;
};

struct KeyActionSeekBackward : public KeyAction
{
    KeyActionSeekBackward(int seekStep = 5);
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
    QString getDesc() const override;
};

struct KeyActionFrameForward : public KeyAction
{
    KeyActionFrameForward() : KeyAction(KeyAction::ACT_FRAME_FORWARD) {}
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
};

struct KeyActionFrameBackward : public KeyAction
{
    KeyActionFrameBackward() : KeyAction(KeyAction::ACT_FRAME_BACKWARD) {}
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
};

struct KeyActionPlayPrev : public KeyAction
{
    KeyActionPlayPrev() : KeyAction(KeyAction::ACT_PLAY_PREVIOUS) {}
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
};

struct KeyActionPlayNext : public KeyAction
{
    KeyActionPlayNext() : KeyAction(KeyAction::ACT_PLAY_NEXT) {}
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
};

struct KeyActionVolumeUp : public KeyAction
{
    KeyActionVolumeUp() : KeyAction(KeyAction::ACT_VOLUME_UP) {}
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
};

struct KeyActionVolumeDown : public KeyAction
{
    KeyActionVolumeDown() : KeyAction(KeyAction::ACT_VOLUME_DOWN) {}
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
};

struct KeyActionFullScreen : public KeyAction
{
    KeyActionFullScreen() : KeyAction(KeyAction::ACT_FULLSCREEN) {}
    bool isValid(QObject *env) override;
    void trigger() override;
};

struct KeyActionMiniMode : public KeyAction
{
    KeyActionMiniMode() : KeyAction(KeyAction::ACT_MINIMODE) {}
    bool isValid(QObject *env) override;
    void trigger() override;
};

struct KeyActionRefreshDanmu : public KeyAction
{
    KeyActionRefreshDanmu() : KeyAction(KeyAction::ACT_REFRESH_DANMU) {}
    bool isValid(QObject *env) override;
    void trigger() override;
};

struct KeyActionAddDanmu : public KeyAction
{
    KeyActionAddDanmu() : KeyAction(KeyAction::ACT_ADD_DANMU) {}
    bool isValid(QObject *env) override;
    void trigger() override;
};

struct KeyActionEditPool : public KeyAction
{
    KeyActionEditPool() : KeyAction(KeyAction::ACT_EDIT_POOL) {}
    bool isValid(QObject *env) override;
    void trigger() override;
};

struct KeyActionEditBlockRules : public KeyAction
{
    KeyActionEditBlockRules() : KeyAction(KeyAction::ACT_EDIT_BLOCK_RULES) {}
    bool isValid(QObject *env) override;
    void trigger() override;
};

struct KeyActionMPVCommand : public KeyAction
{
    KeyActionMPVCommand();
    bool isValid(QObject *env) override { return env && env->inherits("PlayerWindow"); }
    void trigger() override;
    QString getDesc() const override;

    QVector<QStringList> runCommands;
    void parseCommand();
};

#endif // KEYACTION_H

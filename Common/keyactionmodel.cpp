#include "keyactionmodel.h"
#include "keyaction.h"
#include "globalobjects.h"
#include <QSettings>
#include <QKeySequence>

#define SETTING_KEY_KEY_INITED "Key/KeyInited"
#define SETTING_KEY_KEY_ACTIONS "Key/KeyActions"

KeyActionModel *KeyActionModel::instance()
{
    static KeyActionModel model;
    return &model;
}

KeyActionModel::KeyActionModel(QObject *parent)
    : QAbstractItemModel{parent}
{
    bool keyInited = GlobalObjects::appSetting->value(SETTING_KEY_KEY_INITED, false).toBool();
    if (!keyInited)
    {
        initKeys();
    }
    else
    {
        keyActionList = GlobalObjects::appSetting->value(SETTING_KEY_KEY_ACTIONS).value<QList<QSharedPointer<KeyActionItem>>>();
        for(auto &k : keyActionList)
        {
            keyActionHash.insert(k->key, k);
        }
    }
}

void KeyActionModel::runAction(const QString &key, KeyActionItem::ActionTrigger trigger, QObject *env)
{
    auto iter = keyActionHash.find(key);
    if (iter == keyActionHash.end()) return;
    QSharedPointer<KeyActionItem> item = iter.value();
    if (item->triggerType & trigger)
    {
        if (item->action->isValid(env))
        {
            item->action->trigger();
        }
    }
}

bool KeyActionModel::addKeyAction(const QString &key, int triggerType, KeyAction *action)
{
    if (keyActionHash.contains(key) || key.isEmpty())
    {
        delete action;
        return false;
    }
    QSharedPointer<KeyActionItem> item = QSharedPointer<KeyActionItem>::create();
    item->action.reset(action);
    item->key = key;
    item->triggerType = triggerType;
    beginInsertRows(QModelIndex(), keyActionList.size(), keyActionList.size());
    keyActionList.append(item);
    keyActionHash.insert(key, item);
    endInsertRows();
    updateSettings();
    return true;
}

void KeyActionModel::removeKeyAction(const QModelIndex &index)
{
    if (!index.isValid()) return;
    if (index.row() >= keyActionList.size()) return;
    const QString key = keyActionList[index.row()]->key;
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    keyActionList.removeAt(index.row());
    keyActionHash.remove(key);
    endRemoveRows();
    updateSettings();
}

void KeyActionModel::removeKeyAction(const QString &key)
{
    auto iter = keyActionHash.find(key);
    if (iter == keyActionHash.end()) return;
    QSharedPointer<KeyActionItem> item = iter.value();
    int row = keyActionList.indexOf(item);
    if (row != -1)
    {
        beginRemoveRows(QModelIndex(), row, row);
        keyActionList.removeAt(row);
        endRemoveRows();
    }
    keyActionHash.remove(key);
}

QSharedPointer<const KeyActionItem> KeyActionModel::getActionItem(const QModelIndex &index)
{
    if (!index.isValid()) return nullptr;
    if (index.row() >= keyActionList.size()) return nullptr;
    return keyActionList[index.row()];
}

QSharedPointer<const KeyActionItem> KeyActionModel::getActionItem(const QString &key)
{
    auto iter = keyActionHash.find(key);
    if (iter == keyActionHash.end()) return nullptr;
    return iter.value();
}

void KeyActionModel::updateKeyAction(const QModelIndex &index, const QString &key, int triggerType, KeyAction *action)
{
    if (!index.isValid()) return;
    if (index.row() >= keyActionList.size()) return;
    QSharedPointer<KeyActionItem> item = keyActionList[index.row()];
    if (item->key != key)
    {
        if (hasKey(key)) return;
        keyActionHash.remove(item->key);
        item->key = key;
        keyActionHash.insert(key, item);
    }
    item->triggerType = triggerType;
    if (action) item->action.reset(action);
    emit dataChanged(index.siblingAtColumn((int)Columns::KEY), index.siblingAtColumn((int)Columns::DESC));
    updateSettings();
}

QVariant KeyActionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    KeyActionModel::Columns col = static_cast<Columns>(index.column());
    const auto &action = keyActionList.at(index.row());
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
    {
        switch (col)
        {
        case Columns::KEY:
            return action->key;
        case Columns::DESC:
            return action->action->getDesc();
        default:
            break;
        }
    }
    return QVariant();
}

QVariant KeyActionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (section < headers.size()) return headers.at(section);
    }
    return QVariant();
}

void KeyActionModel::initKeys()
{
    auto addKeyAction = [this](const QString &key, KeyAction *action){
        QSharedPointer<KeyActionItem> item = QSharedPointer<KeyActionItem>::create();
        item->action.reset(action);
        item->key = key;
        item->triggerType = KeyActionItem::ActionTrigger::KEY_PRESS;
        keyActionList.append(item);
        keyActionHash.insert(key, item);
    };
    addKeyAction(QKeySequence(Qt::Key_Space).toString(), new KeyActionPlayPause);
    addKeyAction(QKeySequence(Qt::Key_Right).toString(), new KeyActionSeekForward);
    addKeyAction(QKeySequence(Qt::Key_Left).toString(), new KeyActionSeekBackward);
    addKeyAction(QKeySequence(Qt::CTRL | Qt::Key_Right).toString(), new KeyActionFrameForward);
    addKeyAction(QKeySequence(Qt::CTRL | Qt::Key_Left).toString(), new KeyActionFrameBackward);
    addKeyAction(QKeySequence(Qt::Key_Up).toString(),  new KeyActionVolumeUp);
    addKeyAction(QKeySequence(Qt::Key_Down).toString(), new KeyActionVolumeDown);
    addKeyAction(QKeySequence(Qt::Key_PageDown).toString(), new KeyActionPlayNext);
    addKeyAction(QKeySequence(Qt::Key_PageUp).toString(), new KeyActionPlayPrev);
    addKeyAction(QKeySequence(Qt::Key_Return).toString(), new KeyActionFullScreen);
    addKeyAction(QKeySequence(Qt::Key_Escape).toString(), new KeyActionMiniMode);
    addKeyAction(QKeySequence(Qt::Key_F5).toString(), new KeyActionRefreshDanmu);

    GlobalObjects::appSetting->setValue(SETTING_KEY_KEY_INITED, true);
    updateSettings();
}

void KeyActionModel::updateSettings()
{
    GlobalObjects::appSetting->setValue(SETTING_KEY_KEY_ACTIONS, QVariant::fromValue(keyActionList));
}

QDataStream &operator<<(QDataStream &out, const QList<QSharedPointer<KeyActionItem>> &l)
{
    int s = l.size();
    out << s;
    for (const auto &item : l)
    {
        out << item->key << item->triggerType << item->action->actType;
        item->action->serialize(out);
    }
    return out;
}

QDataStream &operator>>(QDataStream &in, QList<QSharedPointer<KeyActionItem>> &l)
{
    int lSize = 0;
    in >> lSize;
    for (int i = 0; i < lSize; ++i)
    {
        QSharedPointer<KeyActionItem> item = QSharedPointer<KeyActionItem>::create();
        in >> item->key >> item->triggerType;
        KeyAction::ActionType actType;
        in >> actType;
        item->action.reset(KeyAction::createAction(actType));
        if (item->action)
        {
            item->action->deserialize(in);
            l.append(item);
        }
    }
    return in;
}

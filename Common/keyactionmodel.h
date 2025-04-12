#ifndef KEYACTIONMODEL_H
#define KEYACTIONMODEL_H

#include <QAbstractItemModel>
#include <QSharedPointer>

struct KeyAction;

struct KeyActionItem
{
    enum ActionTrigger
    {
        KEY_PRESS = 0x1,
        KEY_RELEASE = 0x2,
    };

    QString key;
    int triggerType{ActionTrigger::KEY_PRESS};
    QSharedPointer<KeyAction> action;
};
Q_DECLARE_METATYPE(KeyActionItem)
Q_DECLARE_METATYPE(QList<QSharedPointer<KeyActionItem>>)
QDataStream &operator<<(QDataStream &out, const QList<QSharedPointer<KeyActionItem>> &l);
QDataStream &operator>>(QDataStream &in, QList<QSharedPointer<KeyActionItem>> &l);

class KeyActionModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    static KeyActionModel *instance();
    explicit KeyActionModel(QObject *parent = nullptr);

    void runAction(const QString &key, KeyActionItem::ActionTrigger trigger, QObject *env);
    bool addKeyAction(const QString &key, int triggerType, KeyAction *action);
    void removeKeyAction(const QModelIndex &index);
    void removeKeyAction(const QString &key);
    QSharedPointer<const KeyActionItem> getActionItem(const QModelIndex &index);
    QSharedPointer<const KeyActionItem> getActionItem(const QString &key);
    bool hasKey(const QString &key) const { return keyActionHash.contains(key); }
    void updateKeyAction(const QModelIndex &index, const QString &key, int triggerType, KeyAction *action);

public:
    enum struct Columns
    {
        KEY,
        DESC,
        NONE
    };
    const QStringList headers{tr("Shortcut Key"), tr("Description")};
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{ return parent.isValid() ? QModelIndex() : createIndex(row, column); }
    inline virtual QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    inline virtual int rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : keyActionList.size(); }
    inline virtual int columnCount(const QModelIndex &parent) const{ return parent.isValid() ? 0 : (int)Columns::NONE; }
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

public:

private:
    QHash<QString, QSharedPointer<KeyActionItem>> keyActionHash;
    QList<QSharedPointer<KeyActionItem>> keyActionList;

    void initKeys();
    void updateSettings();
};

#endif // KEYACTIONMODEL_H

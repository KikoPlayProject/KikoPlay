#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H
#include <QList>
#include <QHash>
#include <QSet>
#include <QObject>
#include <QSharedPointer>
#include "scriptbase.h"
class DanmuScript;
class LibraryScript;
class ResourceScript;
class ScriptBase;
class ScriptManager : public QObject
{
    Q_OBJECT
public:
    enum ScriptType
    {
        DANMU, LIBRARY, RESOURCE, UNKNOWN_STYPE
    };
    enum EventType
    {
        KEY_PRESS, UNKNOWN_ETYPE
    };
    enum class ScriptChangeState
    {
        ADD, REMOVE
    };

public:
    ScriptManager();
    ~ScriptManager();
    void refreshScripts(ScriptType type);
    void registerEvent(ScriptType sType, const QString &id, EventType eType, const QVariantList &eventParams);
    void sendEvent(EventType type, const QVariantList &params);

    const QList<QSharedPointer<ScriptBase>> &scripts(ScriptType type)  {return scriptLists[type]; }

signals:
    void scriptChanged(ScriptType type, const QString &id, ScriptChangeState state);
private:
    QList<QSharedPointer<ScriptBase>> scriptLists[ScriptType::UNKNOWN_STYPE];
    QHash<QString, QSharedPointer<ScriptBase>> id2script;
    QHash<QString, QSet<QString>> keys2scriptIds;
    QString getScriptPath();
};

#endif // SCRIPTMANAGER_H

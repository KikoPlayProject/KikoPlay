#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H
#include <QList>
#include <QHash>
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
        DANMU, LIBRARY, RESOURCE, UNKNOWN
    };
    enum EventType
    {
        KEY_PRESS
    };
    enum ScriptChangeState
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
    QList<QSharedPointer<ScriptBase>> scriptLists[ScriptType::UNKNOWN];
    QString getScriptPath();
};

#endif // SCRIPTMANAGER_H

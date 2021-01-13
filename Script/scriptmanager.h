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
    enum class ScriptChangeState
    {
        ADD, REMOVE
    };

public:
    ScriptManager();
    ~ScriptManager();

    void refreshScripts(ScriptType type);
    const QList<QSharedPointer<ScriptBase>> &scripts(ScriptType type)  {return scriptLists[type]; }

signals:
    void scriptChanged(ScriptType type, const QString &id, ScriptChangeState state);
private:
    QList<QSharedPointer<ScriptBase>> scriptLists[ScriptType::UNKNOWN_STYPE];

    QString getScriptPath();
};

#endif // SCRIPTMANAGER_H

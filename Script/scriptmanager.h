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
class ScriptManager : public QObject
{
    Q_OBJECT
public:
    enum class ScriptChangeState
    {
        ADD, REMOVE
    };

public:
    ScriptManager();

    void refreshScripts(ScriptType type);
    void deleteScript(const QString &id);
    const QList<QSharedPointer<ScriptBase>> &scripts(ScriptType type)  {return scriptLists[type]; }
    QSharedPointer<ScriptBase> getScript(const QString &id) {return id2scriptHash.value(id);}

signals:
    void scriptChanged(ScriptType type, const QString &id, ScriptChangeState state);
private:
    QList<QSharedPointer<ScriptBase>> scriptLists[ScriptType::UNKNOWN_STYPE];
    QHash<QString, QSharedPointer<ScriptBase>> id2scriptHash;

    QString getScriptPath();
};

#endif // SCRIPTMANAGER_H

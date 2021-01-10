#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H
#include <QList>
#include <QHash>
#include <QObject>
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
        DANMU=1, LIBRARY=2, RESOURCE=4
    };
    enum EventType
    {
        KEY_PRESS, LIBRARY_ITEM_MENU
    };

public:
    ScriptManager();
    ~ScriptManager();
    void refreshScripts(int types = DANMU|LIBRARY|RESOURCE);
    void sendEvent(EventType type, const QVariantList &params);

    const QList<ScriptBase *> &scripts(ScriptType type)  {return scriptHash[type]; }

signals:
    void scriptChanged(const QString &id, bool removed);
private:
    QHash<ScriptType, QList<ScriptBase *>> scriptHash;
    QString getScriptPath();
};

#endif // SCRIPTMANAGER_H

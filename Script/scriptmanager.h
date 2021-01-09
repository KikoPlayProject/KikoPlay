#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H
#include <QList>
class DanmuScript;
class LibraryScript;
class ResourceScript;
class ScriptManager
{
public:
    enum ScriptType
    {
        DANMU=1, LIBRARY=2, RESOURCE=4
    };
public:
    ScriptManager();
    ~ScriptManager();
    void refreshScripts(int types = DANMU|LIBRARY|RESOURCE);

    const QList<DanmuScript *> &danmuScripts() const {return dmScripts;}
    const QList<LibraryScript *> &libraryScripts() const {return libScripts;}
    const QList<ResourceScript *> &resourceScripts() const {return resScripts;}

private:
    QList<DanmuScript *> dmScripts;
    QList<LibraryScript *> libScripts;
    QList<ResourceScript *> resScripts;
};

#endif // SCRIPTMANAGER_H

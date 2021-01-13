#include "scriptmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include "danmuscript.h"
#include "libraryscript.h"
#include "resourcescript.h"

ScriptManager::ScriptManager()
{
    refreshScripts(ScriptType::DANMU);
    refreshScripts(ScriptType::LIBRARY);
    refreshScripts(ScriptType::RESOURCE);
}

void ScriptManager::refreshScripts(ScriptManager::ScriptType type)
{
    if(type == ScriptType::UNKNOWN_STYPE) return;
    QString scriptPath(getScriptPath());
    const char *subDirs[] = {"/danmu/", "/library/", "/resource/"};
    scriptPath += subDirs[type];

    QHash<QString, QSharedPointer<ScriptBase>> curScripts;
    for(auto &s : scriptLists[type])
    {
        curScripts.insert(s->getValue("path"), s);
    }
    QDir folder(scriptPath);
    for (QFileInfo fileInfo : folder.entryInfoList())
    {
        if (fileInfo.isFile() && fileInfo.suffix().toLower()=="lua")
        {
            QString path(fileInfo.absoluteFilePath());
            qint64 modifyTime = fileInfo.fileTime(QFile::FileModificationTime).toSecsSinceEpoch();
            bool add = curScripts.contains(path);
            if(!add)
            {
                if(curScripts[path]->getValue("time").toLongLong() < modifyTime)
                {
                    QString id = curScripts[path]->id();
                    scriptLists[type].removeAll(curScripts[path]);
                    curScripts.remove(path);
                    add = true;
                    emit scriptChanged(type, id, ScriptChangeState::REMOVE);
                }
            }
            if(add)
            {
                QSharedPointer<ScriptBase> cs;
                if(type == ScriptType::DANMU) cs.reset(new DanmuScript);
                else if(type == ScriptType::LIBRARY) cs.reset(new LibraryScript);
                else if(type == ScriptType::RESOURCE) cs.reset(new ResourceScript);
                if(cs)
                {
                    ScriptState state = cs->loadScript(path);
                    if(state == ScriptState::S_NORM)
                    {
                        scriptLists[type].append(cs);
                        emit scriptChanged(type, cs->id(), ScriptChangeState::ADD);
                    }
                }

            }
        }
    }
}


QString ScriptManager::getScriptPath()
{
    const QString strApp(QCoreApplication::applicationDirPath()+"/script");
#ifdef CONFIG_UNIX_DATA
    const QString strHome(QDir::homePath()+"/.config/kikoplay/script");
    const QString strSys("/usr/share/kikoplay/script");

    const QFileInfo fileinfoHome(strHome);
    const QFileInfo fileinfoSys(strSys);
    const QFileInfo fileinfoApp(strApp);

    QString scriptPathBase;

    if (fileinfoHome.exists() || fileinfoHome.isDir()) {
        scriptPathBase = strHome;
    } else if (fileinfoSys.exists() || fileinfoSys.isDir()) {
        scriptPathBase = strSys;
    } else {
        scriptPathBase = strApp;
    }
    return scriptPathBase;
#else
    return strApp;
#endif
}

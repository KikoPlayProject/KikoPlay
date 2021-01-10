#include "scriptmanager.h"
#include "scriptbase.h"
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>

ScriptManager::ScriptManager()
{

}

void ScriptManager::refreshScripts(int types)
{
    QString scriptPath(getScriptPath());
    QList<QPair<ScriptType, QString>> paths;
    if(types & ScriptType::DANMU) paths.append({ScriptType::DANMU, scriptPath + "/danmu/"});
    if(types & ScriptType::LIBRARY) paths.append({ScriptType::LIBRARY, scriptPath + "/library/"});
    if(types & ScriptType::RESOURCE) paths.append({ScriptType::RESOURCE, scriptPath + "/resource/"});
    for(const auto &p : paths)
    {
        QHash<QString, ScriptBase *> curScripts;
        for(ScriptBase *s : scriptHash[p.first])
        {
            curScripts.insert(s->getValue("path"), s);
        }
        QDir folder(p.second);
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
                        scriptHash[p.first].removeAll(curScripts[path]);
                        delete curScripts[path];
                        curScripts.remove(path);
                        add = true;
                    }
                }
                if(add)
                {
                    ScriptBase *cs = nullptr;
                    if(p.first == ScriptType::DANMU) cs = new DanmuScript();
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

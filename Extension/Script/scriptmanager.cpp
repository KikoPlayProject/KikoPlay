#include "scriptmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include "danmuscript.h"
#include "matchscript.h"
#include "libraryscript.h"
#include "resourcescript.h"
#include "bgmcalendarscript.h"
#include "Common/logger.h"

ScriptManager::ScriptManager(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<ScriptType>("ScriptType");
    qRegisterMetaType<ScriptState>("ScriptState");
    qRegisterMetaType<ScriptManager::ScriptChangeState>("ScriptChangeState");
    refreshScripts(ScriptType::DANMU);
    refreshScripts(ScriptType::MATCH);
    refreshScripts(ScriptType::LIBRARY);
    refreshScripts(ScriptType::RESOURCE);
    refreshScripts(ScriptType::BGM_CALENDAR);

    scriptThread = new QThread();
    scriptThread->setObjectName(QStringLiteral("scriptThread"));
    scriptThread->start(QThread::NormalPriority);
}

ScriptManager::~ScriptManager()
{
    scriptThread->quit();
    scriptThread->wait();
}

void ScriptManager::refreshScripts(ScriptType type)
{
    if(type == ScriptType::UNKNOWN_STYPE) return;
    QString scriptPath(getScriptPath());
    scriptPath += subDirs[type];

    QHash<QString, QSharedPointer<ScriptBase>> curScripts;
    for(auto &s : scriptLists[type])
    {
        curScripts.insert(s->getValue("path"), s);
    }
    QSet<QString> existPaths;
    QDir folder(scriptPath);
    bool changed = false;
    for (QFileInfo fileInfo : folder.entryInfoList())
    {
        if (fileInfo.isFile() && fileInfo.suffix().toLower()=="lua")
        {
            QString path(fileInfo.absoluteFilePath());
            qint64 modifyTime = fileInfo.fileTime(QFile::FileModificationTime).toSecsSinceEpoch();
            bool add = !curScripts.contains(path);
            existPaths.insert(path);
            if (!add)
            {
                if (curScripts[path]->getValue("time").toLongLong() < modifyTime)
                {
                    QString id = curScripts[path]->id();
                    scriptLists[type].removeAll(curScripts[path]);
                    curScripts.remove(path);
                    id2scriptHash.remove(id);
                    add = true;
                    changed = true;
                }
            }
            if (add)
            {
                QSharedPointer<ScriptBase> cs;
                if (type == ScriptType::DANMU) cs.reset(new DanmuScript);
                else if (type == ScriptType::MATCH) cs.reset(new MatchScript);
                else if (type == ScriptType::LIBRARY) cs.reset(new LibraryScript);
                else if (type == ScriptType::RESOURCE) cs.reset(new ResourceScript);
                else if (type == ScriptType::BGM_CALENDAR) cs.reset(new BgmCalendarScript);
                if (cs)
                {
                    ScriptState state = cs->loadScript(path);
                    if (state)
                    {
                        scriptLists[type].append(cs);
                        id2scriptHash[cs->id()] = cs;
                        changed = true;
					}
					else
					{
                        Logger::logger()->log(Logger::Script, QString("[ERROR][%1]%2").arg(path, state.info));
					}
                }

            }
        }
    }
    auto &scripts = scriptLists[type];
    for (auto iter = scripts.begin(); iter != scripts.end();)
    {
        if (!existPaths.contains((*iter)->getValue("path")))
        {
            QString id((*iter)->id());
            id2scriptHash.remove(id);
            iter = scripts.erase(iter);
            changed = true;
        } else {
            ++iter;
        }
    }
    if (changed) emit scriptChanged(type);
}

void ScriptManager::deleteScript(const QString &id)
{
    QString path;
    if(id2scriptHash.contains(id))
    {
        auto script = id2scriptHash.value(id);
        path = script->getValue("path");
        if(script->type()!=ScriptType::UNKNOWN_STYPE)
        {
            scriptLists[script->type()].removeAll(script);
            id2scriptHash.remove(id);
            emit scriptChanged(script->type());
        }
    }
    if(!path.isEmpty())
    {
        QFileInfo fi(path);
        fi.dir().remove(fi.fileName());
        int suffixPos = path.lastIndexOf('.');
        QFileInfo settingFile(path.mid(0,suffixPos)+".json");
        if(settingFile.exists()) settingFile.dir().remove(settingFile.fileName());
    }
}


QString ScriptManager::getScriptPath()
{
    const QString strApp(QCoreApplication::applicationDirPath()+"/extension/script");
#ifdef CONFIG_UNIX_DATA
    const QString strHome(QDir::homePath()+"/.config/kikoplay/extension/script");
    const QString strSys("/usr/share/kikoplay/extension/script");

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

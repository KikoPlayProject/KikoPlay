#include "scriptmanager.h"
#include "lua.hpp"
#include "globalobjects.h"
#include "Common/network.h"

#include <QCoreApplication>
#include <QDir>
#include <QBrush>
#include <QFileInfo>
#include <setjmp.h>

namespace
{
    jmp_buf jbuf;
    QMap<QString,QString> getTableKvMap(lua_State *L,int tablePos)
    {
        QMap<QString,QString> map;
        lua_pushnil(L);
        while (lua_next(L, tablePos))
        {
            lua_pushvalue(L, -2);
            const char* key = lua_tostring(L, -1);
            const char* value = lua_tostring(L, -2);
            if(key && value) map[key]=value;
            lua_pop(L, 2);
        }
        return map;
    }
    static int kiko_HttpGet(lua_State *L)
    {
		const char *curl = luaL_checkstring(L,1);
        luaL_checktype(L,2,LUA_TTABLE);
        luaL_checktype(L,3,LUA_TTABLE);
        auto queryMap=getTableKvMap(L,2);
        auto headerMap=getTableKvMap(L,3);
        QUrlQuery query;
        QStringList headers;
        for(auto iter=queryMap.cbegin();iter!=queryMap.cend();++iter)
            query.addQueryItem(iter.key(),iter.value());
        for(auto iter=headerMap.cbegin();iter!=headerMap.cend();++iter)
            headers<<iter.key()<<iter.value();
        QString errInfo;
        QString content;
        try
        {
            content=Network::httpGet(curl,query,headers);
        }
        catch(Network::NetworkError &err)
        {
            errInfo=err.errorInfo;
        }
        if(errInfo.isEmpty()) lua_pushnil(L);
        else lua_pushstring(L,errInfo.toStdString().c_str());
        lua_pushstring(L,content.toStdString().c_str());
        return 2;
    }
    static int lua_Panic(lua_State *L)
    {
        longjmp(jbuf, 1);
    }
    struct lua_StateWrapper
    {
        lua_State *L;
        lua_StateWrapper() : L(luaL_newstate())
        {
            luaL_openlibs(L);
            lua_register(L, "kiko_HttpGet", kiko_HttpGet);
            lua_atpanic(L, lua_Panic);
        }
        ~lua_StateWrapper() {lua_close(L);}
        void reset()
        {
            lua_close(L);
            L=luaL_newstate();
            luaL_openlibs(L);
            lua_register(L, "kiko_HttpGet", kiko_HttpGet);
            lua_atpanic(L, lua_Panic);
        }
    };
    lua_StateWrapper currentState;
    QList<QMap<QString,QString> > getSearchItems(lua_State *L,int tablePos)
    {
        QList<QMap<QString,QString> > table;
        lua_pushnil(L);
        while (lua_next(L, tablePos))
        {
            lua_pushnil(L);  // k item nil
            QMap<QString,QString> item;
            while (lua_next(L, -2))
            {
                lua_pushvalue(L, -2); // k item kk vv kk
                const char* key = lua_tostring(L, -1);
                const char* value = lua_tostring(L, -2);
                if(key && value) item[key]=value;
                lua_pop(L, 2);
            }
            lua_pop(L, 1);
            if(!item.isEmpty()) table<<item;
        }
        return table;
    }
    QString getScriptPath()
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
}
ScriptManager::ScriptManager(QObject *parent):QAbstractItemModel(parent)
{
    qRegisterMetaType<ScriptInfo>("ScriptInfo");
    qRegisterMetaType<QList<ScriptInfo> >("QList<ScriptInfo>");

    scriptWorker=new ScriptWorker;
    scriptWorker->moveToThread(GlobalObjects::workThread);
    QObject::connect(scriptWorker,&ScriptWorker::refreshDone,this,[this](QList<ScriptInfo> sList){
       if(!scriptList.isEmpty())
       {
           currentState.reset();
           curScriptId="";
       }
       beginResetModel();
       scriptList.swap(sList);
       endResetModel();
       bool hasNSId=false;
       for(auto &s:scriptList)
       {
            if(s.id==normalScriptId)
            {
                hasNSId=true;
                break;
            }
       }
       if((!hasNSId || normalScriptId.isEmpty()) && scriptList.count()>0)
       {
           setNormalScript(createIndex(0,0));
       }
       emit refreshDone();
    });
    normalScriptId = GlobalObjects::appSetting->value("Script/NormalRScript","").toString();
    refresh();
}

void ScriptManager::refresh()
{
    QMetaObject::invokeMethod(scriptWorker,"refreshScriptList");
}

QString ScriptManager::search(QString sid, const QString &keyword, int page, int &pageCount, QList<ResItem> &resultList)
{
    QMutexLocker locker(&searchMutex);
    if(sid.isEmpty())
    {
        sid = normalScriptId;
        if(sid.isEmpty())
        {
            if(scriptList.count()>0)
            {
                sid=scriptList.first().id;
                normalScriptId=sid;
                GlobalObjects::appSetting->setValue("Script/NormalRScript",sid);
            }
            else
            {
                pageCount=0;
                return tr("No Available Script");
            }
        }
    }

    QString scriptPath;

    for(auto &s:scriptList)
    {
        if(s.id==sid)
        {
            scriptPath = getScriptPath() + "/" + s.fileName;
            break;
        }
    }
    if(scriptPath.isEmpty())
    {
        pageCount=0;
        return tr("Script File Not Exist");
    }
    QString errInfo;
    if(curScriptId!=sid)
    {
        if(!curScriptId.isEmpty())
        {
            currentState.reset();
        }
        QFile luaFile(scriptPath);
        luaFile.open(QFile::ReadOnly);
        if(!luaFile.isOpen())
        {
            pageCount=0;
            return tr("Open Script File Failed");
        }
        QString luaScript(luaFile.readAll());
        if(luaL_loadstring(currentState.L,luaScript.toStdString().c_str()) || lua_pcall(currentState.L,0,0,0))
        {
            errInfo="Script Error: "+ QString(lua_tostring(currentState.L, -1));
            lua_pop(currentState.L,1);
            pageCount=0;
            return errInfo;
        }
    }
    curScriptId=sid;
    if(setjmp(jbuf)==0)
    {
        lua_getglobal(currentState.L, "search");
        luaL_checktype(currentState.L,-1,LUA_TFUNCTION);
        lua_pushstring(currentState.L,keyword.toStdString().c_str());
        lua_pushinteger(currentState.L, page);
        if(lua_pcall(currentState.L, 2, 3, 0))
        {
            errInfo="Script Error: "+ QString(lua_tostring(currentState.L, -1));
            lua_pop(currentState.L,1);
            pageCount=0;
            return errInfo;
        }
        if(lua_isnil(currentState.L,1))
        {
            luaL_checktype(currentState.L,2,LUA_TNUMBER);
            luaL_checktype(currentState.L,3,LUA_TTABLE);
            pageCount=lua_tonumber(currentState.L, 2);
            auto list=getSearchItems(currentState.L,3);
            for(auto &item:list)
            {
                ResItem rItem;
                rItem.title=item.value("title");
                rItem.size=item.value("size");
                rItem.time=item.value("time");
                rItem.magnet=item.value("magnet");
                rItem.url=item.value("url");
                resultList<<rItem;
            }
        }
        else
        {
            errInfo="Error: "+ QString(lua_tostring(currentState.L, 1));
            pageCount=0;
        }
        lua_pop(currentState.L,3);
    }
    else
    {
        errInfo="Script Error: "+ QString(lua_tostring(currentState.L, -1));
        pageCount=0;
        lua_settop(currentState.L,0);
    }
    return errInfo;
}

void ScriptManager::setNormalScript(const QModelIndex &index)
{
    const ScriptInfo &script=scriptList.at(index.row());
    QString oId=normalScriptId;
    normalScriptId=script.id;
    for(int i=0;i<scriptList.count();++i)
    {
        if(scriptList.at(i).id==oId)
        {
            QModelIndex oIndex(createIndex(i,0));
            emit dataChanged(oIndex,oIndex);
        }
    }
    emit dataChanged(index,index);
    GlobalObjects::appSetting->setValue("Script/NormalRScript",normalScriptId);
}

void ScriptManager::removeScript(const QModelIndex &index)
{
    const ScriptInfo &script=scriptList.at(index.row());
    if(script.id==curScriptId) curScriptId="";
    if(script.id==normalScriptId)
    {
        if(scriptList.count()>0)
            setNormalScript(createIndex(0,0));
        else
            normalScriptId="";
    }

    QFileInfo fi(getScriptPath() + "/" + script.fileName);

    if(fi.exists()) fi.dir().remove(fi.fileName());
    beginRemoveRows(QModelIndex(),index.row(),index.row());
    scriptList.removeAt(index.row());
    endRemoveRows();
    emit refreshDone();
}

QVariant ScriptManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    int col=index.column();
    const ScriptInfo &script=scriptList.at(index.row());
    if(role==Qt::DisplayRole)
    {
        switch (col)
        {
        case 0:
            return script.id;
        case 1:
            return script.title;
        case 2:
            return script.version;
        case 3:
            return script.desc;
        case 4:
            return script.fileName;
        }
    }
    else if(role==Qt::ForegroundRole)
    {
        if(script.id==normalScriptId)
            return QBrush(QColor(36,151,211));
    }
    return QVariant();
}

QVariant ScriptManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers({tr("Id"),tr("Title"),tr("Version"),tr("Description"),tr("File")});
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<5)return headers.at(section);
    }
    return QVariant();
}

void ScriptWorker::refreshScriptList()
{
    QString scriptPath(getScriptPath() + "/");
    QDir folder(scriptPath);
    QList<ScriptInfo> sList;
    for (QFileInfo fileInfo : folder.entryInfoList())
    {
        if (fileInfo.isFile() && fileInfo.suffix().toLower()=="lua")
        {
            lua_StateWrapper state;
            QFile luaFile(fileInfo.absoluteFilePath());
            luaFile.open(QFile::ReadOnly);
            if(!luaFile.isOpen()) continue;
            QString luaScript(luaFile.readAll());
            int bRet = luaL_loadstring(state.L,luaScript.toStdString().c_str());
            if(bRet) continue;
            bRet = lua_pcall(state.L,0,0,0);
            if(bRet) continue;
            lua_getglobal(state.L, "scriptInfo");
            if(lua_type(state.L,-1)!=LUA_TTABLE) continue;
            auto scriptInfo(getTableKvMap(state.L,1));
            if(!scriptInfo.contains("title") || !scriptInfo.contains("id")) continue;
            lua_pop(state.L,1);
            lua_getglobal(state.L, "search");
            if(lua_type(state.L,-1)!=LUA_TFUNCTION) continue;
            ScriptInfo script;
            script.title=scriptInfo.value("title");
            script.id=scriptInfo.value("id");
            script.desc=scriptInfo.value("description");
            script.version=scriptInfo.value("version");
            script.fileName=fileInfo.fileName();
            sList<<script;
        }
    }
    emit refreshDone(sList);
}

void SearchListModel::setList(QList<ResItem> &nList)
{
    beginResetModel();
    resultList.swap(nList);
    endResetModel();
}

QStringList SearchListModel::getMagnetList(const QModelIndexList &indexes)
{
    QStringList list;
    for(auto &index:indexes)
    {
        if(index.column()==0)
            list<<resultList.at(index.row()).magnet;
    }
    return list;
}

QVariant SearchListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    int col=index.column();
    const ResItem &item=resultList.at(index.row());
    if(role==Qt::DisplayRole)
    {
        switch (col)
        {
        case 0:
            return item.time;
        case 1:
            return item.title;
        case 2:
            return item.size;
        }
    }
    else if(role==Qt::ToolTipRole)
    {
        if(col==1)
            return item.title;
    }
    return QVariant();
}

QVariant SearchListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers({tr("Time"), tr("Title"),tr("Size")});
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<3)return headers.at(section);
    }
    return QVariant();
}

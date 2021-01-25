#ifndef SCRIPTBASE_H
#define SCRIPTBASE_H
#include <QObject>
#include <QHash>
#include <QMutex>
#include "lua.hpp"
class MutexLocker
{
    QMutex &m;
    bool locked;
public:
    MutexLocker(QMutex &mutex):m(mutex), locked(false){}
    ~MutexLocker() { if(locked) m.unlock(); }
    bool tryLock() { if(m.tryLock()) locked = true; return locked; }
private:
    MutexLocker(MutexLocker &);
};
struct ScriptState
{
    enum StateCode
    {
         S_NORM, S_BUSY, S_ERROR
    };
    ScriptState(const QString &e):info(e) {state = (info.isEmpty()?S_NORM:S_ERROR);}
    ScriptState(const char *e) {ScriptState(QString(e));}
    ScriptState(StateCode c, const QString &i=""):state(c), info(i) {}
    operator QString() {return info;}
    operator bool() {return state==S_NORM;}

    StateCode state;
    QString info;
};
class ScriptBase
{
public:
    ScriptBase();
    virtual ~ScriptBase();

public:
    struct ScriptSettingItem
    {
        QString title;
        QString description;
        QString choices;
        QString key;
        QString value;
    };
    const QList<ScriptSettingItem> &settings() const {return scriptSettings;}
    virtual ScriptState setOption(int index, const QString &value, bool callLua=true);
    virtual ScriptState setOption(const QString &key, const QString &value, bool callLua=true);
    virtual QString id() const {return scriptMeta.value("id");}
    virtual QString name() const {return scriptMeta.value("name");}
    virtual QString desc() const {return scriptMeta.value("desc");}
    virtual QString version() const {return scriptMeta.value("version");}
    virtual QString getValue(const QString &key) const {return scriptMeta.value(key);}

    virtual ScriptState loadScript(const QString &path);

protected:
    const char *luaSettingsTable = "settings";
    const char *luaMetaTable = "info";
    const char *luaSetOptionFunc = "setoption";

    lua_State *L;
    QMutex scriptLock;
    QHash<QString, QString> scriptMeta;
    QList<ScriptSettingItem> scriptSettings;
    bool settingsUpdated;

    QVariantList call(const char *fname, const QVariantList &params, int nRet, QString &errInfo);
    QVariant get(const char *name);
    void set(const char *name, const QVariant &val);
    ScriptState setTable(const char *tname, const QVariant &key, const QVariant &val);
    bool checkType(const char *name, int type);

    QString loadMeta(const QString &scriptPath);
    void loadSettings(const QString &scriptPath);
    void registerFuncs(const char *tname, const luaL_Reg *funcs);
public:
    static void pushValue(lua_State *L, const QVariant &val);
    static QVariant getValue(lua_State *L);
    static size_t getTableLength(lua_State *L, int pos);
};

#endif // SCRIPTBASE_H

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
class ScriptBase
{
public:
    ScriptBase();
    virtual ~ScriptBase();

public:
    struct ScriptSettingItem
    {
        enum class ValueType
        {
            SS_STRING, SS_STRINGLIST
        };
        ValueType vtype;
        QString title;
        QString description;
        QString choices;
        QString value;
    };
    const QList<ScriptSettingItem> &settings() const {return scriptSettings;}
    virtual QString setSetting(int index, const QString &value);
protected:
    lua_State *L;
    QMutex scriptLock;
    QHash<QString, QString> scriptMeta;
    QList<ScriptSettingItem> scriptSettings;

    QString loadScript(const QString &path);
    QVariantList call(const char *fname, const QVariantList &params, int nRet, QString &errInfo);
    QVariant get(const char *name);
    void set(const char *name, const QVariant &val);
    int setTable(const char *tname, const QVariant &key, const QVariant &val);
    bool checkType(const char *name, int type);

    void pushValue(const QVariant &val);
    QVariant getValue();
    size_t getTableLength(int pos);
    QString getMeta();
    void loadSettings();
};

#endif // SCRIPTBASE_H

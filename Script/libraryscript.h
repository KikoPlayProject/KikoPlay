#ifndef LIBRARYSCRIPT_H
#define LIBRARYSCRIPT_H
#include "scriptbase.h"
#include <QVariant>
struct EpInfo
{
    enum class EpType
    {
        EP, SP, OP, ED, Trailer, MAD, Other
    };
    EpType type = EpType::EP;
    int index = 0;
    QString name;
    QString localFile;
    qint64 finishTime = 0, lastPlayTime = 0;
    QString toString() const
    {
        if(type == EpType::EP || type == EpType::SP)
            return QObject::tr("No.%0 %1").arg(index).arg(name);
        return name;
    }
};
struct AnimeBase
{
    QString name;
    QString id;
    QScopedPointer<QList<EpInfo>> epList;
};
struct MatchResult
{
    bool success;
    QScopedPointer<AnimeBase> anime;
    QScopedPointer<EpInfo> ep;
};

class LibraryScript  : public ScriptBase
{
public:
    LibraryScript();
    ScriptState load(const QString &scriptPath);
public:
    bool supportMatch() const {return matchSupported;}
public:
    ScriptState animeSearch(const QString &keyword, QList<AnimeBase> &results);
    ScriptState getEp(const QString &id, QList<EpInfo> &results);
    ScriptState getTags(const QString &id, QStringList &results);
    ScriptState match(const QString &path, MatchResult &result);
private:
    bool matchSupported;
};

#endif // LIBRARYSCRIPT_H

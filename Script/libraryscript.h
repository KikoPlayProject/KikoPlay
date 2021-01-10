#ifndef LIBRARYSCRIPT_H
#define LIBRARYSCRIPT_H
#include "scriptbase.h"
//#include "MediaLibrary/animeinfo.h"
#include <QVariant>
#include <QPixmap>
struct EpInfo
{
    enum class EpType
    {
        EP=1, SP, OP, ED, Trailer, MAD, Other
    };
    EpType type = EpType::EP;
    double index = 0;
    QString name;
    QString localFile;
    qint64 finishTime = 0, lastPlayTime = 0;
    QString toString() const
    {
        if(type == EpType::EP || type == EpType::SP)
            return QObject::tr("No.%0 %1").arg(index).arg(name);
        return name;
    }
    QVariantMap toMap() const { return {{"name", name}, {"index", index}, {"type", int(type)}, {"localFile", localFile}, {"finishTime", QString::number(finishTime)}, {"lastPlayTime", QString::number(lastPlayTime)}}; }
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
struct Character
{
    QString name;
    QString id;
    QString actor;
    QString link;
    QString imgURL;
};
struct Anime
{
    QString name;
    QString desc;
    QString airDate;
    QString id;
    QString scriptId;
    qint64 addTime;
    int epCount;
    QString coverURL;
    QPixmap coverPixmap;

    QList<QPair<QString,QString>> staff;
    QList<EpInfo> epList;
    QList<Character> characters;
    QMap<QString, QByteArray> crtImages;
    QList<QByteArray> posters;

    QVariantMap toMap() const
    {
        QVariantList eps;
        for(const auto &ep : epList)
            eps.append(ep.toMap());
        return {{"name", name}, {"desc", desc}, {"id", id}, {"airDate", airDate}, {"epCount", epCount}, {"addTime", QString::number(addTime)}, {"eps", eps}};
    }
};
class LibraryScript  : public ScriptBase
{
public:
    LibraryScript();
    ScriptState loadScript(const QString &scriptPath);
public:
    bool supportMatch() const {return matchSupported;}
    const QList<QPair<QString, QString>> &getMenuItems() const {return menuItems;}
public:
    ScriptState search(const QString &keyword, QList<AnimeBase> &results);
    ScriptState getDetail(const QString &id, Anime &anime, QStringList &posters);
    ScriptState getEp(const QString &id, QList<EpInfo> &results);
    ScriptState getTags(const QString &id, QStringList &results);
    ScriptState match(const QString &path, MatchResult &result);
    ScriptState menuClick(const QString &mid, Anime &anime);
private:
    bool matchSupported, hasTagFunc;
    QList<QPair<QString, QString>> menuItems; // (title, id)
};

#endif // LIBRARYSCRIPT_H

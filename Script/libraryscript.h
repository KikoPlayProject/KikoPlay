#ifndef LIBRARYSCRIPT_H
#define LIBRARYSCRIPT_H
#include "scriptbase.h"
#include "MediaLibrary/animeinfo.h"
#include <QVariant>
#include <QPixmap>
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

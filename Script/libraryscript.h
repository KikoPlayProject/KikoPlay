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
    const QVector<QPair<QString, QString>> &getMenuItems() const {return menuItems;}
public:
    ScriptState search(const QString &keyword, QList<AnimeLite> &results);
    ScriptState getDetail(const AnimeLite &base, Anime *anime);
    ScriptState getEp(Anime *anime, QVector<EpInfo> &results);
    ScriptState getTags(Anime *anime, QStringList &results);
    ScriptState match(const QString &path, MatchResult &result);
    ScriptState menuClick(const QString &mid, Anime *anime);
private:
    const char *searchFunc = "search";
    const char *detailFunc = "detail";
    const char *epFunc = "getep";
    const char *tagFunc = "gettags";
    const char *matchFunc = "match";
    const char *menusTable = "menus";
    const char *menuFunc = "menuclick";

    bool matchSupported, hasTagFunc;
    QVector<QPair<QString, QString>> menuItems; // (title, id)
};

#endif // LIBRARYSCRIPT_H

#ifndef BGMCALENDARSCRIPT_H
#define BGMCALENDARSCRIPT_H

#include "scriptbase.h"
#include <QSet>
#include <QVariant>
struct BgmItem
{
    bool focus;
    bool isNew;
    int weekDay;
    QString title;
    QString airTime;
    QString airDate;
    QString bgmId;
    QStringList onAirURLs;
    QStringList onAirSites;
};
struct BgmSeason
{
    QString title;
    QString data;
    int newBgmCount = 0;
    QList<BgmItem> bgmList;
    QSet<QString> focusSet;
    QVariantMap toMap() const {return {{"title", title}, {"data", data}};}
};

class BgmCalendarScript : public ScriptBase
{
public:
    BgmCalendarScript();
    ScriptState loadScript(const QString &scriptPath);
public:
    ScriptState getSeason(QList<BgmSeason> &results);
    ScriptState getBgmList(BgmSeason &season);
private:
    const char *seasonFunc = "getseason";
    const char *bgmlistFunc = "getbgmlist";
};

#endif // BGMCALENDARSCRIPT_H

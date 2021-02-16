#ifndef DANMUSCRIPT_H
#define DANMUSCRIPT_H
#include "scriptbase.h"
#include "Play/Danmu/common.h"
#include <QStringList>

class DanmuScript : public ScriptBase
{
public:
    DanmuScript();
    ScriptState loadScript(const QString &scriptPath);
public:
    bool supportSearch() const {return canSearch;}
    bool supportLaunch() const {return canLaunch;}
    const QStringList &sampleURLs() const {return sampleSupporedURLs;}
public:
    ScriptState search(const QString &keyword, QList<DanmuSource> &results);
    ScriptState getEpInfo(const DanmuSource *source, QList<DanmuSource> &results);
    ScriptState getURLInfo(const QString &url, QList<DanmuSource> &results);
    ScriptState getDanmu(const DanmuSource *item, DanmuSource **nItem, QList<DanmuComment *> &danmuList);
    ScriptState hasSourceToLaunch(const QList<DanmuSource> &sources, bool &result);
    ScriptState launch(const QList<DanmuSource> &sources, const DanmuComment *comment);
    bool supportURL(const QString &url);

private:
    const char *luaSearchFunc = "search";
    const char *luaEpFunc = "epinfo";
    const char *luaURLFunc = "urlinfo";
    const char *luaDanmuFunc = "danmu";
    const char *luaLaunchCheckFunc = "canlaunch";
    const char *luaLaunchFunc = "launch";
    const char *urlReTable = "supportedURLsRe";
    const char *sampleUrlTable = "sampleSupporedURLs";

    bool canSearch, canLaunch;
    QStringList supportedURLsRe, sampleSupporedURLs;
    QString callGetSources(const char *fname,  const QVariant &param, QList<DanmuSource> &results);
};

#endif // DANMUSCRIPT_H

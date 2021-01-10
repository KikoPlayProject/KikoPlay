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
    const QStringList &sampleURLs() const {return sampleSupporedURLs;}
public:
    ScriptState search(const QString &keyword, QList<DanmuSource> &results);
    ScriptState getEpInfo(const DanmuSource *source, QList<DanmuSource> &results);
    ScriptState getURLInfo(const QString &url, QList<DanmuSource> &results);
    ScriptState getDanmu(const DanmuSource *item, DanmuSource &Item, QList<DanmuComment *> &danmuList);
private:
    bool canSearch;
    QStringList supportedURLsRe, sampleSupporedURLs;
    QString callGetSources(const char *fname,  const QVariant &param, QList<DanmuSource> &results);
};

#endif // DANMUSCRIPT_H

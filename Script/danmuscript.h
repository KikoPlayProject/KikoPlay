#ifndef DANMUSCRIPT_H
#define DANMUSCRIPT_H
#include "scriptbase.h"
#include "Play/Danmu/common.h"
#include <QStringList>

class DanmuScript : public ScriptBase
{
public:
    DanmuScript();
    QString load(const QString &scriptPath);
public:
    QString id() const {return scriptMeta.value("id");}
    bool supportSearch() const {return canSearch;}
    const QStringList &sampleURLs() const {return sampleSupporedURLs;}
public:
    QString search(const QString &keyword, QList<DanmuSource> &results);
    QString getEpInfo(const DanmuSource *source, QList<DanmuSource> &results);
    QString getURLInfo(const QString &url, QList<DanmuSource> &results);
    QString getSourceCode(DanmuSource *item);
    QString getDanmu(DanmuSource *item, QList<DanmuComment *> &danmuList);
    QString getDanmuFromCode(const QString &code, QList<DanmuComment *> &danmuList);
private:
    bool canSearch;
    QStringList supportedURLsRe, sampleSupporedURLs;
};

#endif // DANMUSCRIPT_H

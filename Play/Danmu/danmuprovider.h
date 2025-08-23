#ifndef PROVIDERMANAGER_H
#define PROVIDERMANAGER_H

#include <QObject>
#include "Extension/Script/scriptbase.h"
#include "common.h"
class DanmuProvider : public QObject
{
    Q_OBJECT
public:
    DanmuProvider(QObject *parent = nullptr);

    QList<QPair<QString, QString>> getSearchProviders();
    QList<QPair<QString, QStringList>> getSampleURLs();

    ScriptState danmuSearch(const QString &scriptId, const QString &keyword, const QMap<QString, QString> &options, QList<DanmuSource> &results);
    ScriptState getEpInfo(const DanmuSource *source, QList<DanmuSource> &results);
    ScriptState getURLInfo(const QString &url, QList<DanmuSource> &results);
    ScriptState downloadDanmu(const DanmuSource *item, QVector<DanmuComment *> &danmuList, DanmuSource **nItem=nullptr);

};

#endif // PROVIDERMANAGER_H

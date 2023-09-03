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
    void checkSourceToLaunch(const QString &poolId);
    void launch(const QStringList &ids, const QString &poolId, const QList<DanmuSource> &sources, DanmuComment *comment);

signals:
    void sourceCheckDown(const QString &poolId, const QStringList &supportedScripts);
    void danmuLaunchStatus(const QString &poolId, const QStringList &scriptIds, const QStringList &status, DanmuComment *comment);
};

#endif // PROVIDERMANAGER_H

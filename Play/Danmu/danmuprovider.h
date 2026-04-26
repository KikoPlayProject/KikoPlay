#ifndef PROVIDERMANAGER_H
#define PROVIDERMANAGER_H

#include <QObject>
#include "Extension/Script/scriptbase.h"
#include "common.h"
class TaskContext;
class DanmuProvider : public QObject
{
    Q_OBJECT
public:
    DanmuProvider(QObject *parent = nullptr);

    QList<QPair<QString, QString>> getSearchProviders();
    QList<QPair<QString, QStringList>> getSampleURLs();

    ScriptState danmuSearch(const QString &scriptId, const QString &keyword, const QMap<QString, QString> &options, QList<DanmuSource> &results, TaskContext *ctx = nullptr);
    ScriptState getEpInfo(const DanmuSource *source, QList<DanmuSource> &results, TaskContext *ctx = nullptr);
    ScriptState getURLInfo(const QString &url, QList<DanmuSource> &results, TaskContext *ctx = nullptr);
    ScriptState downloadDanmu(const DanmuSource *item, QVector<DanmuComment *> &danmuList, DanmuSource **nItem=nullptr, TaskContext *ctx = nullptr);

};

#endif // PROVIDERMANAGER_H

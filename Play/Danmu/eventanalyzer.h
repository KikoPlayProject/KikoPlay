#ifndef EVENTANALYZER_H
#define EVENTANALYZER_H
#include <QVector>
#include <QObject>
#include "common.h"
class Pool;
class EventAnalyzer : public QObject
{
public:
    EventAnalyzer(QObject *parent = nullptr);
    QList<DanmuEvent> analyze(Pool *pool);
private:
    Pool *curPool;
    QVector<float> timeSeries;
    int lag;
    float threshold, influence;

    int iterations;
    float c, d;

private:
    void moveAverage();
    QList<int> zScoreThresholding();
    QList<DanmuEvent> postProcess(const QList<int> &eventPoints);

    void setEventDescription(QList<DanmuEvent> &eventList);
    QStringList getDanmuRange(const DanmuEvent &dmEvent);
    QString textRank(const QStringList &dmList);
    float getSimilarity(const QString &t1, const QString &t2);
};

#endif // EVENTANALYZER_H

#ifndef DOWNLOADSPEEDHISTORY_H
#define DOWNLOADSPEEDHISTORY_H

#include <QVector>
#include <QtGlobal>

struct DownloadSpeedSample
{
    qint64 timestampMs;
    qint64 bytesPerSecond;
    bool startsSegment;
};

// Session-only history. Times share the DownloadWindow's monotonic clock.
class DownloadSpeedHistory
{
public:
    void append(qint64 nowMs, qint64 bytesPerSecond, int expectedIntervalMs)
    {
        prune(nowMs);
        const qint64 gapLimit = qMax(3000, qMax(lastIntervalMs, expectedIntervalMs) * 2);
        const bool startsSegment = points.isEmpty() || nowMs - points.last().timestampMs > gapLimit;
        const DownloadSpeedSample sample{nowMs, qMax(qint64(0), bytesPerSecond), startsSegment};
        // Notifications and poll replies may arrive in the same second.
        if (!points.isEmpty() && points.last().timestampMs / 1000 == nowMs / 1000)
        {
            const bool previousStart = points.last().startsSegment;
            points.last() = sample;
            points.last().startsSegment = previousStart;
        }
        else
        {
            points.append(sample);
        }
        lastIntervalMs = expectedIntervalMs;
    }

    void prune(qint64 nowMs)
    {
        int expired = 0;
        while (expired < points.size() && nowMs - points.at(expired).timestampMs > 60000)
            ++expired;
        if (expired > 0)
            points.remove(0, expired);
        if (!points.isEmpty())
            points.first().startsSegment = true;
    }

    const QVector<DownloadSpeedSample> &samples() const { return points; }
    bool isEmpty() const { return points.isEmpty(); }

private:
    QVector<DownloadSpeedSample> points;
    int lastIntervalMs = 1000;
};

#endif // DOWNLOADSPEEDHISTORY_H

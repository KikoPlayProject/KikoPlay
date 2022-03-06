#ifndef COUNTER_H
#define COUNTER_H

#include <QObject>
#include <atomic>
#include <QHash>
#include <QTimer>
#include <QMutex>


class Counter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Counter)
    explicit Counter(QObject *parent = nullptr);

public:
    static Counter *instance();
    void countValue(const QString &key, int val);
    void logCounter(const QString &key = "");
protected:
    void timerEvent(QTimerEvent* event);
private:
    struct CountStat
    {
        std::atomic<qint64> num;
        std::atomic<qint64> value;
    };
    QHash<QString, QSharedPointer<CountStat>> counterKV;
    std::atomic<bool> counterChanged;
    QMutex hashMutex;
    QBasicTimer flushTimer;
};

#endif // COUNTER_H

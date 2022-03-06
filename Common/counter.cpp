#include "counter.h"
#include "logger.h"

Counter::Counter(QObject *parent) : QObject(parent), counterChanged(false)
{
    flushTimer.start(10000, this);
}

Counter *Counter::instance()
{
    static Counter counter;
    return &counter;
}

void Counter::countValue(const QString &key, int val)
{
    if(key.isEmpty()) return;
    QMutexLocker lock(&hashMutex);
    if(!counterKV.contains(key))
    {
        counterKV[key] = QSharedPointer<CountStat>::create();
    }
    counterKV[key]->num++;
    counterKV[key]->value += val;
    counterChanged = true;
}

void Counter::logCounter(const QString &key)
{
    static QString msg("%1: %2, num: %3  total: %4");
    QStringList allKeys;
    if(!key.isEmpty() && counterKV.contains(key))
    {
        allKeys.append(key);
    }
    else
    {
        allKeys = counterKV.keys();
        std::sort(allKeys.begin(), allKeys.end());
    }
    QStringList allMsg{QString("All Counter(%1):").arg(allKeys.size())};
    for(const QString &key : allKeys)
    {
        double val = counterKV[key]->value, num = counterKV[key]->num;
        QString log = msg.arg(key, -20).arg(val / num, 8, 'f', 2).arg((qint64)num, 6).arg((qint64)val, 6);
        allMsg.append(log);
    }
    Logger::logger()->log(Logger::APP, allMsg.join('\n'));
}

void Counter::timerEvent(QTimerEvent *)
{
    if(counterChanged)
    {
        logCounter();
        counterChanged = false;
    }
}

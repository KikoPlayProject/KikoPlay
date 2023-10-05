#ifndef KSTATS_H
#define KSTATS_H
#include <QJsonDocument>
#include <QTimer>

class KStats: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KStats)
    KStats(QObject *parent = nullptr);
public:
    static KStats *instance();
protected:
    void timerEvent(QTimerEvent* event);
private:
    void statsUV();
private:
    const QString baseURL;
    QBasicTimer eventTimer;
    void post(const QString &path, const QJsonDocument &doc);
};

#endif // KSTATS_H

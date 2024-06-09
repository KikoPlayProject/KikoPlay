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
public:
    void statsUseApp(const QString &appId, const QString &appVersion, qint64 tc);
protected:
    void timerEvent(QTimerEvent* event);
private:
    void statsUV(bool isStartup = false);
private:
    const QString baseURL;
    QBasicTimer eventTimer;
    QJsonObject getBaseInfo(const QString &ev, qint64 ts = -1);
    void post(const QString &path, const QJsonDocument &doc);
};

#endif // KSTATS_H

#include "kstats.h"
#include "globalobjects.h"
#include <QSettings>
#include <QDateTime>
#include <QJsonObject>
#include <QSysInfo>
#include <QTimer>
#include "network.h"
#include "logger.h"

KStats::KStats(QObject *parent) : QObject(parent), baseURL("http://www.kstat.top")
{
    statsUV(true);
    eventTimer.start(7200 * 1000, this);
}

KStats *KStats::instance()
{
    static KStats stats;
    return &stats;
}

void KStats::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == eventTimer.timerId())
    {
        statsUV();
    }
}

void KStats::statsUV(bool isStartup)
{
    const qint64 lastTs = GlobalObjects::appSetting->value("Stats/DayFirstStartTime", 0).toLongLong();
    const QDate lastDate = QDateTime::fromSecsSinceEpoch(lastTs).date();
    const QDate curDate = QDate::currentDate();
    const qint64 curTs = QDateTime::currentSecsSinceEpoch();
    if (lastTs > 0 && lastDate.daysTo(curDate) <= 0) return;
    GlobalObjects::appSetting->setValue("Stats/DayFirstStartTime", curTs);
    QJsonObject uvInfo = {
        {"ts", curTs},
        {"ev", "uv"},
        {"os", QString("%1 %2").arg(QSysInfo::productType(), QSysInfo::productVersion())},
        {"kv", GlobalObjects::kikoVersion},
    };
    if (isStartup)
    {
        uvInfo["st"] = GlobalObjects::startupTime;
        QJsonObject stepTime;
        for (auto &p : GlobalObjects::stepTime)
        {
            stepTime[p.first] = p.second;
        }
        uvInfo["sst"] = stepTime;
    }
    post("/stats/", QJsonDocument(uvInfo));
    Logger::logger()->log(Logger::APP, "[KStats]stats uv: " + curDate.toString());
}

void KStats::post(const QString &path, const QJsonDocument &doc)
{
    QNetworkRequest request(QUrl(baseURL + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QByteArray data = doc.toJson(QJsonDocument::Compact);
    QNetworkReply *reply = Network::getManager()->post(request, data);
    QObject::connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

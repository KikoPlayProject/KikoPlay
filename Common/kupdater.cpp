#include "kupdater.h"
#include "globalobjects.h"
#include "Common/network.h"

KUpdater::KUpdater(QObject *parent)
    : QObject{parent}, hasNVer(false)
{

}

KUpdater *KUpdater::instance()
{
    static KUpdater updater;
    return &updater;
}

void KUpdater::check()
{
    const qint64 curTs = QDateTime::currentSecsSinceEpoch();
    GlobalObjects::appSetting->setValue("KikoPlay/LastCheckUpdate", curTs);

    QNetworkRequest req(QUrl("https://raw.githubusercontent.com/KikoPlayProject/KikoPlay/master/newVersion/version.json"));
    QNetworkAccessManager *manager = Network::getManager();
    QNetworkReply *reply = manager->get(req);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply](){
        if (reply->error() == QNetworkReply::NoError)
        {
            QJsonObject newVersionObj(Network::toJson(reply->readAll()).object());
            QString nVersionStr = newVersionObj.value("Version").toString();
            if (hasNewVersion(nVersionStr))
            {
                hasNVer = true;
                nVer = nVersionStr;
                nVerURL = newVersionObj.value("URL").toString();
                nVerDesc = QByteArray::fromBase64(newVersionObj.value("Description").toString().toUtf8());
            }
        }
        else
        {
            errorInfo = reply->errorString();
        }
        reply->deleteLater();
        emit checkDone();
    });
}

bool KUpdater::needCheck()
{
    const qint64 lastTs = GlobalObjects::appSetting->value("KikoPlay/LastCheckUpdate", 0).toLongLong();
    const QDate lastDate = QDateTime::fromSecsSinceEpoch(lastTs).date();
    const QDate curDate = QDate::currentDate();
    if (lastDate.daysTo(curDate) <= 0) return false;
    return true;
}

bool KUpdater::hasNewVersion(const QString &nv)
{
    const QStringList curVer(QString(GlobalObjects::kikoVersion).split('.' ,Qt::SkipEmptyParts));
    const QStringList newVer(nv.split('.', Qt::SkipEmptyParts));
    if (curVer.size() != newVer.size()) return false;
    for(int i = 0; i < curVer.size(); ++i)
    {
        int cv = curVer.at(i).toInt(), nv = newVer.at(i).toInt();
        if (cv < nv)
        {
            return true;
        }
        else if(cv > nv)
        {
            break;
        }
    }
    return false;
}

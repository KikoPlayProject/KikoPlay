#include "trackersubscriber.h"
#include <QDateTime>
#include "Common/network.h"
#include "Common/logger.h"
#include "globalobjects.h"

#define SETTING_KEY_TRACKER_AUTOCHECK "Download/TrackerSubscriberAutoCheck"

TrackerSubscriber::TrackerSubscriber(QObject *parent) : QAbstractItemModel(parent), lastCheckTime(0)
{
    loadTrackerListSource();
    QObject::connect(&checkTimer, &QTimer::timeout, this, [this](){
        Logger::logger()->log(Logger::APP, QString("Tracker Subsciber start checking..."));
        check(-1);
    });
    if (QDateTime::currentSecsSinceEpoch() - lastCheckTime > checkInterval/1000)
    {
        QTimer::singleShot(1000, this, [this](){
            Logger::logger()->log(Logger::APP, QString("Tracker Subsciber start checking..."));
            check(-1);
        });
    }
    bool autoCheck = GlobalObjects::appSetting->value(SETTING_KEY_TRACKER_AUTOCHECK, false).toBool();
    if (autoCheck) checkTimer.start(checkInterval);
}

TrackerSubscriber *TrackerSubscriber::subscriber()
{
    static TrackerSubscriber subscriber;
    return &subscriber;
}

void TrackerSubscriber::add(const QString &url)
{
    if(isChecking) return;
    beginInsertRows(QModelIndex(), trackerListSrcs.count(), trackerListSrcs.count());
    TrackerListSource src;
    src.url = url;
    src.lastTime = 0;
    src.isChecking = false;
    trackerListSrcs.append(src);
    endInsertRows();
    saveTrackerListSource();
}

void TrackerSubscriber::remove(int index)
{
    if(isChecking) return;
    Q_ASSERT(index>=0 && index<trackerListSrcs.size());
    beginRemoveRows(QModelIndex(), index, index);
    trackerListSrcs.removeAt(index);
    endRemoveRows();
    saveTrackerListSource();
}

void TrackerSubscriber::check(int index)
{
    QStringList urls;
    QList<QUrlQuery> queries;
    QVector<int> indexes;
    isChecking = true;
    auto updateState = [this](int i){
        QModelIndex itemIndex(createIndex(i, 0));
        emit dataChanged(itemIndex,itemIndex.sibling(itemIndex.row(),headers.count()-1));
    };
    emit checkStateChanged(true);
    if(index == -1)
    {
        int i = 0;
        for(TrackerListSource &src : trackerListSrcs)
        {
            urls.append(src.url);
            queries.append(QUrlQuery());
            src.isChecking = true;
            indexes.append(i);
            updateState(i++);
        }
        lastCheckTime = QDateTime::currentSecsSinceEpoch();
    }
    else
    {
        Q_ASSERT(index>=0 && index<trackerListSrcs.size());
        TrackerListSource &src = trackerListSrcs[index];
        urls.append(src.url);
        queries.append(QUrlQuery());
        indexes.append(index);
        src.isChecking = true;
        updateState(index);
    }
    QList<Network::Reply> replies = Network::httpGetBatch(urls, queries);
    for(int i = 0; i < replies.size(); ++i)
    {
        TrackerListSource &src = trackerListSrcs[indexes[i]];
        Network::Reply &reply = replies[i];
        if(reply.hasError)
        {
            Logger::logger()->log(Logger::APP, QString("Tracker Subsciber checking faild: %1, %2").arg(src.url, reply.errInfo));
        }
        else
        {
            src.lastTime = QDateTime::currentSecsSinceEpoch();
            QStringList tracks = QString(reply.content).split('\n', Qt::SkipEmptyParts);
            src.trackerlist.clear();
            for(auto &t : tracks)
            {
                QString tracker = t.trimmed();
                if(!tracker.isEmpty()) src.trackerlist.append(t);
            }
            emit trackerListChanged(indexes[i]);
        }
        src.isChecking = false;
        updateState(indexes[i]);
    }
    isChecking = false;
    saveTrackerListSource();
    emit checkStateChanged(false);
}

void TrackerSubscriber::setAutoCheck(bool on)
{
    on? checkTimer.start(checkInterval) : checkTimer.stop();
    GlobalObjects::appSetting->setValue(SETTING_KEY_TRACKER_AUTOCHECK, on);
}

bool TrackerSubscriber::isAutoCheck() const
{
    return GlobalObjects::appSetting->value(SETTING_KEY_TRACKER_AUTOCHECK, false).toBool();
}

QStringList TrackerSubscriber::allTrackers() const
{
    QSet<QString> trackerSet;
    for(const TrackerListSource &src : trackerListSrcs)
    {
        for(const QString &t : src.trackerlist)
        {
            trackerSet.insert(t);
        }
    }
    return QStringList(trackerSet.begin(), trackerSet.end());
}

QStringList TrackerSubscriber::getTrackers(int index) const
{
    Q_ASSERT(index>=0 && index<trackerListSrcs.size());
    const TrackerListSource &src = trackerListSrcs[index];
    return src.trackerlist;
}

QVariant TrackerSubscriber::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section < headers.size())return headers.at(section);
    }
    return QVariant();
}

QVariant TrackerSubscriber::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    Columns col = static_cast<Columns>(index.column());
    const TrackerListSource &tSrc = trackerListSrcs[index.row()];
    if(role == Qt::DisplayRole || role == Qt::ToolTipRole)
    {
        switch (col)
        {
        case Columns::URL:
            return tSrc.url;
        case Columns::TRACKERS:
            return tSrc.trackerlist.size();
        case Columns::LAST_CHECK:
            if(tSrc.isChecking) return tr("Checking...");
            if(tSrc.lastTime == 0) return "--";
            return QDateTime::fromSecsSinceEpoch(tSrc.lastTime).toString("yyyy-MM-dd hh:mm:ss");
        }
    }
    return QVariant();
}

void TrackerSubscriber::loadTrackerListSource()
{
    if(isChecking) return;
    QString filename = GlobalObjects::context()->dataPath + "tracker.xml";
    QFile trackerFile(filename);
    bool ret=trackerFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamReader reader(&trackerFile);
    trackerListSrcs.clear();
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name == "TrackerSource")
            {
                TrackerListSource src;
                src.url = reader.attributes().value("url").toString();
                src.lastTime = reader.attributes().value("lastCheck").toLongLong();
                src.isChecking = false;
                trackerListSrcs.append(src);
            }
            else if(name == "Tracker")
            {
                Q_ASSERT(!trackerListSrcs.isEmpty());
                trackerListSrcs.back().trackerlist.append(reader.readElementText().trimmed());
            }
            else if(name == "TrackerSubscriber")
            {
                lastCheckTime = reader.attributes().value("lastCheck").toLongLong();
            }
        }
        reader.readNext();
    }
}

void TrackerSubscriber::saveTrackerListSource()
{
    if(isChecking) return;
    QString filename = GlobalObjects::context()->dataPath + "tracker.xml";
    QFile trackerFile(filename);
    bool ret=trackerFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&trackerFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("TrackerSubscriber");
    writer.writeAttribute("lastCheck", QString::number(lastCheckTime));
    for(const TrackerListSource &src : trackerListSrcs)
    {
         writer.writeStartElement("TrackerSource");
         writer.writeAttribute("url", src.url);
         writer.writeAttribute("lastCheck", QString::number(src.lastTime));
         for(const QString &t : src.trackerlist)
         {
             writer.writeStartElement("Tracker");
             writer.writeCharacters(t);
             writer.writeEndElement();
         }
         writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndDocument();
}



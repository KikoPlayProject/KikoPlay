#ifndef TRACKERSUBSCRIBER_H
#define TRACKERSUBSCRIBER_H

#include <QAbstractItemModel>
#include <QTimer>
class TrackerSubscriber : public QAbstractItemModel
{
    Q_OBJECT
    Q_DISABLE_COPY(TrackerSubscriber)
    explicit TrackerSubscriber(QObject *parent = nullptr);
public:
    static TrackerSubscriber *subscriber();

    const QStringList headers={tr("URL"), tr("Tracker Count"), tr("Last Check")};
    enum class Columns
    {
        URL, TRACKERS, LAST_CHECK
    };
    struct TrackerListSource
    {
        QString url;
        qint64 lastTime;
        QStringList trackerlist;
        bool isChecking;
    };

    void add(const QString &url);
    void remove(int index);
    void check(int index = -1);
    void setAutoCheck(bool on);
    bool isAutoCheck() const;
    QStringList allTrackers() const;
    QStringList getTrackers(int index) const;
signals:
    void trackerListChanged(int index);
    void checkStateChanged(bool checking);
    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const {return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const {return parent.isValid() ? 0 : trackerListSrcs.size();}
    virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:headers.size();}
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

private:
    QVector<TrackerListSource> trackerListSrcs;
    bool isChecking;
    const int checkInterval = 12*60*60*1000;
    QTimer checkTimer;
    qint64 lastCheckTime;

    void loadTrackerListSource();
    void saveTrackerListSource();
};

#endif // TRACKERSUBSCRIBER_H

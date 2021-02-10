#ifndef CAPTURELISTMODEL_H
#define CAPTURELISTMODEL_H
#include <QAbstractListModel>
#include "animeinfo.h"
class CaptureListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit CaptureListModel(const QString &animeName, QObject *parent = nullptr);

    void deleteCaptures(const QModelIndexList &indexes);
    void deleteRow(int row);
    QPixmap getFullCapture(int row);
    const AnimeImage *getCaptureItem(int row);
    void setAnimeName(const QString &name);
signals:
    void fetching(bool);
private:
    QString animeName;
    const int limitCount=20;
    int currentOffset;
    bool hasMoreCaptures;
    QList<AnimeImage> captureList;
    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent) const{return parent.isValid()?0:captureList.count();};
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual void fetchMore(const QModelIndex &parent);
    virtual bool canFetchMore(const QModelIndex &) const {return hasMoreCaptures;};
};

#endif // CAPTURELISTMODEL_H

#ifndef CAPTURELISTMODEL_H
#define CAPTURELISTMODEL_H
#include <QAbstractListModel>
#include "animeinfo.h"
class CaptureListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit CaptureListModel(const QString &animeName, QObject *parent = nullptr);

    void deleteRow(int row);
    QPixmap getFullCapture(int row);
    const AnimeImage *getCaptureItem(int row);
    void setAnimeName(const QString &name);
    const QString getSnippetFile(int row);
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
    virtual int rowCount(const QModelIndex &parent) const override{return parent.isValid()?0:captureList.count();}
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual void fetchMore(const QModelIndex &parent) override;
    virtual bool canFetchMore(const QModelIndex &) const override {return hasMoreCaptures;};
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int) override;
};

#endif // CAPTURELISTMODEL_H

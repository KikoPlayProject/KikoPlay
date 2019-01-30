#ifndef ANIMEFILTERPROXYMODEL_H
#define ANIMEFILTERPROXYMODEL_H
#include <QtCore>
#include <QAbstractItemModel>
class AnimeFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AnimeFilterProxyModel(QObject *parent = nullptr):QSortFilterProxyModel(parent),filterType(0){}
    void setFilterType(int type);
    void setTags(const QStringList &tagList);
    void setTime(const QSet<QString> &timeSet);
private:
    int filterType;
    QSet<QString> timeFilterSet;
    QStringList tagFilterList;
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};
#endif // ANIMEFILTERPROXYMODEL_H

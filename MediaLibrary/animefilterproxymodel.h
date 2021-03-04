#ifndef ANIMEFILTERPROXYMODEL_H
#define ANIMEFILTERPROXYMODEL_H
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include "labelmodel.h"
class AnimeModel;
class AnimeFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AnimeFilterProxyModel(AnimeModel *srcModel, QObject *parent = nullptr);
    void setFilter(int type, const QString &str);
    void setTags(SelectedLabelInfo &&selectedLabels);
    enum OrderType
    {
        O_AddTime, O_Title, O_AirDate
    };
    void setOrder(OrderType oType);
    void setAscending(bool on);

signals:
    void animeMessage(const QString &msg, bool hasMore);
private:
    int filterType;
    OrderType orderType;
    bool ascending;
    SelectedLabelInfo filterLabels;
    void refreshAnimeCount(int cur, int total);
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
};
#endif // ANIMEFILTERPROXYMODEL_H

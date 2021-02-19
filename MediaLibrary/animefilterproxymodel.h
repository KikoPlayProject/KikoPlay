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
signals:
    void animeMessage(const QString &msg, bool hasMore);
private:
    int filterType;
    SelectedLabelInfo filterLabels;
    void refreshAnimeCount(int cur, int total);
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};
#endif // ANIMEFILTERPROXYMODEL_H

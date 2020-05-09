#ifndef ANIMEFILTERPROXYMODEL_H
#define ANIMEFILTERPROXYMODEL_H
#include <QtCore>
#include <QAbstractItemModel>
class AnimeFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AnimeFilterProxyModel(QObject *parent = nullptr);
    void setFilterType(int type);
    void setTags(const QStringList &tagList);
    void setTime(const QSet<QString> &timeSet);
signals:
    void animeMessage(const QString &msg, int flags, bool hasMore);
private:
    int filterType;
    QSet<QString> timeFilterSet;
    QStringList tagFilterList;
    void refreshAnimeCount(int cur, int total);
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};
#endif // ANIMEFILTERPROXYMODEL_H

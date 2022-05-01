#ifndef ANIMELISTMODEL_H
#define ANIMELISTMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
class AnimeModel;
class AnimeListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    AnimeListModel(AnimeModel *srcAnimeModel, QObject *parent = nullptr);

    enum class Columns
    {
        TITLE, AIR_DATE, ADD_TIME, SCRIPT, STATE
    };

    void updateCheckedInfo();
    void updateCheckedTag();
    void removeChecked();
    int checkedCount();
    void setAllChecked(bool on);
signals:
    void showMessage(const QString &msg, int flag);
    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const {return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:checkStatus.size();}
    inline virtual int columnCount(const QModelIndex &) const {return headers.count();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
private:
    const QStringList headers={tr("Title"),tr("Air Date"), tr("Add Time"), tr("Script"), tr("State")};
    AnimeModel *animeModel;
    QVector<Qt::CheckState> checkStatus;
    QHash<int, QString> stateHash;
    void cleanState();
    void refreshState();  
};
class AnimeListProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AnimeListProxyModel(QObject *parent = nullptr):QSortFilterProxyModel(parent){}
    // QSortFilterProxyModel interface
protected:
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
};
#endif // ANIMELISTMODEL_H

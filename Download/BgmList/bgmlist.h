#ifndef BGMLIST_H
#define BGMLIST_H
#include <QtCore>
#include <QAbstractItemModel>
struct BgmItem
{
    QString title;
    QString time;
    QString showDate;
    int bgmId;
    int week;
    QStringList onAirURL;
    QString sitesName;
    bool focus;
    bool isNew;
};
struct BgmSeason
{
    QList<BgmItem> bgmList;
    QSet<QString> focusSet;
    QString id;
    QString version;
    QString path;
    QString latestVersion;
    int newBgmCount;
};

class BgmList : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit BgmList(QObject *parent = nullptr);
    ~BgmList();
    void init();
    const QStringList &seasonList(){return seasons;}
    const QList<BgmItem> &bgmList(){return curSeason->bgmList;}
    void setSeason(const QString &id);
    void refresh();
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:(curSeason?curSeason->bgmList.count():0);}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:4;}
    inline virtual Qt::ItemFlags flags(const QModelIndex &index) const {return index.column()==3?(QAbstractItemModel::flags(index)|Qt::ItemIsUserCheckable):QAbstractItemModel::flags(index);}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
signals:
    //1:static 2:process 3: refresh
    void bgmStatusUpdated(int type, const QString &msg);
    void seasonsUpdated();
private:
    bool inited;
    bool needSave;
    QMap<QString,QString> sitesName;
    bool seasonsDownloaded, curSeasonDownloaded;
    QMap<QString, BgmSeason> bgmSeasons;
    QStringList seasons;
    BgmSeason *curSeason;

    bool fetchMetaInfo();
    bool fetchBgmList(BgmSeason &season);
    bool loadLocal(BgmSeason &season);
    void saveLocal(const BgmSeason &season);
    bool getLocalSeasons();

    QString getSiteName(const QString &url);

};
class BgmListFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit BgmListFilterProxyModel(QObject *parent = nullptr):QSortFilterProxyModel(parent),week(8),onlyFocus(false),onlyNew(false){}
    void setWeekFilter(int week);
    void setFocusFilter(bool onlyFocus);
    void setNewBgmFilter(bool onlyNew);
private:
    int week;
    bool onlyFocus;
    bool onlyNew;
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &) const;
};
#endif // BGMLIST_H

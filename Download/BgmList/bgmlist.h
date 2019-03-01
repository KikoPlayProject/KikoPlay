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
};
class BgmWorker : public QObject
{
    Q_OBJECT
public:
    explicit BgmWorker(QObject *parent = nullptr):QObject(parent){}
public slots:
    void fetchListInfo(int localYear, int localMonth, const QString &dataVersion, bool forceRefresh);
signals:
    void fetchDone(const QString &errInfo, const QString &content, int nYear,int nMonth,const QString &nDataVersion);
};
class BgmList : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit BgmList(QObject *parent = nullptr);
    ~BgmList();
    void init();
    const QList<BgmItem> &bgmList(){return bgms;}
    void refreshData(bool forceRefresh=false);
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:bgms.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:4;}
    inline virtual Qt::ItemFlags flags(const QModelIndex &index) const {return index.column()==3?(QAbstractItemModel::flags(index)|Qt::ItemIsUserCheckable):QAbstractItemModel::flags(index);}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
signals:
    //1:static 2:process 3: refresh
    void bgmStatusUpdated(int type, const QString &msg);
private:
    bool inited;
    bool needSave;
    bool showOnlyFocus;
    int curWeek;
    int localYear,localMonth;
    QString dataVersion;
    QMap<QString,QString> sitesName;
    QSet<QString> focusSet;
    QList<BgmItem> bgms;
    BgmWorker *bgmWorker;
    bool loadLocal();
    void saveLocal();
    QString getSiteName(const QString &url);

};
class BgmListFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit BgmListFilterProxyModel(QObject *parent = nullptr):QSortFilterProxyModel(parent),week(8),onlyFocus(false){}
    void setWeekFilter(int week);
    void setFocusFilter(bool onlyFocus);
private:
    int week;
    bool onlyFocus;
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &) const;
};
#endif // BGMLIST_H

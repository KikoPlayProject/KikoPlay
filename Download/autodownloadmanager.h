#ifndef AUTODOWNLOADMANAGER_H
#define AUTODOWNLOADMANAGER_H
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QIcon>
#include <QQueue>
struct DownloadRule
{
    DownloadRule():minSize(0),maxSize(0),lastCheckTime(0),searchInterval(10),state(0),download(true){}

    int id;
    QString name;
    QString searchWord;
    QString filterWord;
    QString scriptId;
    QString filePath;
    int minSize,maxSize; //MB

    QStringList lastCheckPosition;
    qint64 lastCheckTime;
    int searchInterval; //minute
    int state; //0: free 1: checking 2: stop 3: pause temporary
    bool download; //true: add download task; false: add to discoveredURLs
    QList<QPair<QString, QString> > discoveredURLs;
    QMutex lock;
};
struct DownloadRuleLog
{
    int ruleId;
    QString name;
    qint64 time;
    int type;  //0: general  1: resource finded  2: error
    QString content, addition;
    static DownloadRuleLog setLog(DownloadRule *rule, int type, const QString &content, const QString &addition="");
};
struct ResourceItem;
class DownloadRuleChecker : public QObject
{
    Q_OBJECT
public:
    explicit DownloadRuleChecker(QObject *parent = nullptr);
    void checkRules(QList<QSharedPointer<DownloadRule> > rules);
signals:
    void log(const DownloadRuleLog &ruleLog);
    void updateState(QSharedPointer<DownloadRule> rule);
private:
    QQueue<QSharedPointer<DownloadRule> > ruleQueue;
    bool isChecking;
    void check();
    void fetchInfo(DownloadRule *rule);
    bool satisfyRule(ResourceItem *item, DownloadRule *rule, const QList<QRegExp> &filterRegExps);
};
class LogModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit LogModel(QObject *parent = nullptr):QAbstractItemModel(parent){}

    void addLog(const DownloadRuleLog &log);
    void removeLog(int ruleId);
    QString getLog(const QModelIndex &index);
private:
    QList<DownloadRuleLog> logList;
    const QStringList headers={tr("Time"),tr("Rule"),tr("Content")};

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const {return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:logList.count();}
    virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:3;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
class LogFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LogFilterProxyModel(QObject *parent = nullptr):QSortFilterProxyModel(parent),filterId(-1){}
    void setFilterId(int id);
private:
    int filterId;
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};
class URLModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit URLModel(QObject *parent = nullptr):QAbstractItemModel(parent), rule(nullptr){}

    void setRule(DownloadRule *curRule);
    DownloadRule *getRule() const {return rule;}
    void addURI(const QString &title, const QString &url);
    QStringList getSelectedURIs(const QModelIndexList &indexes);
    void removeSelectedURIs(const QModelIndexList &indexes);
signals:
    void ruleChanged();
private:
    DownloadRule *rule;
    const QStringList headers={tr("Size/Title"),tr("URI")};
    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const {return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:2;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
class AutoDownloadManager : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AutoDownloadManager(QObject *parent = nullptr);
    ~AutoDownloadManager();
public:
    void addRule(DownloadRule *rule);
    void removeRules(const QModelIndexList &removeIndexes);
    DownloadRule *getRule(const QModelIndex &index, bool lockForEdit=false);
    void applyRule(DownloadRule *rule, bool ruleModified);
    void stopRules(const QModelIndexList &indexes);
    void startRules(const QModelIndexList &indexes);
    void checkAtOnce(const QModelIndexList &indexes);

    LogModel *logModel;
    URLModel *urlModel;
private:
    QList<QSharedPointer<DownloadRule> > downloadRules;
    QHash<int, DownloadRule *> ruleIdHash;
    DownloadRuleChecker *checker;
    QThread checkerThread;
    const QStringList status={tr("Free"),tr("Checking"),tr("Stop"),tr("Editing")};
    QIcon statusIcons[4]={QIcon(":/res/images/free.png"), QIcon(":/res/images/checking.png"), QIcon(":/res/images/stop.png"), QIcon(":/res/images/free.png")};
    const QStringList headers={tr("Status"),tr("Title"),tr("SearchWord"),tr("FilterWord"),tr("Interval(min)"),tr("Last Check")};
    QString ruleFileName;
    bool ruleChanged;
    QTimer *timer;

    void loadRules();
    void saveRules();
signals:
    void addTask(const QString &url, const QString &path);
    void checkRules(QList<QSharedPointer<DownloadRule> > rules);
    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:downloadRules.count();}
    virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:6;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // AUTODOWNLOADMANAGER_H

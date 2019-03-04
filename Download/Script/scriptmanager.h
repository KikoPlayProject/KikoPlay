#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H
#include <QAbstractItemModel>
struct ScriptInfo
{
    QString title;
    QString id;
    QString desc;
    QString version;
    QString fileName;
};
struct ResItem
{
    QString title;
    QString time;
    QString size;
    QString magnet;
    QString url;
};

class ScriptWorker : public QObject
{
    Q_OBJECT
public:
    explicit ScriptWorker(QObject *parent = nullptr):QObject(parent){}
public slots:
    void refreshScriptList();
signals:
    void refreshDone(QList<ScriptInfo> sList);
};
class ScriptManager : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ScriptManager(QObject *parent = nullptr);
    void refresh();
    QString search(QString sid, const QString &keyword, int page, int &pageCount, QList<ResItem> &resultList);
    const QString &getNormalScriptId() const {return normalScriptId;}
    const QList<ScriptInfo> &getScriptList() const {return scriptList;}
    void setNormalScript(const QModelIndex &index);
    void removeScript(const QModelIndex &index);
signals:
    void refreshDone();
private:
    QList<ScriptInfo> scriptList;
    ScriptWorker *scriptWorker;
    QString curScriptId;
    QString normalScriptId;
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:scriptList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:5;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
class SearchListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SearchListModel(QObject *parent = nullptr):QAbstractItemModel(parent){}
    void setList(QList<ResItem> &nList);
    ResItem getItem(const QModelIndex &index) const {return resultList.value(index.row());}
    QStringList getMagnetList(const QModelIndexList &indexes);

private:
    QList<ResItem> resultList;
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:resultList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:3;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // SCRIPTMANAGER_H

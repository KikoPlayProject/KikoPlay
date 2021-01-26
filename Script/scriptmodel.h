#ifndef SCRIPTMODEL_H
#define SCRIPTMODEL_H
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include "scriptmanager.h"

class ScriptModel  : public QAbstractItemModel
{
    Q_OBJECT
public:
    ScriptModel(QObject *parent = nullptr);
public:
    enum struct Columns
    {
        TYPE,
        ID,
        NAME,
        VERSION,
        DESC,
        NONE
    };
    QStringList scriptTypes{tr("Danmu"), tr("Library"),tr("Resources")};
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:scriptList.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:(int)Columns::NONE;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
private:
    struct ScriptInfo
    {
        ScriptType type;
        QString id;
        QString name;
        QString version;
        QString desc;
        QString path;
    };
    QList<ScriptInfo> scriptList;
};
class ScriptProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    int ctype;
public:
    explicit ScriptProxyModel(QObject *parent=nullptr):QSortFilterProxyModel(parent), ctype(-1){}
    void setType(int type);
public:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};
#endif // SCRIPTMODEL_H

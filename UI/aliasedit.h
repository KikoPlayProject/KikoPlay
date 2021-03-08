#ifndef ALIASEDIT_H
#define ALIASEDIT_H
#include "framelessdialog.h"
#include <QAbstractItemModel>
class AliasModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AliasModel(const QString &animeName, QObject *parent = nullptr);
signals:
    void addFailed();
private:
    QString anime;
    QStringList aliasList;
public slots:
   QModelIndex addAlias();
   void removeAlias(const QModelIndex &index);
    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:aliasList.size();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:1;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int );
    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    {
        Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
        if(!index.isValid()) return defaultFlags;
        return defaultFlags | Qt::ItemIsEditable;
    }
};
class AliasEdit : public CFramelessDialog
{
    Q_OBJECT
public:
    AliasEdit(const QString &animeName, QWidget *parent = nullptr);
};

#endif // ALIASEDIT_H

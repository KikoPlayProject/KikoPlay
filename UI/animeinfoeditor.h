#ifndef ANIMEINFOEDITOR_H
#define ANIMEINFOEDITOR_H
#include "framelessdialog.h"
#include <QAbstractItemModel>
class Anime;
class QDateEdit;
class QTextEdit;
class QLineEdit;
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
class StaffModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit StaffModel(const Anime *anime, QObject *parent = nullptr);
    QModelIndex addStaff();
    void removeStaff(const QModelIndex &index);

    QVector<QPair<QString, QString>> staffs;
signals:
    void staffChanged();
    // QAbstractItemModel interface
public:
    const QStringList headers={tr("Type"), tr("Staff")};
    enum class Columns
    {
        TYPE, STAFF
    };

    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:staffs.size();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:headers.size();}
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int );
    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    {
        Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
        if(!index.isValid()) return defaultFlags;
        return defaultFlags | Qt::ItemIsEditable;
    }
};
class AnimeInfoEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    AnimeInfoEditor(Anime *anime, QWidget *parent = nullptr);

    bool airDateChanged;
    bool epsChanged;
    bool staffChanged;
    bool descChanged;

    const QString airDate() const;
    int epCount() const;
    const  QVector<QPair<QString, QString>> &staffs() const;
    const QString desc() const;

private:
    Anime *curAnime;
    StaffModel *staffModel;
    QDateEdit *dateEdit;
    QLineEdit *epsEdit;
    QTextEdit *descEdit;
};

#endif // ANIMEINFOEDITOR_H

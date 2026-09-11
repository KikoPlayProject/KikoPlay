#ifndef DANMUVIEWMODEL_H
#define DANMUVIEWMODEL_H

#include <QSortFilterProxyModel>
#include <QAbstractItemModel>
#include "common.h"

template<typename T>
class DanmuViewModel : public QAbstractItemModel
{
public:
    explicit DanmuViewModel(const QVector<T> *danmuList, QObject *parent = nullptr): QAbstractItemModel(parent)
    {
        this->danmuList=danmuList;
    }
    enum Columns
    {
        TIME, TYPE, SENDER, DATETIME, TEXT,
    };
    enum Roles
    {
        SourceRole = Qt::UserRole+1,
        TypeRole, ClippedRole, SortRole
    };

private:
    const QVector<T> *danmuList;
public:
    inline virtual int columnCount(const QModelIndex &) const {return 5;}
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const {return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const{return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const{return parent.isValid()?0:danmuList->count();}
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        static QStringList headers={QObject::tr("Time"),QObject::tr("Type"),QObject::tr("User"),QObject::tr("Send Time"),QObject::tr("Content")};
        if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
        {
            if(section<5)return headers[section];
        }
        return QVariant();
    }
    virtual QVariant data(const QModelIndex &index, int role) const
    {
        if(!index.isValid()) return QVariant();
        const T &comment=danmuList->at(index.row());
        int col=index.column();
        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        {
            switch (col)
            {
            case 0:
            {
                int sec_total=comment->time/1000;
                int min=sec_total/60;
                int sec=sec_total-min*60;
                return QString("%1:%2").arg(min,2,10,QChar('0')).arg(sec,2,10,QChar('0'));
            }
            case 1:
            {
                static QString typeStr[]={QObject::tr("Roll"),QObject::tr("Top"),QObject::tr("Bottom")};
                return comment->type >= DanmuComment::Rolling && comment->type <= DanmuComment::Bottom
                        ? typeStr[comment->type] : QString();
            }
            case 2:
            {
                return comment->sender;
            }
            case 3:
            {
                if (comment->date <= 0) return QStringLiteral("—");
                return QDateTime::fromSecsSinceEpoch(comment->date).toString(
                            role == Qt::ToolTipRole ? "yyyy-MM-dd hh:mm:ss" : "yyyy-MM-dd");
            }
            case 4:
            {
                return comment->text;
            }
            }

            break;
        }
        case Qt::ForegroundRole:
        {
            if (col != TEXT) return QVariant();
            int c = comment->color;
            return QColor((c>>16)&0xff,(c>>8)&0xff,c&0xff);
        }
        case SortRole:
            if (col == TIME) return comment->time;
            if (col == DATETIME) return comment->date;
            if (col == TYPE) return int(comment->type);
            return data(index, Qt::DisplayRole);
        case SourceRole:
            return comment->source;
        case TypeRole:
            return comment->type;
        case ClippedRole:
            return comment->clipped;
        default:
            return QVariant();
        }
        return QVariant();
    }

};
class DanmuViewProxyModel : public QSortFilterProxyModel
{
public:
    explicit DanmuViewProxyModel(QObject *parent=nullptr):QSortFilterProxyModel(parent), sourceId(-1), typeFilter(-1)
    {
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setFilterKeyColumn(DanmuViewModel<DanmuComment *>::TEXT);
        setSortRole(DanmuViewModel<DanmuComment *>::SortRole);
    }

public:
    void setSourceId(int sid)
    {
        sourceId = sid;
        invalidateFilter();
    }
    void setTypeFilter(int type)
    {
        if (typeFilter == type) return;
        typeFilter = type;
        invalidateFilter();
    }
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
    {
        QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
        int sid = index.data(DanmuViewModel<DanmuComment *>::Roles::SourceRole).toInt();
        if(sourceId !=-1 && sid != sourceId) return false;
        if (index.data(DanmuViewModel<DanmuComment *>::Roles::ClippedRole).toBool()) return false;
        if (typeFilter != -1 && index.data(DanmuViewModel<DanmuComment *>::TypeRole).toInt() != typeFilter) return false;
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    }
private:
    int sourceId;
    int typeFilter;
};
#endif // DANMUVIEWMODEL_H

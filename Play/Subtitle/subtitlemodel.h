#ifndef SUBTITLEMODEL_H
#define SUBTITLEMODEL_H

#include <QAbstractItemModel>
#include "subitem.h"

class SubtitleModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SubtitleModel(QObject *parent = nullptr);

    QModelIndex getTimeIndex(qint64 time);
    SubItem getSub(int row);

    bool saveCurSRTSub(const QString &path);
    bool hasSub() const { return _hasSub; }
    const SubFile &curSub() const { return curSubFile; }

    void setBlocked(bool on);

private:
    SubFile curSubFile;
    bool blocked{false};
    bool needRefresh{false};
    bool _hasSub{false};

    void refreshSub();
    void reset();    

    // QAbstractItemModel interface
public:
    enum struct Columns
    {
        TEXT,
        START,
        END,
    };

    inline virtual int columnCount(const QModelIndex &) const {return 3;}
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const { return parent.isValid() ? QModelIndex() : createIndex(row,column); }
    virtual QModelIndex parent(const QModelIndex &child) const { return QModelIndex(); }
    virtual int rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : curSubFile.items.size(); }
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // SUBTITLEMODEL_H

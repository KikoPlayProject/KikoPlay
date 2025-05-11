#ifndef SUBTITLEEDITMODEL_H
#define SUBTITLEEDITMODEL_H

#include "subitem.h"
#include <QAbstractItemModel>

class SubtitleEditModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SubtitleEditModel(bool enableTranslate, SubFile &subFile, QObject *parent = nullptr);
    void setEnableTranslate(bool on);
    bool translated() const { return _translate; }
    SubFile getTranslatedSubFile(bool onlyTranslated, bool translatedFirst = true);

    void removeItems(QVector<int> rows);
private:
    SubFile &_subFile;
    bool _translate;
    QStringList _translateItems;

    // QAbstractItemModel interface
public:
    enum struct Columns
    {
        START,
        END,
        TEXT,
        TEXT_TRANSLATED,
    };

    inline virtual int columnCount(const QModelIndex &) const { return _translate ? 4 : 3;}
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const { return parent.isValid() ? QModelIndex() : createIndex(row,column); }
    virtual QModelIndex parent(const QModelIndex &child) const { return QModelIndex(); }
    virtual int rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : _subFile.items.size(); }
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // SUBTITLEEDITMODEL_H

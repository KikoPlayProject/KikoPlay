#ifndef LABELMODEL_H
#define LABELMODEL_H
#include <QAbstractItemModel>
#include <QtCore>
class AnimeLibrary;
class LabelModel : public QAbstractItemModel
{
public:
    explicit LabelModel(AnimeLibrary *library = nullptr);
    void refreshLabel();
    void removeTag(const QModelIndex &index);
    void selLabelList(const QModelIndexList &indexes, QStringList &tags, QSet<QString> &times);
    inline const QMap<QString,QSet<QString> > getTags(){return tagMap;}
    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    inline virtual int columnCount(const QModelIndex &) const {return 1;}
    virtual QVariant data(const QModelIndex &index, int role) const;
private:
    QMap<QString,QSet<QString> > tagMap;
    QList<QPair<QString,int> > tagList,timeList;

};

#endif // LABELMODEL_H

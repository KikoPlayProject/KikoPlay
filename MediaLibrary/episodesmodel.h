#ifndef EPISODESMODEL_H
#define EPISODESMODEL_H
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include "animeinfo.h"
class EpItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    QStringList *epNames;
public:
    EpItemDelegate(QStringList *eps,QObject *parent = 0):QStyledItemDelegate(parent),epNames(eps){}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    inline void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const override
    {
        editor->setGeometry(option.rect);
    }
};
class EpisodesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit EpisodesModel(Anime *anime, QObject *parent = nullptr);
    void setAnime(Anime *anime);
    bool episodeChanged;
private:
    Anime *currentAnime;
    const QStringList headers={tr("Title"),tr("Last Play"),tr("LocalFile")};
    void updatePath(const QString &oldPath,const QString &newPath);
    void updateTitle(const QString &path,const QString &title);
public slots:
    void addEpisode(const QString &title,const QString &path);
    void removeEpisodes(const QModelIndexList &removeIndexes);
    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:(currentAnime?currentAnime->eps.count():0);}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:3;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    inline virtual Qt::ItemFlags flags(const QModelIndex &index) const{return (index.isValid()&&index.column()!=1)?QAbstractItemModel::flags(index)|Qt::ItemIsEditable:QAbstractItemModel::flags(index);}
};

#endif // EPISODESMODEL_H

#ifndef EPISODEITEM_H
#define EPISODEITEM_H
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QWidget>
#include "animeinfo.h"

class EpItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    Anime *curAnime;
public:
    EpItemDelegate(QObject *parent = 0):QStyledItemDelegate(parent),curAnime(nullptr){}

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void setAnime(Anime *anime) {curAnime = anime;}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    inline void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const override
    {
        editor->setGeometry(option.rect);
    }
};

#endif // EPISODEITEM_H

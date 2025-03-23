#ifndef ANIMEITEMDELEGATE_H
#define ANIMEITEMDELEGATE_H
#include <QStyledItemDelegate>
#include <QPen>
class AnimeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AnimeItemDelegate(QObject *parent = nullptr);
    static int CoverWidth, CoverHeight, TitleHeight;
    void setBlockCoverFetch(bool on) { blockCoverFetch = on; }
    void setBorderColor(const QColor &color);

    // QAbstractItemDelegate interface
public:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);
    virtual QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
signals:
    void ItemClicked(const QModelIndex &index);
private:
    bool blockCoverFetch;
    QFont titleFont;
    QColor titleColor{220, 220, 220};
    QPixmap nullCoverPixmap;
    QPen borderPen;
};

#endif // ANIMEITEMDELEGATE_H

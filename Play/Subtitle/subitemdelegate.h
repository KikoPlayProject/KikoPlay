#ifndef SUBITEMDELEGATE_H
#define SUBITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTextLayout>

class SubItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SubItemDelegate(QObject *parent = nullptr);

    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void reset() { indexSize.clear(); }
private:
    const int marginX = 2, marginY = 4;
    mutable QHash<int, QSize> indexSize;
    double setTextLayout(const QModelIndex &index, QList<QSharedPointer<QTextLayout>> &layout, int maxWidth) const;
    double setTimeLayout(const QModelIndex &index, QTextLayout &layout, int maxWidth) const;
};

#endif // SUBITEMDELEGATE_H

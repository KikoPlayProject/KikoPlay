#ifndef KTREEVIEWITEMDELEGATE_H
#define KTREEVIEWITEMDELEGATE_H

#include <QStyledItemDelegate>

class KTreeviewItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit KTreeviewItemDelegate(QObject *parent = nullptr);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // KTREEVIEWITEMDELEGATE_H

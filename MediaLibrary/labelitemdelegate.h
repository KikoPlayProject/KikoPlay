#ifndef LABELITEMDELEGATE_H
#define LABELITEMDELEGATE_H
#include <QStyledItemDelegate>
class LabelItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit LabelItemDelegate(QObject* parent = nullptr);
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // LABELITEMDELEGATE_H

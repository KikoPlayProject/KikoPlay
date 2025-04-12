#include "ktreeviewitemdelegate.h"
#include <QPainter>

KTreeviewItemDelegate::KTreeviewItemDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{

}

void KTreeviewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant itemFg = index.data(Qt::ForegroundRole);
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption, index);
    if (itemFg.isValid())
    {
        QColor itemForegroundColor = itemFg.value<QColor>();
        if (itemForegroundColor != option.palette.color(QPalette::WindowText))
            viewOption.palette.setColor(QPalette::HighlightedText, itemForegroundColor);

    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QRect itemRect = option.rect;

    if(index.column() == 0) itemRect.setLeft(0);
    painter->setBrush(QColor(255, 255, 255, 40));
    painter->setPen(Qt::NoPen);

    if (option.state & QStyle::State_Selected)
    {
        painter->drawRect(itemRect);
    }
    else
    {
        if (option.state & QStyle::State_MouseOver)
        {
            painter->drawRect(itemRect);
        }
    }
    painter->restore();

    QStyledItemDelegate::paint(painter, viewOption, index);
}

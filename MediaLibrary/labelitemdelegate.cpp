#include "labelitemdelegate.h"
#include <QPainter>
#define TYPELEVEL 1
#define LABEL_TIME 2
#define LABEL_TAG 3
#define CountRole Qt::UserRole+1

LabelItemDelegate::LabelItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void LabelItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem ViewOption(option);
    QColor itemForegroundColor = index.data(Qt::ForegroundRole).value<QColor>();
    if (itemForegroundColor.isValid())
    {
        if (itemForegroundColor != option.palette.color(QPalette::WindowText))
            ViewOption.palette.setColor(QPalette::HighlightedText, itemForegroundColor);

    }
    QStyledItemDelegate::paint(painter, ViewOption, index);
    if(index.internalId()==TYPELEVEL) return;
    QRect decoration = option.rect;
    decoration.setHeight(decoration.height()-10);
    decoration.setWidth(decoration.width()/6);
    decoration.moveCenter(option.rect.center());
    decoration.moveRight(option.rect.width());

    static QPen decorationPen(QColor(220,220,220));
    static QBrush decorationBrush(QColor( 126, 135, 147 ));
    static QFont decorationFont("Microsoft Yahei UI", 9);
    painter->setFont(decorationFont);
    painter->setPen(decorationPen);
    QPainterPath path;
    path.addRoundedRect(decoration, 14, 14);
    painter->fillPath(path, decorationBrush);
    int count=index.data(CountRole).toInt();
    painter->drawText(decoration, Qt::AlignCenter, count>999?"999+":QString::number(count));

}

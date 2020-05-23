#include "labelitemdelegate.h"
#include <QPainter>
#include "globalobjects.h"
#define TYPELEVEL 1
#define LABEL_YEAR 2
#define LABEL_MONTH 3
#define LABEL_TAG 4
#define CountRole Qt::UserRole+1
#define TypeRole Qt::UserRole+2

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
    if(index.data(TypeRole)==TYPELEVEL) return;

    static QFont decorationFont("Microsoft Yahei UI", 9);
    painter->setFont(decorationFont);
    painter->setPen(QPen(penColor));

    QRect decoration = option.rect;
    decoration.setHeight(decoration.height()-10);
    int decorationWidth = painter->fontMetrics().width(QStringLiteral("00000"));
    decoration.setWidth(decorationWidth);
    decoration.moveCenter(option.rect.center());
    decoration.moveRight(option.rect.width());


    QPainterPath path;
    path.addRoundedRect(decoration, 4, 4);
    painter->fillPath(path, QBrush(brushColor));
    int count=index.data(CountRole).toInt();
    painter->drawText(decoration, Qt::AlignCenter, count>999?"999+":QString::number(count));

}

void LabelItemDelegate::setPenColor(const QColor &color)
{
    penColor = color;
}

void LabelItemDelegate::setBrushColor(const QColor &color)
{
    brushColor = color;
}

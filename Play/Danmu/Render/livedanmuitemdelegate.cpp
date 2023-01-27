#include "livedanmuitemdelegate.h"
#include <QPainter>
#define AlignmentRole Qt::UserRole+1

LiveDanmuItemDelegate::LiveDanmuItemDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{

}

void LiveDanmuItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap);
    textOption.setAlignment(static_cast<Qt::Alignment>(index.data(AlignmentRole).toInt()));
    QTextLayout layout(text, index.data(Qt::FontRole).value<QFont>());
    layout.setTextOption(textOption);
    int marginY = option.fontMetrics.height() / 4, marginX = option.fontMetrics.height() / 4;
    QSizeF textRect = textLayout(layout, text, option.rect.width() - 2 * marginX);
    painter->save();
    static QBrush bgBrush(QColor(0, 0, 0, 90));
    painter->setBrush(bgBrush);
    painter->setPen(QPen(Qt::NoPen));
    int boxX = option.rect.x();
    int boxWidth = textRect.width() + marginX * 2;
    if(textOption.alignment() == Qt::AlignRight)
    {
        boxX = option.rect.width() - boxWidth;
    }
    painter->drawRoundedRect(boxX, option.rect.y() + marginY / 2, boxWidth, textRect.height() + marginY, 8, 8);
    painter->setPen(index.data(Qt::ForegroundRole).value<QColor>());
    layout.draw(painter, QPointF(option.rect.x() + marginX, option.rect.y() + marginY));
    painter->restore();
}

QSize LiveDanmuItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    int marginY = option.fontMetrics.height() / 4, marginX = option.fontMetrics.height() / 4;
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap);
    textOption.setAlignment(static_cast<Qt::Alignment>(index.data(AlignmentRole).toInt()));
    QTextLayout layout(text, index.data(Qt::FontRole).value<QFont>());
    layout.setTextOption(textOption);
    QSizeF textRect = textLayout(layout, text, option.rect.width() - 2 * marginX);
    return QSize(option.rect.width(), textRect.height() + 2 * marginY);
}

QSizeF LiveDanmuItemDelegate::textLayout(QTextLayout &layout, const QString &text, int maxWidth) const
{
    double height = 0.0,  widthUsed = 0.0;
    layout.beginLayout();
    int i = 0;
    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(maxWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
        ++i;
    }
    layout.endLayout();
    return QSizeF(widthUsed, height);
}

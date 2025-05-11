#include "subitemdelegate.h"
#include "subtitlemodel.h"
#include <QPainter>

SubItemDelegate::SubItemDelegate(QObject *parent) : QStyledItemDelegate{parent}
{

}

void SubItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() != (int)SubtitleModel::Columns::TEXT)
    {
        return QStyledItemDelegate::paint(painter, option, index);
    }
    QList<QSharedPointer<QTextLayout>> layout;
    setTextLayout(index, layout, option.rect.width() - 4);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QRect itemRect = option.rect;

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

    for (auto &line : layout)
    {
        line->draw(painter, QPointF(option.rect.x() + marginX, option.rect.y() + marginY) + line->position());
    }
}

QSize SubItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() != (int)SubtitleModel::Columns::TEXT)
    {
        return QStyledItemDelegate::sizeHint(option, index);
    }
    const int curWidth = option.rect.width();
    if (!indexSize.contains(index.row()) || indexSize[index.row()].width() != curWidth)
    {
        QList<QSharedPointer<QTextLayout>> layout;
        double itemHeight = setTextLayout(index, layout, curWidth - marginX * 2);
        indexSize[index.row()] = QSize(curWidth, itemHeight);
    }
    return QSize(curWidth, indexSize[index.row()].height() + marginY * 2);
}

double SubItemDelegate::setTextLayout(const QModelIndex &index, QList<QSharedPointer<QTextLayout>> &layout, int maxWidth) const
{
    QSharedPointer<QTextLayout> timeLayout = QSharedPointer<QTextLayout>::create();
    layout.append(timeLayout);

    double height = setTimeLayout(index, *timeLayout, maxWidth);
    height += 2;

    QString subText = index.data(Qt::DisplayRole).toString();
    QStringList subTextLines = subText.split('\n');

    for (const QString &line : subTextLines)
    {
        QSharedPointer<QTextLayout> lineLayout = QSharedPointer<QTextLayout>::create(line);
        layout.append(lineLayout);

        QTextOption textOption;
        textOption.setWrapMode(QTextOption::WordWrap);
        lineLayout->setTextOption(textOption);

        QTextLayout::FormatRange textFormat;
        textFormat.start = 0;
        textFormat.length = line.length();
        textFormat.format.setFontPointSize(12);
        textFormat.format.setForeground(QColor(255, 255, 255, 200));
        lineLayout->setFormats({textFormat});

        lineLayout->beginLayout();
        while (true)
        {
            QTextLine line = lineLayout->createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(maxWidth);
            line.setPosition(QPointF(0, height));
            height += line.height();
        }
        lineLayout->endLayout();
    }

    return height;
}

double SubItemDelegate::setTimeLayout(const QModelIndex &index, QTextLayout &layout, int maxWidth) const
{
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::NoWrap);
    layout.setTextOption(textOption);

    QString timeStart = index.siblingAtColumn((int)SubtitleModel::Columns::START).data(Qt::DisplayRole).toString();
    QString timeEnd = index.siblingAtColumn((int)SubtitleModel::Columns::START).data(Qt::DisplayRole).toString();
    QString subTime = QString("%1 -> %2").arg(timeStart, timeEnd);

    QTextLayout::FormatRange timeFormat;
    timeFormat.start = 0;
    timeFormat.length = subTime.length();
    timeFormat.format.setFontPointSize(10);
    timeFormat.format.setForeground(QColor(255, 255, 255, 100));

    layout.setFormats({timeFormat});
    layout.setText(subTime);

    double height = 0.0;
    layout.beginLayout();
    while (true)
    {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(maxWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
    }
    layout.endLayout();

    return height;
}

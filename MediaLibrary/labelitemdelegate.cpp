#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QLineEdit>
#include "tagnode.h"
#include "globalobjects.h"
#include "labelitemdelegate.h"

#define CountRole Qt::UserRole+1
#define TypeRole Qt::UserRole+2
#define TagNodeRole Qt::UserRole+3

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
    const TagNode *tag = (const TagNode *)index.data(TagNodeRole).value<void *>();
    if(tag->tagType == TagNode::TAG_ROOT_CATE)
    {
        if(tag->subNodes && !tag->subNodes->empty() && tag->subNodes->first()->tagType == TagNode::TAG_CUSTOM)
        {
            GlobalObjects::iconfont->setPointSize(12);
            painter->setFont(*GlobalObjects::iconfont);
            tagSearchIconRect = painter->boundingRect(option.rect, QChar(0xe609));
            tagSearchIconRect.moveCenter(option.rect.center());
            tagSearchIconRect.moveRight(option.rect.right() - 6);
            painter->drawText(tagSearchIconRect, Qt::AlignCenter, QChar(0xe609));
        }
        return;
    }
    if(tag->tagType == TagNode::TAG_CUSTOM && tag->subNodes) return;

    static QFont decorationFont(GlobalObjects::normalFont, 9);
    painter->setFont(decorationFont);
    painter->setPen(QPen(penColor));

    QRect decoration = option.rect;
    decoration.setHeight(decoration.height()-10);
    int decorationWidth = painter->fontMetrics().horizontalAdvance(QStringLiteral("00000"));
    if(decorationWidth > decoration.width()/3) return;
    decoration.setWidth(decorationWidth);
    decoration.moveCenter(option.rect.center());
    decoration.moveRight(option.rect.right()-6);

    painter->fillRect(decoration, brushColor);
    int count=index.data(CountRole).toInt();
    painter->drawText(decoration, Qt::AlignCenter, count>999?"999+":QString::number(count));
}

bool LabelItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    QMouseEvent *pEvent = static_cast<QMouseEvent *> (event);
    switch (event->type())
    {
    case QEvent::MouseButtonPress:
    {
        if(pEvent->button()==Qt::LeftButton && tagSearchIconRect.contains(pEvent->pos()))
        {
            emit openTagSearchEditor(index);
        }
        break;
    }
    default:
        break;
    }
    return QStyledItemDelegate::editorEvent(event,model,option,index);
}

QWidget *LabelItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const TagNode *tag = (const TagNode *)index.data(TagNodeRole).value<void *>();
    if(tag->tagType == TagNode::TAG_ROOT_CATE)
    {
        if(tag->subNodes && !tag->subNodes->empty() && tag->subNodes->first()->tagType == TagNode::TAG_CUSTOM)
        {
            QLineEdit *editor = new QLineEdit(parent);
            QObject::connect(editor, &QLineEdit::textChanged, this, [=](const QString &text){
                emit tagSearchTextChanged(text, index);
            });
            return editor;
        }
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void LabelItemDelegate::setPenColor(const QColor &color)
{
    penColor = color;
}

void LabelItemDelegate::setBrushColor(const QColor &color)
{
    brushColor = color;
}

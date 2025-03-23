#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QWidgetAction>
#include "UI/widgets/klineedit.h"
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
    QStyleOptionViewItem viewOption(option);
    QColor itemForegroundColor = index.data(Qt::ForegroundRole).value<QColor>();
    if (itemForegroundColor.isValid())
    {
        if (itemForegroundColor != option.palette.color(QPalette::WindowText))
            viewOption.palette.setColor(QPalette::HighlightedText, itemForegroundColor);

    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QRect itemRect = viewOption.rect;
    itemRect.setLeft(0);
    painter->setBrush(QColor(255, 255, 255, 40));
    painter->setPen(Qt::NoPen);
    if (viewOption.state & QStyle::State_Selected)
    {
        painter->drawRoundedRect(itemRect, 2, 2);
    }
    else
    {
        if (viewOption.state & QStyle::State_MouseOver)
        {
            painter->drawRoundedRect(itemRect, 2, 2);
        }
    }
    painter->restore();
    QStyledItemDelegate::paint(painter, viewOption, index);


    const TagNode *tag = (const TagNode *)index.data(TagNodeRole).value<void *>();
    if (!tag) return;
    if (tag->tagType == TagNode::TAG_ROOT_CATE)
    {
        if (tag->subNodes && !tag->subNodes->empty() && tag->subNodes->first()->tagType == TagNode::TAG_CUSTOM)
        {
            painter->save();
            GlobalObjects::iconfont->setPointSize(12);
            painter->setFont(*GlobalObjects::iconfont);
            painter->setPen(QPen(penColor));
            tagSearchIconRect = painter->boundingRect(option.rect, QChar(0xea8a));
            tagSearchIconRect.moveCenter(option.rect.center());
            tagSearchIconRect.moveRight(option.rect.right() - 6);
            painter->drawText(tagSearchIconRect, Qt::AlignCenter, QChar(0xea8a));
            painter->restore();
        }
        return;
    }
    if (tag->tagType == TagNode::TAG_CUSTOM && tag->subNodes) return;

    static QFont decorationFont(GlobalObjects::normalFont, 9);
    painter->save();
    painter->setFont(decorationFont);

    QRect decoration = option.rect;
    decoration.setHeight(decoration.height()-10);
    int decorationWidth = painter->fontMetrics().horizontalAdvance(QStringLiteral("00000"));
    if(decorationWidth > decoration.width()/3) return;
    decoration.setWidth(decorationWidth);
    decoration.moveCenter(option.rect.center());
    decoration.moveRight(option.rect.right()-6);

    painter->setPen(Qt::NoPen);
    painter->setBrush(brushColor);
    painter->drawRoundedRect(decoration, 2, 2);

    int count=index.data(CountRole).toInt();
    painter->setPen(QPen(penColor));

    painter->drawText(decoration, Qt::AlignCenter, count>999?"999+":QString::number(count));
    painter->restore();
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
            QLineEdit *editor = new KLineEdit(parent);
            editor->setObjectName(QStringLiteral("TagFilterLineEdit"));
            QPushButton *clearBtn = new QPushButton(editor);
            clearBtn->setObjectName(QStringLiteral("ListSearchButton"));
            GlobalObjects::iconfont->setPointSize(12);
            clearBtn->setFont(*GlobalObjects::iconfont);
            clearBtn->setText(QChar(0xe60b));
            clearBtn->setCursor(Qt::ArrowCursor);
            clearBtn->setFocusPolicy(Qt::NoFocus);
            QWidgetAction *clearAction = new QWidgetAction(editor);
            clearAction->setDefaultWidget(clearBtn);
            editor->addAction(clearAction, QLineEdit::TrailingPosition);
            QObject::connect(editor, &QLineEdit::textChanged, this, [=](const QString &text){
                emit tagSearchTextChanged(text, index);
            });
            QObject::connect(clearBtn, &QPushButton::clicked, this, [=](){
                emit tagSearchTextChanged("", index);
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

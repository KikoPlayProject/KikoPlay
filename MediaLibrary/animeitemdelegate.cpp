#include "animeitemdelegate.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QMouseEvent>
#include <QToolTip>
#include "animeinfo.h"
#include "globalobjects.h"
#define AnimeRole Qt::UserRole+1
int AnimeItemDelegate::CoverWidth = 0;
int AnimeItemDelegate::CoverHeight = 0;
int AnimeItemDelegate::TitleHeight = 0;

AnimeItemDelegate::AnimeItemDelegate(QObject *parent):QStyledItemDelegate(parent), blockCoverFetch(false)
{
    AnimeItemDelegate::CoverWidth = 153;
    AnimeItemDelegate::CoverHeight = 208;
    titleFont.setFamily(GlobalObjects::normalFont);
    titleFont.setPointSize(11);
    AnimeItemDelegate::TitleHeight = QFontMetrics(titleFont).height() + 4;

    const float pxR = GlobalObjects::context()->devicePixelRatioF;
    nullCoverPixmap = QPixmap(AnimeItemDelegate::CoverWidth*pxR, AnimeItemDelegate::CoverHeight*pxR);
    nullCoverPixmap.setDevicePixelRatio(pxR);
    nullCoverPixmap.fill(Qt::transparent);
    QPainter painter(&nullCoverPixmap);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
    QPainterPath path;
    path.addRoundedRect(0, 0, AnimeItemDelegate::CoverWidth, AnimeItemDelegate::CoverHeight, 8, 8);
    painter.setClipPath(path);
    painter.drawPixmap(QRect(0, 0, AnimeItemDelegate::CoverWidth, AnimeItemDelegate::CoverHeight), QPixmap(":/res/images/null-cover.png"));

    borderPen.setWidth(2);
    borderPen.setColor(QColor(28, 160, 228));
}

void AnimeItemDelegate::setBorderColor(const QColor &color)
{
    borderPen.setColor(color);
}


void AnimeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption,index);
    Anime *anime = (Anime *)index.data(AnimeRole).value<void *>();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    QRect coverRect(option.rect.x(), option.rect.y(), AnimeItemDelegate::CoverWidth, AnimeItemDelegate::CoverHeight);
    coverRect.adjust(6, 6, -6, -6);
    QPixmap coverPixmap(anime->cover(blockCoverFetch));
    painter->drawPixmap(coverRect, coverPixmap.isNull() ? nullCoverPixmap : coverPixmap);
    if (viewOption.state.testFlag(QStyle::State_MouseOver))
    {
        painter->save();
        painter->setPen(borderPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(coverRect, 8, 8);
        painter->restore();
    }

    QRect titleRect(option.rect.x(), option.rect.y() + AnimeItemDelegate::CoverHeight, AnimeItemDelegate::CoverWidth, AnimeItemDelegate::TitleHeight);
    titleRect.adjust(6, 0, -6, 0);
    painter->setFont(titleFont);
    painter->setPen(titleColor);
    QString drawText = index.data(Qt::DisplayRole).toString();
    if (painter->fontMetrics().horizontalAdvance(drawText) > titleRect.width())
    {
        drawText = painter->fontMetrics().elidedText(drawText, Qt::ElideRight, titleRect.width());
    }
    painter->drawText(titleRect, Qt::AlignCenter, drawText);
    painter->restore();
}

bool AnimeItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    QMouseEvent *pEvent = static_cast<QMouseEvent *> (event);
    switch (event->type())
    {
    case QEvent::MouseMove:
    {
        QRect titleRect(option.rect.x(), option.rect.y() + AnimeItemDelegate::CoverHeight, AnimeItemDelegate::CoverWidth, AnimeItemDelegate::TitleHeight);
        if (titleRect.contains(pEvent->pos()))
            QToolTip::showText(pEvent->globalPos(), index.model()->data(index,Qt::DisplayRole).toString());
        else
            QToolTip::hideText();
        break;
    }
    case QEvent::MouseButtonPress:
    {
        if (pEvent->button() == Qt::LeftButton && !(pEvent->modifiers()&Qt::ControlModifier)
                && !(pEvent->modifiers()&Qt::ShiftModifier))
            emit ItemClicked(index);
        break;
    }
    default:
        break;
    }
    return QStyledItemDelegate::editorEvent(event,model,option,index);
}

QSize AnimeItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(CoverWidth, CoverHeight+TitleHeight);
}

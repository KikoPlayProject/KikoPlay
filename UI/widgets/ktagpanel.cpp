#include "ktagpanel.h"
#include "globalobjects.h"
#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QToolTip>
#include <QHelpEvent>
#include <QSvgRenderer>

KTagPanel::KTagPanel(QWidget *parent, int fontSize) : QWidget(parent)
{
    setMouseTracking(true);
    QFont f = font();
    f.setPointSizeF(GlobalObjects::fontSize(fontSize));
    setFont(f);

    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
}

KTagPanel::TagItem &KTagPanel::addTag(const QString &text, const QString &tooltip, const QIcon &icon, const QColor &bgColor, const QColor &textColor)
{
    TagItem item;
    item.text = text;
    item.bgColor = bgColor;
    item.textColor = textColor;
    item.icon = icon;
    item.tooltip = tooltip;

    return addTag(item);
}

KTagPanel::TagItem &KTagPanel::addTag(const TagItem &item)
{
    m_tags.append(item);

    updateGeometry();
    doLayout(width(), true);
    update();

    return m_tags.back();
}

void KTagPanel::clearTags()
{
    m_tags.clear();
    updateGeometry();
    doLayout(width(), true);
    update();
}


bool KTagPanel::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        for (const auto& tag : m_tags)
        {
            if (tag.rect.contains(helpEvent->pos()))
            {
                if (!tag.tooltip.isEmpty())
                {
                    QToolTip::showText(helpEvent->globalPos(), tag.tooltip, this, tag.rect);
                } else {
                    QToolTip::hideText();
                }
                return true;
            }
        }
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    return QWidget::event(event);
}

void KTagPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (event->size().width() != event->oldSize().width())
    {
        doLayout(event->size().width(), true);
    }
}

void KTagPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QFontMetrics fm(font());
    int iconSize = fm.height() - 2;

    for (const auto& tag : m_tags)
    {
        QColor currentBgColor = tag.isHovered ? tag.bgColor.lighter(110) : tag.bgColor;
        painter.setPen(Qt::NoPen);
        painter.setBrush(currentBgColor);
        painter.drawRoundedRect(tag.rect, m_radius, m_radius);

        int currentX = tag.rect.left() + m_paddingX;

        if (!tag.icon.isNull())
        {
            QRect iconRect(currentX, tag.rect.top() + (tag.rect.height() - iconSize) / 2, iconSize, iconSize);
            tag.icon.paint(&painter, iconRect);
            currentX += iconSize + m_iconSpacing;
        }

        painter.setPen(tag.textColor);
        QRect textRect(currentX, tag.rect.top(), fm.horizontalAdvance(tag.text), tag.rect.height());
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, tag.text);
    }
}

void KTagPanel::mouseMoveEvent(QMouseEvent *event)
{
    bool needUpdate = false;
    bool isHoveringAnyTag = false;

    for (auto& tag : m_tags)
    {
        bool isHit = tag.rect.contains(event->pos());
        if (isHit) isHoveringAnyTag = true;

        if (tag.isHovered != isHit)
        {
            tag.isHovered = isHit;
            needUpdate = true;
        }
    }

    setCursor(isHoveringAnyTag ? Qt::PointingHandCursor : Qt::ArrowCursor);

    if (needUpdate) update();
}

void KTagPanel::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    bool needUpdate = false;
    for (auto& tag : m_tags)
    {
        if (tag.isHovered)
        {
            tag.isHovered = false;
            needUpdate = true;
        }
    }
    setCursor(Qt::ArrowCursor);
    if (needUpdate) update();
}

void KTagPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        for (int i = 0; i < m_tags.size(); ++i)
        {
            if (m_tags[i].rect.contains(event->pos()))
            {
                emit tagClicked(i);
                break;
            }
        }
    }
    QWidget::mouseReleaseEvent(event);
}

QSize KTagPanel::doLayout(int targetWidth, bool applyRects) const
{
    if (m_tags.isEmpty()) return QSize(0, 0);

    QFontMetrics fm(font());
    int iconSize = fm.height() - 2;

    int x = m_spacing;
    int y = m_vspacing;
    int maxRowHeight = 0;
    int actualMaxWidth = 0;

    for (int i = 0; i < m_tags.size(); ++i)
    {
        const auto& tag = m_tags[i];

        int tagWidth = m_paddingX * 2;
        if (!tag.icon.isNull())
        {
            tagWidth += iconSize + m_iconSpacing;
        }
        tagWidth += fm.horizontalAdvance(tag.text);
        int tagHeight = fm.height() + m_paddingY * 2;

        if (x + tagWidth + m_spacing > targetWidth && x > m_spacing)
        {
            x = m_spacing;
            y += maxRowHeight + m_vspacing;
            maxRowHeight = 0;
        }

        if (applyRects)
        {
            TagItem* mutableTag = const_cast<TagItem*>(&m_tags[i]);
            mutableTag->rect = QRect(x, y, tagWidth, tagHeight);
        }

        maxRowHeight = qMax(maxRowHeight, tagHeight);
        x += tagWidth + m_spacing;
        actualMaxWidth = qMax(actualMaxWidth, x);
    }

    return QSize(actualMaxWidth, y + maxRowHeight + m_vspacing);
}



void KTagPanel::TagItem::setIconFromSvgStr(const QString &svgStr, const QSize &renderSize)
{
    if (svgStr.isEmpty()) return;
    QSvgRenderer renderer(svgStr.toUtf8());

    if (!renderer.isValid()) return;

    QPixmap pixmap(renderSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    painter.end();

    icon = QIcon(pixmap);
}

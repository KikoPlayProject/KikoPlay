#include "poolitemdelegate.h"
#include "Extension/Script/danmuscript.h"
#include "Extension/Script/scriptmanager.h"
#include "Play/Danmu/Manager/managermodel.h"
#include "globalobjects.h"
#include <QPainter>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QToolTip>
#include <QTreeView>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>


PoolItemDelegate::PoolItemDelegate(QObject *parent) : KTreeviewItemDelegate(parent) {}

QSize PoolItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return KTreeviewItemDelegate::sizeHint(option, index);
}

void PoolItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KTreeviewItemDelegate::paint(painter, option, index);
    if (!index.isValid()) return;
    if (index.column() == (int)DanmuManagerModel::Columns::SOURCE)
    {
        const DanmuPoolNode *node = (const DanmuPoolNode *)index.data(DanmuManagerModel::DataRole::PoolNodeRole).value<void *>();
        if (!node || node->type != DanmuPoolNode::SourecNode) return;

        auto srcNode = static_cast<const DanmuPoolSourceNode *>(node);
        drawTags(painter, srcNode, option);
    }
    else if (index.column() == (int)DanmuManagerModel::Columns::DELAY)
    {
        const DanmuPoolNode *node = (const DanmuPoolNode *)index.data(DanmuManagerModel::DataRole::PoolNodeRole).value<void *>();
        if (!node || node->type != DanmuPoolNode::SourecNode) return;

        auto srcNode = static_cast<const DanmuPoolSourceNode *>(node);
        drawDelayInfo(painter, srcNode, option);
    }
}

bool PoolItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event && event->type() == QEvent::ToolTip && index.column() == (int)DanmuManagerModel::Columns::SOURCE)
    {
        const DanmuPoolNode *node = (const DanmuPoolNode *)index.data(DanmuManagerModel::DataRole::PoolNodeRole).value<void *>();
        if (node && node->type == DanmuPoolNode::SourecNode)
        {
            auto srcNode = static_cast<const DanmuPoolSourceNode *>(node);
            for (int i = 0; i < 1 + srcNode->tags.size(); ++i)
            {
                if (tagRect(srcNode, option, i).contains(event->pos()))
                {
                    if (i == 0)
                    {
                        QToolTip::showText(event->globalPos(), srcNode->url.isEmpty() ? srcNode->idInfo : srcNode->url, view);
                        return true;
                    }
                    else
                    {
                        auto &curTag = srcNode->tags[i - 1];
                        if (!curTag.tooltip.isEmpty())
                        {
                            QToolTip::showText(event->globalPos(), curTag.tooltip, view);
                            return true;
                        }
                    }
                    break;
                }
            }
        }
    }
    return KTreeviewItemDelegate::helpEvent(event, view, option, index);
}

bool PoolItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event && event->type() == QEvent::MouseButtonRelease)
    {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton)
        {
            if (index.column() == (int)DanmuManagerModel::Columns::SOURCE)
            {
                const DanmuPoolNode *node = (const DanmuPoolNode *)index.data(DanmuManagerModel::DataRole::PoolNodeRole).value<void *>();
                if (node && node->type == DanmuPoolNode::SourecNode)
                {
                    auto srcNode = static_cast<const DanmuPoolSourceNode *>(node);
                    for (int i = 0; i < 1 + srcNode->tags.size(); ++i)
                    {
                        if (tagRect(srcNode, option, i).contains(me->pos()))
                        {
                            if (i == 0)
                            {
                                if (!srcNode->url.isEmpty())
                                {
                                    QDesktopServices::openUrl(QUrl(srcNode->url));
                                    return true;
                                }
                            }
                            else
                            {
                                auto &curTag = srcNode->tags[i - 1];
                                if (!curTag.link.isEmpty())
                                {
                                    QDesktopServices::openUrl(QUrl(curTag.link));
                                    return true;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            else if (index.column() == (int)DanmuManagerModel::Columns::DELAY)
            {
                const DanmuPoolNode *node = (const DanmuPoolNode *)index.data(DanmuManagerModel::DataRole::PoolNodeRole).value<void *>();
                if (node && node->type == DanmuPoolNode::SourecNode)
                {
                    auto srcNode = static_cast<const DanmuPoolSourceNode *>(node);
                    if (srcNode->hasTimeline || srcNode->hasClip)
                    {
                        QFontMetrics fm(option.font);
                        int iconHeight = fm.height() + 2 * 2;
                        int x = option.rect.left() + 2;
                        int y = option.rect.top() + (option.rect.height() - iconHeight) / 2;
                        if (srcNode->hasTimeline)
                        {
                            if (QRect(x, y, iconHeight, iconHeight).contains(me->pos()))
                            {
                                emit delayTimelineClicked(index);
                                return true;
                            }
                        }
                        if (srcNode->hasClip)
                        {
                            if (QRect(x + (srcNode->hasTimeline ? iconHeight + 4 : 0), y, iconHeight, iconHeight).contains(me->pos()))
                            {
                                emit delayClipClicked(index);
                                return true;
                            }
                        }
                    }

                }
            }
        }
    }
    return KTreeviewItemDelegate::editorEvent(event, model, option, index);
}

void PoolItemDelegate::drawTags(QPainter *painter, const DanmuPoolSourceNode *srcNode, const QStyleOptionViewItem &option) const
{
    if (!cacheInfo.contains(srcNode->idInfo))
    {
        DrawCache cache;
        if (srcNode->isKikoSrc)
        {
            cache.name = "Kiko";
            cache.bgColor = QColor{ 19, 165, 200 };
            cache.icon = QIcon(":/res/images/kikoplay.svg");
        }
        else
        {
            auto curScript = GlobalObjects::scriptManager->getScript(srcNode->idInfo).dynamicCast<DanmuScript>();
            if (curScript)
            {
                cache.name = curScript->name();
                cache.bgColor = curScript->labelColor();
                if (!curScript->scriptIcon().isNull()) cache.icon = curScript->scriptIcon();
            }
            else
            {
                cache.name = tr("Local");
                cache.bgColor = QColor{ 43, 106, 176 };
            }
        }
        cacheInfo.insert(srcNode->idInfo, cache);
    }
    DrawCache &cache = cacheInfo[srcNode->idInfo];
    QFontMetrics fm(option.font);

    if (cache.width == -1 || cache.height == -1)
    {
        if (!cache.icon.isNull())
        {
            cache.iconSize = fm.height() - 2;
        }
        cache.width = 8 * 2 + (cache.icon.isNull() ? 0 : cache.iconSize + 2) + fm.horizontalAdvance(cache.name);
        cache.height = fm.height() + 2 * 2;
    }

    int x = option.rect.left() + 2;
    int y = option.rect.top() + (option.rect.height() - cache.height) / 2;

    painter->save();
    painter->setClipRect(option.rect);
    painter->setPen(Qt::NoPen);
    painter->setBrush(cache.bgColor);
    painter->drawRoundedRect(QRect(x, y, cache.width, cache.height), 4, 4);
    x += 8;
    if (!cache.icon.isNull())
    {
        QRect iconRect(x, y + (cache.height - cache.iconSize) / 2, cache.iconSize, cache.iconSize);
        cache.icon.paint(painter, iconRect);
        x += cache.iconSize + 2;
    }
    painter->setPen(Qt::white);
    QRect textRect(x, y, cache.width - 16 - (cache.icon.isNull() ? 0 : cache.iconSize + 2), cache.height);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, cache.name);

    x = option.rect.left() + cache.width + 4;

    for (auto &tag : srcNode->tags)
    {
        int srcX = x;
        painter->setPen(Qt::NoPen);
        painter->setBrush(tag.bgColor);
        int tagWidth = 8 * 2 + (tag.icon.isNull() ? 0 : cache.iconSize + 2) + fm.horizontalAdvance(tag.text);
        painter->drawRoundedRect(QRect(x, y, tagWidth, cache.height), 4, 4);
        x += 8;
        if (!tag.icon.isNull())
        {
            QRect iconRect(x, y + (cache.height - cache.iconSize) / 2, cache.iconSize, cache.iconSize);
            tag.icon.paint(painter, iconRect);
            x += cache.iconSize + 2;
        }
        painter->setPen(tag.textColor);
        QRect textRect(x, y, tagWidth - 16 - (tag.icon.isNull() ? 0 : cache.iconSize + 2), cache.height);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, tag.text);
        x = srcX + tagWidth + 2;
    }
    painter->restore();
}

void PoolItemDelegate::drawDelayInfo(QPainter *painter, const DanmuPoolSourceNode *srcNode, const QStyleOptionViewItem &option) const
{
    QFontMetrics fm(option.font);
    int iconHeight = fm.height() + 2 * 2;
    int x = option.rect.left() + 2;
    int y = option.rect.top() + (option.rect.height() - iconHeight) / 2;

    painter->save();
    painter->setClipRect(option.rect);
    if (srcNode->hasTimeline)
    {
        static QIcon timeLineTip = GlobalObjects::context()->getFontIcon(QChar(0xe6b5), QColor(23, 196, 237));
        QRect timelineRect(x, y, iconHeight, iconHeight);
        timelineRect.adjust(1, 1, -1, -1);
        timeLineTip.paint(painter, timelineRect);
        x += iconHeight + 4;
    }
    if (srcNode->hasClip)
    {
        static QIcon clipTip = GlobalObjects::context()->getFontIcon(QChar(0xe6ee), QColor(23, 196, 237));
        QRect clipRect(x, y, iconHeight, iconHeight);
        clipRect.adjust(2, 2, -2, -2);
        clipTip.paint(painter, clipRect);
        x += iconHeight + 4;
    }
    const QString delayStr = QString::number(srcNode->delay / 1000);
    QRect textRect(x, y, fm.horizontalAdvance(delayStr), iconHeight);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, delayStr);
    painter->restore();
}

QRect PoolItemDelegate::tagRect(const DanmuPoolSourceNode *srcNode, const QStyleOptionViewItem &option, int index) const
{
    if (!cacheInfo.contains(srcNode->idInfo)) return QRect();
    const DrawCache &cache = cacheInfo[srcNode->idInfo];
    if (cache.width <= 0 || cache.height <= 0) return QRect();

    int x = option.rect.left() + 2;
    int y = option.rect.top() + (option.rect.height() - cache.height) / 2;

    if (index == 0)
    {
        return QRect(x, y, cache.width, cache.height);
    }
    if (index > 0 && index - 1 >= srcNode->tags.size()) return QRect();

    QFontMetrics fm(option.font);
    x += cache.width + 2;
    for (int i = 0; i < srcNode->tags.size(); ++i)
    {
        auto &curTag = srcNode->tags[i];
        int curWidth = 8 * 2 + (curTag.icon.isNull() ? 0 : cache.iconSize + 2) + fm.horizontalAdvance(curTag.text);
        if (i == index - 1)
        {
            return QRect(x, y, curWidth, cache.height);
        }
        x += curWidth + 2;
    }
    return QRect();
}


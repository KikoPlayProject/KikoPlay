#ifndef POOLITEMDELEGATE_H
#define POOLITEMDELEGATE_H

#include "ktreeviewitemdelegate.h"

struct DanmuPoolSourceNode;
class PoolItemDelegate : public KTreeviewItemDelegate
{
    Q_OBJECT
public:
    explicit PoolItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

signals:
    void delayTimelineClicked(const QModelIndex &index);
    void delayClipClicked(const QModelIndex &index);

private:
    void drawTags(QPainter *painter, const DanmuPoolSourceNode *srcNode, const QStyleOptionViewItem &option) const;
    void drawDelayInfo(QPainter *painter, const DanmuPoolSourceNode *srcNode, const QStyleOptionViewItem &option) const;
    QRect tagRect(const DanmuPoolSourceNode *srcNode, const QStyleOptionViewItem &option, int index) const;
private:
    struct DrawCache
    {
        QString name;
        QColor bgColor;
        QIcon icon;
        int width = -1;
        int height = -1;
        int iconSize = 0;
    };
    mutable QHash<QString, DrawCache> cacheInfo;
};

#endif // POOLITEMDELEGATE_H

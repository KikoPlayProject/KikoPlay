#ifndef DOWNLOADITEMDELEGATE_H
#define DOWNLOADITEMDELEGATE_H
#include <QStyledItemDelegate>

class DownloadItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DownloadItemDelegate(QObject *parent = nullptr);
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // DOWNLOADITEMDELEGATE_H

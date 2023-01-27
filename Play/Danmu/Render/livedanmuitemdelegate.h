#ifndef LIVEDANMUITEMDELEGATE_H
#define LIVEDANMUITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTextLayout>

class LiveDanmuItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit LiveDanmuItemDelegate(QObject *parent = nullptr);

    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
    QSizeF textLayout(QTextLayout &layout, const QString &text, int maxWidth) const;
};

#endif // LIVEDANMUITEMDELEGATE_H

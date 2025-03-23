#ifndef DOWNLOADITEMDELEGATE_H
#define DOWNLOADITEMDELEGATE_H
#include <QStyledItemDelegate>

class QProgressBar;
class DownloadItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DownloadItemDelegate(QObject *parent = nullptr);
    virtual ~DownloadItemDelegate();
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // QAbstractItemDelegate interface
public:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    const int textSpacing{6};
    const int progressHeight{6};
    const int marginTop{8};
    const int marginBottom{10};
    const int marginLeft{12};
    const int marginRight{12};
    const int titleFontSize{12};
    const int statusFontSize{10};

    int itemHeight{0};

    QProgressBar *downloadProgress{nullptr};

};

#endif // DOWNLOADITEMDELEGATE_H

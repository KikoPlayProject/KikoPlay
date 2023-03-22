#ifndef LABELITEMDELEGATE_H
#define LABELITEMDELEGATE_H
#include <QStyledItemDelegate>
#include <QColor>
class LabelItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit LabelItemDelegate(QObject* parent = nullptr);
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void setPenColor(const QColor &color);
    void setBrushColor(const QColor &color);
signals:
    void openTagSearchEditor(const QModelIndex &index);
    void tagSearchTextChanged(const QString &text, const QModelIndex &index) const;
private:
    QColor penColor, brushColor;
    mutable QRectF tagSearchIconRect;
    mutable bool mouseOverSearchRect = false;
};

#endif // LABELITEMDELEGATE_H

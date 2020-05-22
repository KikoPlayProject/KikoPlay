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

    void setPenColor(const QColor &color);
    void setBrushColor(const QColor &color);

private:
    QColor penColor, brushColor;
};

#endif // LABELITEMDELEGATE_H

#ifndef COLORPREVIEW_H
#define COLORPREVIEW_H

#include <QWidget>

class ColorPreview : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPreview(QWidget *parent = nullptr);
    void setColor(const QColor &c);
    void setPixmap(const QPixmap &p);
    void setUseColorDialog(bool on);
    void setCheckable(bool on);
    void setChecked(bool on);
    QColor getColor() const { return color; }

signals:
    void click(bool checked);
    void colorChanged(QColor c);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    bool useColor{true};
    bool useColorDialog{false};
    QColor color;
    QPixmap pixmap;
    bool isEnter{false};
    bool checkable{false};
    bool isChecked{false};

};

#endif // COLORPREVIEW_H

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

signals:
    void click();
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

};

#endif // COLORPREVIEW_H

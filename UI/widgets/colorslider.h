#ifndef COLORSLIDER_H
#define COLORSLIDER_H
#include <QSlider>
#include <QStyle>

class ColorSlider : public QSlider
{
    Q_OBJECT
public:
    explicit ColorSlider(QWidget *parent=nullptr):QSlider(Qt::Horizontal,parent),
        hoverControl(QStyle::SC_None), pressedControl(QStyle::SC_None)
    {
        setObjectName(QStringLiteral("ColorSlider"));
    }
    void setGradient(const QLinearGradient &g) {fillGradient = g; update();}
    void setEnabled(bool on);
signals:
    void sliderClick(int pos);
    void sliderUp(int pos);
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    bool event(QEvent *event) override;
private:
    QLinearGradient fillGradient;
    QStyle::SubControl hoverControl, pressedControl;
    void updateHoverControl(const QPoint &pos);
};

#endif // COLORSLIDER_H

#ifndef OPTIONSLIDER_H
#define OPTIONSLIDER_H

#include <QSlider>
#include <QStyle>

class OptionSlider : public QSlider
{
public:
    OptionSlider(QWidget *parent = nullptr);
    void setLabels(const QStringList &labels);
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    bool event(QEvent *event) override;
private:
    QStyle::SubControl hoverControl, pressedControl;
    void updateHoverControl(const QPoint &pos);
    QStringList labels;
};

#endif // OPTIONSLIDER_H

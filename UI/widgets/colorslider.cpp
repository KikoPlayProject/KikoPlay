#include "colorslider.h"
#include <QHoverEvent>
#include <QStylePainter>
#include <QStyleOption>

void ColorSlider::setEnabled(bool on)
{
    QSlider::setEnabled(on);
    update();
}
void ColorSlider::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect grooveRect(style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove,this));
    fillGradient.setStart(0, 0);
    fillGradient.setFinalStop(grooveRect.width(), 0);
    painter.fillRect(grooveRect, QBrush(fillGradient));
    if(!isEnabled()) painter.fillRect(grooveRect, QColor(255,255,255,200));
    opt.subControls = QStyle::SC_SliderHandle | QStyle::SC_SliderGroove;
    if (pressedControl)
    {
        opt.activeSubControls = pressedControl;
        opt.state |= QStyle::State_Sunken;
    }
    else
    {
        opt.activeSubControls = hoverControl;
    }
    painter.drawComplexControl(QStyle::CC_Slider, opt);
}

void ColorSlider::mousePressEvent(QMouseEvent *ev)
{
    if ((ev->button() & style()->styleHint(QStyle::SH_Slider_AbsoluteSetButtons)) == ev->button())
    {
        pressedControl = QStyle::SC_SliderHandle;
    }
    else if ((ev->button() & style()->styleHint(QStyle::SH_Slider_PageSetButtons)) == ev->button())
    {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        pressedControl = style()->hitTestComplexControl(QStyle::CC_Slider, &opt, ev->pos(), this);
    }
    QSlider::mousePressEvent(ev);
    if(!isSliderDown() && ev->button() == Qt::LeftButton)
    {
        int dur = maximum()-minimum();
        double x=qBound<double>(0., ev->x(), width());
        int pos = minimum() + dur * (x /width());
        emit sliderClick(pos);
        setValue(pos);
    }
    ev->accept();
}

void ColorSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    pressedControl = QStyle::SC_None;

    int dur = maximum()-minimum();
    double x=qBound<double>(0., ev->x(), width());
    int pos = minimum() + dur * (x /width());
    emit sliderUp(pos);

    QSlider::mouseReleaseEvent(ev);
    ev->accept();
}

bool ColorSlider::event(QEvent *event)
{
    switch(event->type())
    {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
            updateHoverControl(he->pos());
        break;
    default:
        break;

    }
    return QSlider::event(event);
}

void ColorSlider::updateHoverControl(const QPoint &pos)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    if (handleRect.contains(pos))
    {
        hoverControl = QStyle::SC_SliderHandle;
    } else if (grooveRect.contains(pos))
    {
        hoverControl = QStyle::SC_SliderGroove;
    } else
    {
        hoverControl = QStyle::SC_None;
    }

}

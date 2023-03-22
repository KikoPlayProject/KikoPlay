#include "optionslider.h"
#include <QHoverEvent>
#include <QStylePainter>
#include <QStyleOption>


OptionSlider::OptionSlider(QWidget *parent) : QSlider(Qt::Horizontal, parent),
    hoverControl(QStyle::SC_None), pressedControl(QStyle::SC_None)
{
    setObjectName(QStringLiteral("OptionSlider"));
    QObject::connect(this, &QSlider::valueChanged, this, [=](int i){
        if(i < labels.size()) setToolTip(labels[i]);
    });
}

void OptionSlider::setLabels(const QStringList &labels)
{
    this->labels = labels;
    setMaximum(labels.size() - 1);
    setMinimum(0);
    setSingleStep(1);
    setPageStep(1);
}

void OptionSlider::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect grooveRect(style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove,this));
    QRect handleRect(style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle,this));
    opt.subControls = QStyle::SC_SliderGroove;
    painter.drawComplexControl(QStyle::CC_Slider, opt);

    int available = style()->pixelMetric(QStyle::PM_SliderSpaceAvailable, &opt, this);
    int lablePointWidth = 4*logicalDpiX()/96;
    int len = style()->pixelMetric(QStyle::PM_SliderLength, &opt, this) - handleRect.width() / 2 - lablePointWidth / 2;
    int h = grooveRect.height() + 2;
    int y = grooveRect.y() - 1;
    int fontY = grooveRect.bottom() + painter.fontMetrics().height();
    for(int i = 0; i < labels.size(); ++i)
    {
        int x = QStyle::sliderPositionFromValue(0, labels.size() - 1, i, available) + len;
        painter.fillRect(x, y, lablePointWidth, h, QColor(255, 255, 255, 160));
        int labelWidth = i == 0? 0 : painter.fontMetrics().horizontalAdvance(labels[i]);
        painter.setPen(i == value()? QColor(240, 240, 240) : QColor(120, 120, 120));
        painter.drawText(QPoint(x - (i == labels.size() - 1 ? labelWidth - lablePointWidth : labelWidth/2), fontY), labels[i]);
    }
    opt.subControls = QStyle::SC_SliderHandle;

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

void OptionSlider::mousePressEvent(QMouseEvent *ev)
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
}

void OptionSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    pressedControl = QStyle::SC_None;
    QSlider::mouseReleaseEvent(ev);
}

bool OptionSlider::event(QEvent *event)
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

void OptionSlider::updateHoverControl(const QPoint &pos)
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

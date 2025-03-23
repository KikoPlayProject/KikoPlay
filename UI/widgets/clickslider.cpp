#include "clickslider.h"
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <QStylePainter>

ClickSlider::ClickSlider(QWidget *parent) : QSlider(Qt::Horizontal, parent), mousePos(0)
{
    _style = new ClickSliderStyle(this, style());
    setStyle(_style);
}

void ClickSlider::setEventMark(const QVector<DanmuEvent> &eventList)
{
    this->eventList=eventList;
    update();
}

void ClickSlider::setChapterMark(const QVector<MPVPlayer::ChapterInfo> &chapters)
{
    this->chapters=chapters;
    update();
}

int ClickSlider::sliderLeft() const
{
    return _style->_sliderLeft;
}

int ClickSlider::sliderWidth() const
{
    return _style->_sliderWidth;
}

void ClickSlider::mousePressEvent(QMouseEvent *event)
{
    QSlider::mousePressEvent(event);
    if(!isSliderDown() && event->button() == Qt::LeftButton)
    {
        int pos = QStyle::sliderValueFromPosition(minimum(), maximum(), event->x() - _style->_sliderLeft, _style->_sliderWidth);
        emit sliderClick(pos);
        setValue(pos);
    }
    event->accept();
}

void ClickSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (isSliderDown())
    {
        int pos = QStyle::sliderValueFromPosition(minimum(), maximum(), event->x() - _style->_sliderLeft, _style->_sliderWidth);
        emit sliderUp(pos);
   }
   QSlider::mouseReleaseEvent(event);
}

void ClickSlider::mouseMoveEvent(QMouseEvent *event)
{
   mousePos = QStyle::sliderValueFromPosition(minimum(), maximum(), event->x() - _style->_sliderLeft, _style->_sliderWidth);
    QString desc;
    if (!chapters.isEmpty())
    {
        for (auto &c:chapters)
        {
            if (qAbs(mousePos - c.position) < 3000)
            {
                desc = tr("Chapter: %1").arg(c.title);
                break;
            }
        }
    }
    if (!eventList.isEmpty())
    {
        for (auto &e : eventList)
        {
            if (mousePos >= e.start - 3000)
            {
                if (mousePos <= e.start+e.duration + 3000)
                {
                    if (!desc.isEmpty()) desc += "\n";
                    desc += e.description;
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
    mouseX = event->x();
    emit mouseMove(event->x(),event->y(),mousePos,desc);
    QSlider::mouseMoveEvent(event);
}

void ClickSlider::enterEvent(QEvent *e)
{
    update();
    emit mouseEnter();
    QSlider::enterEvent(e);
}

void ClickSlider::leaveEvent(QEvent *e)
{
    update();
    emit mouseLeave();
    QSlider::leaveEvent(e);
}

ClickSliderStyle::ClickSliderStyle(ClickSlider *slider, QStyle *style) : QProxyStyle(style), _slider(slider), _colorStyle(new ClickSliderColorStyle(nullptr))
{
    setProperty("circleRadius", 0.01);

}

void ClickSliderStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    switch (control)
    {
    case QStyle::CC_Slider:
    {
        const QStyleOptionSlider* sopt = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if (!sopt)
        {
            break;
        }

        painter->save();
        painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        QRect sliderRect = sopt->rect;
        QRect sliderHandleRect = subControlRect(control, sopt, SC_SliderHandle, widget);
        sliderHandleRect.adjust(1, 1, -1, -1);

        painter->setPen(Qt::NoPen);
        // painter->setBrush(ElaThemeColor(_themeMode, BasicChute));
        painter->setBrush(QColor(255, 255, 255, 100));
        if (sopt->orientation == Qt::Horizontal)
        {
            int t_sliderLeft = sliderRect.x() + sliderHandleRect.width() / 2;
            int top = sliderRect.y() + sliderRect.height() * 0.375;
            int t_sliderWidth = sliderRect.width() - sliderHandleRect.width();
            bool _sliderPosChanged = _sliderLeft != t_sliderLeft || _sliderWidth != t_sliderWidth;
            _sliderLeft = t_sliderLeft;
            _sliderWidth = t_sliderWidth;
            if (_sliderPosChanged) emit _slider->sliderPosChanged();
            int h = sliderRect.height() / 4;
            if (sopt->state & QStyle::State_MouseOver)
            {
                top = sliderRect.y() + sliderRect.height() / 4;
                h = sliderRect.height() / 2;
            }

            painter->drawRoundedRect(QRect(_sliderLeft, top, _sliderWidth, h), sliderRect.height() / 8, sliderRect.height() / 8);
            painter->setBrush(_colorStyle->getSliderColor());
            painter->drawRoundedRect(QRect(_sliderLeft, top, sliderHandleRect.x(), h), sliderRect.height() / 8, sliderRect.height() / 8);

            if (!_slider->chapters.empty())
            {
                const int sliderMin = _slider->minimum();
                const int sliderMax = _slider->maximum();
                for(auto &c : _slider->chapters)
                {
                    int l = sliderPositionFromValue(sliderMin, sliderMax, qBound(sliderMin, c.position, sliderMax), _sliderWidth);
                    painter->fillRect(l + _sliderLeft, top, 2, h, c.position<_slider->value()?_colorStyle->getCColor2():_colorStyle->getCColor1());
                }
            }
            if (!_slider->eventList.empty())
            {
                const int sliderMin = _slider->minimum();
                const int sliderMax = _slider->maximum();
                for (auto &e : _slider->eventList)
                {
                    int l = sliderPositionFromValue(sliderMin, sliderMax, qBound(sliderMin, e.start, sliderMax), _sliderWidth);
                    int r = sliderPositionFromValue(sliderMin, sliderMax, qBound(sliderMin, e.start+e.duration, sliderMax), _sliderWidth);
                    painter->fillRect(l + _sliderLeft, top, r - l, h, e.start+e.duration<_slider->value()?_colorStyle->getEColor2():_colorStyle->getEColor1());
                }
            }
        }
        if (sopt->state & QStyle::State_MouseOver)
        {
            painter->setPen(Qt::NoPen);
            painter->setBrush(_colorStyle->getHandleColor());
            if (!_lastState.testFlag(QStyle::State_MouseOver))
            {
                _circleRadius = sliderHandleRect.width() / 8;
                _startRadiusAnimation(_circleRadius, sliderHandleRect.width() / 2, const_cast<QWidget*>(widget));
                _lastState = sopt->state;
            }
            painter->drawEllipse(QPointF(sliderHandleRect.center().x(), sliderHandleRect.center().y()), _circleRadius, _circleRadius);
        }
        else
        {
            if (_lastState.testFlag(QStyle::State_MouseOver))
            {
                _startRadiusAnimation(_circleRadius, sliderHandleRect.width() / 8, const_cast<QWidget*>(widget));
                _lastState &= ~QStyle::State_MouseOver;
            }
            if (_circleRadius > sliderHandleRect.width() / 8)
            {
                painter->setBrush(_colorStyle->getHandleColor());
                painter->drawEllipse(QPointF(sliderHandleRect.center().x(), sliderHandleRect.center().y()), _circleRadius, _circleRadius);
            }
        }
        painter->restore();
        return;
    }
    default:
    {
        break;
    }
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

int ClickSliderStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric)
    {
    case QStyle::PM_SliderLength:
    {
        return 20;
    }
    case QStyle::PM_SliderThickness:
    {
        return 20;
    }
    default:
    {
        break;
    }
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

void ClickSliderStyle::_startRadiusAnimation(qreal startRadius, qreal endRadius, QWidget *widget) const
{
    ClickSliderStyle* style = const_cast<ClickSliderStyle*>(this);
    QPropertyAnimation* circleRadiusAnimation = new QPropertyAnimation(style, "circleRadius");
    connect(circleRadiusAnimation, &QPropertyAnimation::valueChanged, style, [=](const QVariant& value) {
        this->_circleRadius = value.toReal();
        widget->update();
    });
    circleRadiusAnimation->setEasingCurve(QEasingCurve::InOutSine);
    circleRadiusAnimation->setStartValue(startRadius);
    circleRadiusAnimation->setEndValue(endRadius);
    circleRadiusAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}


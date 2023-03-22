#include "smoothscrollbar.h"
#include <QWheelEvent>
SmoothScrollBar::SmoothScrollBar(QWidget* parent):QScrollBar(parent)
{
    m_scrollAni=new QPropertyAnimation;
    m_scrollAni->setTargetObject(this);
    m_scrollAni->setPropertyName("value");
    m_scrollAni->setEasingCurve(QEasingCurve::OutQuint);
    m_scrollAni->setDuration(400);
    m_targetValue_v=value();
}

void SmoothScrollBar::setValue(int value)
{
    m_scrollAni->stop();
    m_scrollAni->setStartValue(this->value());
    m_scrollAni->setEndValue(value);
    m_scrollAni->start();
}

void SmoothScrollBar::scroll(int value)
{
    // 对动画方向和滚动方向进行判断
    if ((this->value() <= m_targetValue_v && value < 0) || (this->value() >= m_targetValue_v && value > 0)) {
        // 1. 同向，则距离叠加
        m_targetValue_v -= value;
    } else if ((this->value() <= m_targetValue_v && value > 0) || (this->value() >= m_targetValue_v && value < 0)) {
        // 2. 反向，则重置 m_targetValue_v 为当前值并重新计算
        m_targetValue_v = this->value();
        m_targetValue_v -= value;
    }

    // 对于 m_targetValue_v 超出上下限处理（滚动条接近上下边界会减速，没啥用）
    if (m_targetValue_v < 0) {
        m_targetValue_v = 0;
    } else if (m_targetValue_v > this->maximum()) {
        m_targetValue_v = this->maximum();
    }

    setValue(m_targetValue_v);
}
void SmoothScrollBar::mousePressEvent(QMouseEvent *e)
{
    m_scrollAni->stop();
    QScrollBar::mousePressEvent(e);
    m_targetValue_v=value();

}

void SmoothScrollBar::mouseReleaseEvent(QMouseEvent *e)
{
    m_scrollAni->stop();
    QScrollBar::mouseReleaseEvent(e);
    m_targetValue_v=value();
}

void SmoothScrollBar::mouseMoveEvent(QMouseEvent *e)
{
    m_scrollAni->stop();
    QScrollBar::mouseMoveEvent(e);
    m_targetValue_v=value();
}


#include "clickslider.h"
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <QStylePainter>

void ClickSlider::setEventMark(const QList<DanmuEvent> &eventList)
{
    this->eventList=eventList;
}

void ClickSlider::mousePressEvent(QMouseEvent *event)
{
    QSlider::mousePressEvent(event);
    if(!isSliderDown() && event->button() == Qt::LeftButton)
    {
        int dur = maximum()-minimum();
        double x=event->x();
        if(x<0)x=0;
        if(x>maximum())x=maximum();
        int pos = minimum() + dur * (x /width());
        emit sliderClick(pos);
        setValue(pos);
    }

}

void ClickSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if(isSliderDown())
    {
        int dur = maximum()-minimum();
        double x=event->x();
        if(x<0)x=0;
        if(x>maximum())x=maximum();
        int pos = minimum() + dur * (x /width());
        emit sliderUp(pos);
        setValue(pos);
    }
    QSlider::mouseReleaseEvent(event);
}

void ClickSlider::mouseMoveEvent(QMouseEvent *event)
{
    int dur = maximum()-minimum();
    double x=event->x();
    if(x<0)x=0;
    if(x>maximum())x=maximum();
    int pos = minimum() + dur * (x /width());
    if(!eventList.isEmpty())
    {
        for(auto &e:eventList)
        {
            if(pos>=e.start && pos<=e.start+e.duration)
            {
                emit mouseMove(event->x(),event->y(),pos,e.description);
                return;
            }
        }
    }
    emit mouseMove(event->x(),event->y(),pos);
}

void ClickSlider::paintEvent(QPaintEvent *event)
{
    QSlider::paintEvent(event);
    if(eventList.isEmpty()) return;

    QStylePainter painter(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    static QColor eColor(255,117,0);
    int h = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove,this).height();
    int y =height()-h;
    y=(y&1)?y/2+1:y/2;

    for(auto &e:eventList)
    {
        int left = QStyle::sliderPositionFromValue(minimum(), maximum(), e.start, width());
        int right = QStyle::sliderPositionFromValue(minimum(), maximum(), e.start+e.duration, width());
        painter.fillRect(left,y,right-left,h,eColor);
    }

}

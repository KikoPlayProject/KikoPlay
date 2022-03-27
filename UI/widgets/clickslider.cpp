#include "clickslider.h"
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <QStylePainter>

void ClickSlider::setEventMark(const QVector<DanmuEvent> &eventList)
{
    this->eventList=eventList;
}

void ClickSlider::setChapterMark(const QVector<MPVPlayer::ChapterInfo> &chapters)
{
    this->chapters=chapters;
}

void ClickSlider::mousePressEvent(QMouseEvent *event)
{
    QSlider::mousePressEvent(event);
    if(!isSliderDown() && event->button() == Qt::LeftButton)
    {
        int dur = maximum()-minimum();
        double x=qBound<double>(0., event->x(), width());
        int pos = minimum() + dur * (x /width());
        emit sliderClick(pos);
        setValue(pos);
    }
    event->accept();

}

void ClickSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if(isSliderDown())
    {
        int dur = maximum()-minimum();
        double x=qBound<double>(0., event->x(), width());
        int pos = minimum() + dur * (x /width());
        emit sliderUp(pos);
        setValue(pos);
    }
    QSlider::mouseReleaseEvent(event);
    event->accept();
}

void ClickSlider::mouseMoveEvent(QMouseEvent *event)
{
    int dur = maximum()-minimum();
    double x=qBound<double>(0., event->x(), width());
    mousePos = minimum() + dur * (x /width());
    QString desc;
    if(!chapters.isEmpty())
    {
        for(auto &c:chapters)
        {
            if(qAbs(mousePos-c.position)<3000)
            {
                desc = tr("Chapter: %1").arg(c.title);
                break;
            }
        }
    }
    if(!eventList.isEmpty())
    {
        for(auto &e:eventList)
        {
            if(mousePos>=e.start-3000)
            {
                if(mousePos<=e.start+e.duration+3000)
                {
                    if(!desc.isEmpty()) desc += "\n";
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
}

void ClickSlider::paintEvent(QPaintEvent *event)
{
    QSlider::paintEvent(event);
    if(eventList.isEmpty() && chapters.isEmpty()) return;

    QStylePainter painter(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect grooveRect(style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove,this));
    int h = grooveRect.height();
    int y = grooveRect.y();
    for(auto &e:eventList)
    {
        int left = QStyle::sliderPositionFromValue(minimum(), maximum(), qBound(minimum(), e.start, maximum()), width());
        int right = QStyle::sliderPositionFromValue(minimum(), maximum(), qBound(minimum(), e.start+e.duration, maximum()), width());
        painter.fillRect(left,y,right-left,h,e.start+e.duration<value()?eColor2:eColor1);
    }
    for(auto &c:chapters)
    {
        int left = QStyle::sliderPositionFromValue(minimum(), maximum(), qBound(minimum(), c.position, maximum()), width());
        painter.fillRect(left,y/2,2,y-y/2+1,c.position<value()?cColor2:cColor1);
    }
}

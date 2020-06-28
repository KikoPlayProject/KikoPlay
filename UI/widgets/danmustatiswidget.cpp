#include "danmustatiswidget.h"
#include <QPainter>
#include <QStyleOption>
#include "globalobjects.h"

DanmuStatisWidget::DanmuStatisWidget(QWidget *parent) : QWidget(parent), curDuration(0)
{

}

void DanmuStatisWidget::refreshStatis()
{
    const StatisInfo &stat = GlobalObjects::danmuPool->getStatisInfo();
    QImage statImg(width(), height(), QImage::Format_ARGB32);
    statImg.fill(bgColor);
    QPainter painter(&statImg);
    auto bRect = statImg.rect();
    if(curDuration==0)
    {
        statPixmap = QPixmap::fromImage(statImg);
        return;
    }
    bRect.adjust(1, 0, -1, 0);
    float hRatio=(float)bRect.height()/stat.maxCountOfMinute;
    float margin=8*logicalDpiX()/96;
    float wRatio=(float)(bRect.width()-margin*2)/curDuration;
    float bHeight=bRect.height();
    for(auto iter=stat.countOfMinute.cbegin();iter!=stat.countOfMinute.cend();++iter)
    {
        float l((*iter).first*wRatio);
        float h(floor((*iter).second*hRatio));
        painter.fillRect(l+margin,bHeight-h,wRatio<1.f?1.f:wRatio,h,barColor);
    }
    painter.setPen(penColor);
    painter.drawText(bRect,Qt::AlignLeft|Qt::AlignTop,QObject::tr("Total:%1 Max:%2 Block:%3 Merge:%4")
                     .arg(QString::number(stat.totalCount),
                          QString::number(stat.maxCountOfMinute),
                          QString::number(stat.blockCount),
                          QString::number(stat.mergeCount)));
    statPixmap = QPixmap::fromImage(statImg);
}

void DanmuStatisWidget::setDuration(int duration)
{
    curDuration = duration;
    refreshStatis();
}

void DanmuStatisWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(0,0,0,0));
    painter.drawPixmap(0, 0, statPixmap);
    QStyleOption styleOpt;
    styleOpt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOpt, &painter, this);
}

void DanmuStatisWidget::resizeEvent(QResizeEvent *event)
{
    refreshStatis();
    QWidget::resizeEvent(event);
}

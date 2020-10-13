#include "danmustatiswidget.h"
#include <QPainter>
#include <QStyleOption>
#include "globalobjects.h"

DanmuStatisWidget::DanmuStatisWidget(QWidget *parent) : QWidget(parent), curDuration(0)
{

}

void DanmuStatisWidget::refreshStatis()
{
    QImage statImg(width(), height(), QImage::Format_ARGB32);
    statImg.fill(bgColor);
    QPainter painter(&statImg);
    auto bRect = statImg.rect();
    if(curDuration==0)
    {
        statPixmap = QPixmap::fromImage(statImg);
        return;
    }
    const StatisInfo &stat = GlobalObjects::danmuPool->getStatisInfo();
    bRect.adjust(1, 0, -1, 0);
    float hRatio=(float)bRect.height()/stat.maxCountOfMinute;
    float margin=8*logicalDpiX()/96;
    float wRatio=(bRect.width()-margin*2)/curDuration;
    float bHeight=bRect.height();
    int w = bRect.width()-margin*2;
    if(w>curDuration)
    {
        int lastC=0, lastP=-1;
        float lastX = margin, lastW = 0.0;
        for(const auto &p: stat.countOfSecond)
        {
            int cx, cw;
            //smoothing
            if(lastP+2==p.first)
            {
                cx = lastX + lastW;
                cw = (lastP+2)*wRatio+margin - cx;
                float h(floor((lastC+p.second)/2*hRatio));
                painter.fillRect(cx,bHeight-h,cw,h,barColor);
                ++lastP;
                lastX = cx; lastW = cw;
            }
            if(lastP+1==p.first)
            {
                cx = lastX + lastW;
                cw = (p.first+1)*wRatio+margin - cx;
            }
            else
            {
                cx = p.first*wRatio+margin;
                cw = wRatio;
            }
            lastX = cx; lastW = cw;
            float h(floor(p.second*hRatio));
            painter.fillRect(cx,bHeight-h,cw,h,barColor);
            lastP = p.first;
            lastC = p.second;
        }
    }
    else
    {
        static QVector<int> bins;
        bins.resize(w);
        for(int i=0;i<w;++i) bins[i]=0;
        float percent = 1.0/curDuration;
        for(const auto &p: stat.countOfSecond)
        {
            int pos =qMin(int(percent*p.first*w), w-1);
            bins[pos] = qMax(bins[pos], p.second);
        }
        for(int i=0;i<w;++i)
        {
            if(bins[i]>0)
            {
                float h(floor(bins[i]*hRatio));
                painter.fillRect(i+margin,bHeight-h,1,h,barColor);
                //smoothing
                if(i>1 && bins[i-2]>0 && bins[i-1]==0)
                {
                    float h(floor((bins[i-2]+bins[i])/2*hRatio));
                    painter.fillRect(i-1+margin,bHeight-h,1,h,barColor);
                }
            }
        }
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

void DanmuStatisWidget::paintEvent(QPaintEvent *)
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

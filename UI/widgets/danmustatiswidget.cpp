#include "danmustatiswidget.h"
#include <QPainter>
#include <QStyleOption>
#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"

#include "clickslider.h"

DanmuStatisWidget::DanmuStatisWidget(QWidget *parent) : QWidget(parent), curDuration(0)
{

}

void DanmuStatisWidget::refreshStatis()
{
    int pxW = width();
    int pxH = height();
    double pxR = devicePixelRatio();
    QPixmap statImg(pxW * pxR, pxH * pxR);
    statImg.setDevicePixelRatio(pxR);
    statImg.fill(bgColor);
    QPainter painter(&statImg);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
    auto bRect = statImg.rect();
    if (curDuration==0)
    {
        statPixmap = statImg;
        return;
    }
    const StatisInfo &stat = GlobalObjects::danmuPool->getStatisInfo();
    bRect.adjust(1, 0, -1, 0);

    QVector<QPair<int, int>> bins{1, {0, 0}};
    int timeWindow = 15;
    int lastTime = 0;
    int binMax = 0;
    for(const auto &p: stat.countOfSecond)
    {
        if (p.first - lastTime < timeWindow)
        {
            bins.back().second += p.second;
        }
        else
        {
            binMax = qMax(binMax, bins.back().second);
            bins.append({p.first / timeWindow, p.second});
            lastTime = bins.back().first * timeWindow;
        }
    }

    QPainterPath path;

    float h = height();
    float hr = h / binMax;
    float w = pxW - 8 * pxR;
    if (refSlider && refSlider->sliderWidth() > 0) w = refSlider->sliderWidth();
    int left = refSlider? refSlider->sliderLeft() : 0;
    QPointF last(left, h);
    path.moveTo(last);
    for (int i = 0; i < bins.size(); ++i)
    {
        if (i > 0 && bins[i].first - bins[i-1].first > 1)
        {
            float x = left + QStyle::sliderPositionFromValue(0, curDuration, (bins[i-1].first + 1) * timeWindow + timeWindow/2, w);
            float y = h;
            QPointF ep(x, y);
            QPointF c1 = QPointF((last.x() + ep.x()) / 2, last.y());
            QPointF c2 = QPointF((last.x() + ep.x()) / 2, ep.y());
            path.cubicTo(c1, c2, ep);
            last = ep;

            x = left + QStyle::sliderPositionFromValue(0, curDuration, (bins[i].first - 1) * timeWindow + timeWindow/2, w);
            y = h;
            ep = {x, y};
            c1 = QPointF((last.x() + ep.x()) / 2, last.y());
            c2 = QPointF((last.x() + ep.x()) / 2, ep.y());
            path.cubicTo(c1, c2, ep);
            last = ep;
        }
        if (bins[i].first * timeWindow + timeWindow / 2 >= curDuration)
        {
            break;
        }
        float x = left + QStyle::sliderPositionFromValue(0, curDuration, bins[i].first * timeWindow + timeWindow / 2, w);
        float y = h - bins[i].second * hr;
        QPointF ep(x, y);
        QPointF c1 = QPointF((last.x() + ep.x()) / 2, last.y());
        QPointF c2 = QPointF((last.x() + ep.x()) / 2, ep.y());
        path.cubicTo(c1, c2, ep);
        last = ep;
    }
    QPointF ep(w+left, h);
    QPointF c1 = QPointF((last.x() + ep.x()) / 2, last.y());
    QPointF c2 = QPointF((last.x() + ep.x()) / 2, ep.y());
    path.cubicTo(c1, c2, ep);
    path.lineTo(left, h);
    painter.setPen(Qt::NoPen);
    painter.setBrush(barColor);

    painter.drawPath(path);
    painter.setPen(penColor);
    painter.drawText(bRect,Qt::AlignLeft|Qt::AlignTop,QObject::tr("Total:%1 Max:%2 Block:%3 Merge:%4")
                                                              .arg(QString::number(stat.totalCount),
                                                                   QString::number(stat.maxCountOfMinute),
                                                                   QString::number(stat.blockCount),
                                                                   QString::number(stat.mergeCount)));
    statPixmap = statImg;

}

void DanmuStatisWidget::setDuration(int duration)
{
    curDuration = duration;
    refreshStatis();
}

void DanmuStatisWidget::setRefSlider(ClickSlider *s)
{
    refSlider = s;
    QObject::connect(s, &ClickSlider::sliderPosChanged, this, [=](){
        refreshStatis();
        update();
    });
}

void DanmuStatisWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(0,0,0,0));
    painter.drawPixmap(rect(), statPixmap);
    QStyleOption styleOpt;
    styleOpt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOpt, &painter, this);
}

void DanmuStatisWidget::resizeEvent(QResizeEvent *event)
{
    refreshStatis();
    QWidget::resizeEvent(event);
}

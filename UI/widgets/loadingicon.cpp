#include "loadingicon.h"
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

LoadingIcon::LoadingIcon(const QColor &color, QWidget *parent) : QWidget(parent), iconColor(color), angleSpeed(12)
{
    createIcon();
    refreshTimer = new QTimer(this);
    QObject::connect(refreshTimer, &QTimer::timeout, this, [=](){
        angle = (angle + angleSpeed) % 360;
        update();
    });
}

void LoadingIcon::show()
{
    if(!refreshTimer->isActive())
        refreshTimer->start(30);
    QWidget::show();
}

void LoadingIcon::hide()
{
    refreshTimer->stop();
    QWidget::hide();
}

void LoadingIcon::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.save();
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QPoint center(width()/2, height()/2);
    p.translate(center);
    p.rotate(angle);
    p.translate(-center);

    int length = qMin(width(), height());
    if(length & 1) --length;
    int cx = width()/2, cy=height()/2;
    p.drawPixmap(QRect(cx-length/2, cy-length/2, length, length),  icon);
    p.restore();
}

void LoadingIcon::resizeEvent(QResizeEvent *)
{
    createIcon();
}

void LoadingIcon::createIcon()
{
    int length = qMin(width(), height());
    if(length & 1) --length;
    icon = QPixmap(length, length);
    icon.fill(Qt::transparent);
    QPainter p(&icon);
    p.setPen(QPen(Qt::NoPen));
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QPainterPath path;

    QPoint center(length/2, length/2);

    auto &&boundingRect = icon.rect();
    int cw = length*3/4;
    if(cw & 1) --cw;
    int cx = (length-cw)/2;
    QRect boundingRectInner(cx,cx, cw,cw);

    path.moveTo(center);
    path.arcTo(boundingRect, 0, 90);
    path.moveTo(center);
    path.arcTo(boundingRectInner, 0, 90);

    path.moveTo(center);
    path.arcTo(boundingRect, 180, 90);
    path.moveTo(center);
    path.arcTo(boundingRectInner, 180, 90);

    p.setBrush(QBrush(iconColor));
    p.drawPath(path);
}

QSize LoadingIcon::minimumSizeHint() const
{
    return QSize(28 * logicalDpiX()/96, 28 * logicalDpiY()/96);
}

#include "kpushbutton.h"
#include "UI/ela/ElaTheme.h"
#include <QPainter>


void KPushButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing | QPainter::TextAntialiasing);
    eTheme->drawEffectShadow(&painter, rect(), 3, 4);
}

QSize KPushButton::sizeHint() const
{
    return QPushButton::sizeHint() + QSize(8, 0);
}

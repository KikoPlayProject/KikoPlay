#include "backgroundfadewidget.h"
#include <QPainter>
#include <QPropertyAnimation>
BackgroundFadeWidget::BackgroundFadeWidget(QWidget *parent) : QWidget(parent), backOpacity(0), opacityStart(0), opacityEnd(120), animeDuration(500), hideWidget(false), backColor(0,0,0,0)
{
    fadeAnimation = new QPropertyAnimation(this, "opacity", this);
    fadeAnimation->setStartValue(opacityStart);
    fadeAnimation->setEndValue(opacityEnd);
    fadeAnimation->setDuration(animeDuration);
    fadeAnimation->setEasingCurve(QEasingCurve::OutExpo);
    QObject::connect(fadeAnimation, &QPropertyAnimation::finished, this, [=](){
        if(hideWidget) hide();
    });
}

void BackgroundFadeWidget::fadeIn()
{
    show();
    raise();
    hideWidget = false;
    fadeAnimation->setDirection(QPropertyAnimation::Direction::Forward);
    fadeAnimation->start();
}

void BackgroundFadeWidget::fadeOut(bool hideFinished)
{
    hideWidget = hideFinished;
    fadeAnimation->setDirection(QPropertyAnimation::Direction::Backward);
    fadeAnimation->start();
}

void BackgroundFadeWidget::setOpacity(int o)
{
    backOpacity = o;
    update();
}

void BackgroundFadeWidget::setOpacityStart(int s)
{
    opacityStart = s;
    fadeAnimation->setStartValue(s);
}

void BackgroundFadeWidget::setOpacityEnd(int e)
{
    opacityEnd = e;
    fadeAnimation->setEndValue(e);
}

void BackgroundFadeWidget::setDuration(int d)
{
    animeDuration = d;
    fadeAnimation->setDuration(d);
}

void BackgroundFadeWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    backColor.setAlpha(backOpacity);
    painter.fillRect(rect(), backColor);
}

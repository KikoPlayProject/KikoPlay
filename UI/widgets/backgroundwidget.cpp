#include "backgroundwidget.h"
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QPropertyAnimation>
BackgroundWidget::BackgroundWidget(QWidget *parent):BackgroundWidget(QColor(0,0,0,90), parent)
{

}

BackgroundWidget::BackgroundWidget(const QColor &opactiyColor, QWidget *parent) : QWidget(parent), opColor(opactiyColor)
{

}

void BackgroundWidget::setBackground(const QImage &image)
{
    setImg(image);
}

void BackgroundWidget::setBackground(const QPixmap &pixmap)
{
    setImg(pixmap.toImage());
}


void BackgroundWidget::setBlur(bool on, qreal blurRadius)
{
    if(on)
    {
        QGraphicsBlurEffect *bgBlur = new QGraphicsBlurEffect;
        bgBlur->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
        bgBlur->setBlurRadius(blurRadius);
        setGraphicsEffect(bgBlur);
    }
    else
    {
        setGraphicsEffect(nullptr);
    }
}

void BackgroundWidget::setBlurAnimation(qreal s, qreal e)
{
    static QPropertyAnimation *lastAnime = nullptr;
    if(lastAnime)
    {
        lastAnime->stop();
        lastAnime = nullptr;
    }
    QGraphicsBlurEffect *bgBlur = new QGraphicsBlurEffect;
    bgBlur->setBlurHints(QGraphicsBlurEffect::AnimationHint);
    bgBlur->setBlurRadius(s);
    setGraphicsEffect(bgBlur);
    QPropertyAnimation *blurAnime = new QPropertyAnimation(bgBlur, "blurRadius");
    lastAnime = blurAnime;
    blurAnime->setDuration(500);
    blurAnime->setEasingCurve(QEasingCurve::OutExpo);
    blurAnime->setStartValue(s);
    blurAnime->setEndValue(e);
    blurAnime->start(QAbstractAnimation::DeleteWhenStopped);
    QObject::connect(blurAnime,&QPropertyAnimation::finished,[this, e, blurAnime](){
        if(e==0.) setGraphicsEffect(nullptr);
        if(lastAnime == blurAnime) lastAnime = nullptr;
    });
}

void BackgroundWidget::setImg(const QImage &nImg)
{
    img = nImg;
    QPainter p(&img);
    p.fillRect(img.rect(), opColor);
    p.end();
    bgCache = QPixmap::fromImage(img.scaled(width(),height(),Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    update();
}


void BackgroundWidget::paintEvent(QPaintEvent *e)
{
    if(!img.isNull())
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPixmap(0, 0, bgCache);
        QWidget::paintEvent(e);
    }
}
 void BackgroundWidget::resizeEvent(QResizeEvent *)
{
    bgCache = QPixmap::fromImage(img.scaled(width(),height(),Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
}

#include "backgroundwidget.h"
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QPropertyAnimation>
#include "globalobjects.h"
#include <QDebug>
#include <QSettings>
QT_BEGIN_NAMESPACE
  extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

BackgroundWidget::BackgroundWidget(QWidget *parent):BackgroundWidget(QColor(0,0,0,100), parent)
{
    blurAnime = new QPropertyAnimation(this, "blurRadius", this);
    blurAnime->setEasingCurve(QEasingCurve::OutExpo);
}

BackgroundWidget::BackgroundWidget(const QColor &opactiyColor, QWidget *parent) : QWidget(parent), opColor(opactiyColor), backBlurRadius(0)
{
    opColor.setAlpha(GlobalObjects::appSetting->value("MainWindow/BackgroundDarkness", 100).toInt());
}

void BackgroundWidget::setBlurRadius(qreal radius)
{
    if(backBlurRadius != radius)
    {
        backBlurRadius = radius;
        setBgCache();
        update();
    }
}

void BackgroundWidget::setBackground(const QImage &image)
{
    setImg(image);
}

void BackgroundWidget::setBackground(const QPixmap &pixmap)
{
    setImg(pixmap.toImage());
}

void BackgroundWidget::setBgDarkness(int val)
{
    opColor.setAlpha(val);
    update();
}

int BackgroundWidget::bgDarkness() const
{
    return opColor.alpha();
}


void BackgroundWidget::setBlur(bool on, qreal blurRadius)
{
    if(on)
    {
        setBlurRadius(blurRadius);
    }
    else
    {
        setBlurRadius(0);
    }
    update();
}

void BackgroundWidget::setBlurAnimation(qreal s, qreal e, int duration)
{
    if(blurAnime->state()==QPropertyAnimation::Running)
    {
        blurAnime->stop();
    }
    blurAnime->setDuration(duration);
    blurAnime->setStartValue(s);
    blurAnime->setEndValue(e);
    blurAnime->start();
}

void BackgroundWidget::setImg(const QImage &nImg)
{
    img = nImg;
    if(!img.isNull())
    {
        bgCacheSrc = img.scaled(width(),height(),Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        setBgCache();
    }
    update();
}

void BackgroundWidget::setBgCache()
{
    if(bgCacheSrc.isNull()) return;
    if(backBlurRadius <= 1)
    {
        bgCache = QPixmap::fromImage(bgCacheSrc);
        return;
    }
    if(bgCache.isNull() || bgCache.size()!=bgCacheSrc.size())
        bgCache = QPixmap(bgCacheSrc.size());
    bgCache.fill(Qt::transparent);
    QPainter painter(&bgCache);
    QImage imgTmp(bgCacheSrc);
    qt_blurImage(&painter, imgTmp, backBlurRadius*2.5, false, false);
}


void BackgroundWidget::paintEvent(QPaintEvent *)
{
    if(!img.isNull())
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPixmap(0, 0, bgCache);
        painter.fillRect(rect(), opColor);
    }
}
 void BackgroundWidget::resizeEvent(QResizeEvent *)
{
    if(!img.isNull())
    {
        bgCacheSrc = (img.scaled(width(),height(),Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        setBgCache();
    }
}

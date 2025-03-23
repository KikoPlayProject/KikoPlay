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

#define SETTING_KEY_BG_DARKNESS "MainWindow/BackgroundDarkness"

BackgroundMainWindow::BackgroundMainWindow(QWidget *parent) : QMainWindow(parent)
{
    blurAnime = new QPropertyAnimation(this, "blurRadius", this);
    blurAnime->setEasingCurve(QEasingCurve::OutExpo);
    opColor = QColor(0, 0, 0, 100);
    opColor.setAlpha(GlobalObjects::appSetting->value(SETTING_KEY_BG_DARKNESS, 100).toInt());
}

void BackgroundMainWindow::setBlurRadius(qreal radius)
{
    if (backBlurRadius != radius)
    {
        backBlurRadius = radius;
        setBgCache();
        update();
    }
}

void BackgroundMainWindow::setBackground(const QImage &image)
{
    setImg(image);
}

void BackgroundMainWindow::setBackground(const QPixmap &pixmap)
{
    setImg(pixmap.toImage());
}

void BackgroundMainWindow::setBgDarkness(int val)
{
    opColor.setAlpha(val);
    GlobalObjects::appSetting->setValue(SETTING_KEY_BG_DARKNESS, val);
    update();
}

int BackgroundMainWindow::bgDarkness() const
{
    return opColor.alpha();
}

void BackgroundMainWindow::setBlur(bool on, qreal blurRadius)
{

    setBlurRadius(on ? blurRadius : 0);
    setBlurRadius(0);
    update();
}

void BackgroundMainWindow::setBlurAnimation(qreal s, qreal e, int duration)
{
    if (blurAnime->state()==QPropertyAnimation::Running)
    {
        blurAnime->stop();
    }
    blurAnime->setDuration(duration);
    blurAnime->setStartValue(s);
    blurAnime->setEndValue(e);
    blurAnime->start();
}

void BackgroundMainWindow::setImg(const QImage &nImg)
{
    img = nImg;
    if (!img.isNull())
    {
        bgCacheSrc = img.scaled(width(),height(),Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        setBgCache();
    }
    update();
}

void BackgroundMainWindow::setBgCache()
{
    if (bgCacheSrc.isNull()) return;
    if (backBlurRadius <= 1)
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

void BackgroundMainWindow::paintEvent(QPaintEvent *event)
{
    if (!img.isNull())
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPixmap(0, 0, bgCache);
        painter.fillRect(rect(), opColor);
    }
    else
    {
        QMainWindow::paintEvent(event);
    }
}

void BackgroundMainWindow::resizeEvent(QResizeEvent *event)
{
    if (!img.isNull())
    {
        bgCacheSrc = (img.scaled(width(), height(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        setBgCache();
    }
}

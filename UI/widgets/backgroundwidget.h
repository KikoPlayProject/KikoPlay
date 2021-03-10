#ifndef BACKGROUNDWIDGET_H
#define BACKGROUNDWIDGET_H

#include <QWidget>
class QGraphicsBlurEffect;
class QPropertyAnimation;
class BackgroundWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal blurRadius READ getBlurRadius WRITE setBlurRadius)

public:
    BackgroundWidget(QWidget *parent = nullptr);
    BackgroundWidget(const QColor &opactiyColor, QWidget *parent = nullptr);

    qreal getBlurRadius() const {return backBlurRadius;}
    void setBlurRadius(qreal radius);

    void setBackground(const QImage &image);
    void setBackground(const QPixmap &pixmap);

    void setBgDarkness(int val);
    int bgDarkness() const;

    void setBlur(bool on, qreal blurRadius = 0);
    void setBlurAnimation(qreal s, qreal e, int duration = 500);
private:
    QImage img;
    QPixmap bgCache; QImage bgCacheSrc;
    QColor opColor;
    QPropertyAnimation *blurAnime;
    qreal backBlurRadius;
    void setImg(const QImage &nImg);
    void setBgCache();

    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // BACKGROUNDWIDGET_H

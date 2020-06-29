#ifndef BACKGROUNDWIDGET_H
#define BACKGROUNDWIDGET_H

#include <QWidget>
class BackgroundWidget : public QWidget
{
    Q_OBJECT
public:
    BackgroundWidget(QWidget *parent = nullptr);
    BackgroundWidget(const QColor &opactiyColor, QWidget *parent = nullptr);

    void setBackground(const QImage &image);
    void setBackground(const QPixmap &pixmap);

    void setBgDarkness(int val);

    void setBlur(bool on, qreal blurRadius = 0);
    void setBlurAnimation(qreal s, qreal e, int duration = 500);
private:
    QImage img;
    QPixmap bgCache;
    QColor opColor;
    void setImg(const QImage &nImg);

    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // BACKGROUNDWIDGET_H

#ifndef LOADINGICON_H
#define LOADINGICON_H

#include <QWidget>

class LoadingIcon : public QWidget
{
    Q_OBJECT
public:
    LoadingIcon(const QColor &color = Qt::white,  QWidget *parent = nullptr);
    void show();
    void hide();
    void anglePerFrame(int nAngle) { angleSpeed = nAngle; }
protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
private:
    QPixmap icon;
    QColor iconColor;
    int angle, angleSpeed;
    QTimer *refreshTimer;

    void createIcon();


    // QWidget interface
public:
    virtual QSize minimumSizeHint() const;
};

#endif // LOADINGICON_H

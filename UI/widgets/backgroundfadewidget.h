#ifndef BACKGROUNDFADEWIDGET_H
#define BACKGROUNDFADEWIDGET_H

#include <QWidget>
class QPropertyAnimation;
class BackgroundFadeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int opacity READ getOpacity WRITE setOpacity)
public:
    explicit BackgroundFadeWidget(QWidget *parent = nullptr);

    void fadeIn();
    void fadeOut(bool hideFinished);

    int getOpacity() const {return backOpacity;}
    void setOpacity(int o);
    int getOpacityStart() const {return opacityStart;}
    void setOpacityStart(int s);
    int getOpacityEnd() const {return opacityEnd;}
    void setOpacityEnd(int e);
    int getDuration() const {return animeDuration;}
    void setDuration(int d);
private:
    int backOpacity;
    int opacityStart, opacityEnd;
    int animeDuration;
    bool hideWidget;
    QColor backColor;
    QPropertyAnimation *fadeAnimation;
signals:

    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent *event);
};

#endif // BACKGROUNDFADEWIDGET_H

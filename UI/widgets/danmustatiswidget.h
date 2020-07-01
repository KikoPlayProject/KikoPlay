#ifndef DANMUSTATISWIDGET_H
#define DANMUSTATISWIDGET_H

#include <QWidget>
#include "Play/Danmu/danmupool.h"
class DanmuStatisWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor barColor READ getBarColor WRITE setBarColor)
    Q_PROPERTY(QColor bgColor READ getBgColor WRITE setBgColor)
public:
    explicit DanmuStatisWidget(QWidget *parent = nullptr);
    void refreshStatis();
    void setDuration(int duration);

    QColor getBarColor() const {return barColor;}
    void setBarColor(const QColor& color) { barColor =  color; refreshStatis(); }
    QColor getBgColor() const {return bgColor;}
    void setBgColor(const QColor& color) { bgColor =  color; refreshStatis(); }

    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private:
    QColor bgColor = QColor(0,0,0,150), barColor = QColor(51,168,255,200),penColor = QColor(255,255,255);
    QPixmap statPixmap;
    int curDuration;
};

#endif // DANMUSTATISWIDGET_H

#ifndef CLICKSLIDER_H
#define CLICKSLIDER_H
#include <QSlider>
#include "../Danmu/eventanalyzer.h"
#include "../Play/Video/mpvplayer.h"
class ClickSlider: public QSlider
{
    Q_OBJECT
public:
    explicit ClickSlider(QWidget *parent=nullptr):QSlider(Qt::Horizontal,parent){}
    void setEventMark(const QList<DanmuEvent> &eventList);
    void setChapterMark(const QList<MPVPlayer::ChapterInfo> &chapters);
private:
    QList<DanmuEvent> eventList;
    QList<MPVPlayer::ChapterInfo> chapters;
signals:
    void sliderClick(int position);
    void sliderUp(int position);
    void mouseMove(int x,int y,int pos,const QString &desc="");
    void mouseEnter();
    void mouseLeave();
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent *event);
    virtual void enterEvent(QEvent *)
    {
        emit mouseEnter();
    }
    virtual void leaveEvent(QEvent *)
    {
        emit mouseLeave();
    }
};

#endif // CLICKSLIDER_H

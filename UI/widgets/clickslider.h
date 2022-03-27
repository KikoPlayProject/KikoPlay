#ifndef CLICKSLIDER_H
#define CLICKSLIDER_H
#include <QSlider>
#include "../Danmu/eventanalyzer.h"
#include "../Play/Video/mpvplayer.h"
class ClickSlider: public QSlider
{
    Q_OBJECT
    Q_PROPERTY(QColor eColor1 READ getEColor1 WRITE setEColor1)
    Q_PROPERTY(QColor cColor1 READ getCColor1 WRITE setCColor1)
    Q_PROPERTY(QColor eColor2 READ getEColor2 WRITE setEColor2)
    Q_PROPERTY(QColor cColor2 READ getCColor2 WRITE setCColor2)
public:
    explicit ClickSlider(QWidget *parent=nullptr):QSlider(Qt::Horizontal,parent),mousePos(0){}
    void setEventMark(const QVector<DanmuEvent> &eventList);
    void setChapterMark(const QVector<MPVPlayer::ChapterInfo> &chapters);
    inline int curMousePos() const {return mousePos;}
    inline int curMouseX() const {return mouseX;}

    QColor getEColor1() const {return eColor1;}
    void setEColor1(const QColor& color) { eColor1 = color; }
    QColor getCColor1() const {return cColor1;}
    void setCColor1(const QColor& color) { cColor1 = color; }
    QColor getEColor2() const {return eColor2;}
    void setEColor2(const QColor& color) { eColor2 = color; }
    QColor getCColor2() const {return cColor2;}
    void setCColor2(const QColor& color) { cColor2 = color; }

private:
    QVector<DanmuEvent> eventList;
    QVector<MPVPlayer::ChapterInfo> chapters;
    int mousePos, mouseX;
    QColor eColor1{255,117,0}, eColor2{0xff,0xff,0x00}, cColor1{0xbc,0xbc,0xbc}, cColor2{0x43,0x9c,0xf3};
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

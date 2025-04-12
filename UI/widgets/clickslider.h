#ifndef CLICKSLIDER_H
#define CLICKSLIDER_H
#include <QSlider>
#include "Play/Video/mpvplayer.h"
#include <QProxyStyle>

class ClickSlider : public QSlider
{
    friend class ClickSliderStyle;
    Q_OBJECT
public:
    explicit ClickSlider(QWidget *parent=nullptr);
    void setEventMark(const QVector<DanmuEvent> &eventList);
    void setChapterMark(const QVector<MPVPlayer::ChapterInfo> &chapters);
    inline int curMousePos() const {return mousePos;}
    inline int curMouseX() const {return mouseX;}
    int sliderLeft() const;
    int sliderWidth() const;

private:
    ClickSliderStyle *_style;
    QVector<DanmuEvent> eventList;
    QVector<MPVPlayer::ChapterInfo> chapters;
    int mousePos, mouseX;

signals:
    void sliderClick(int position);
    void sliderUp(int position);
    void mouseMove(int x,int y,int pos,const QString &desc="");
    void mouseEnter();
    void mouseLeave();
    void sliderPosChanged();
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEnterEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
};

class ClickSliderColorStyle : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor eColor1 READ getEColor1 WRITE setEColor1)
    Q_PROPERTY(QColor cColor1 READ getCColor1 WRITE setCColor1)
    Q_PROPERTY(QColor eColor2 READ getEColor2 WRITE setEColor2)
    Q_PROPERTY(QColor cColor2 READ getCColor2 WRITE setCColor2)
    Q_PROPERTY(QColor sliderColor READ getSliderColor WRITE setSliderColor)
    Q_PROPERTY(QColor handleColor READ getHandleColor WRITE setHandleColor)
public:
    using QWidget::QWidget;

    QColor getEColor1() const {return eColor1;}
    void setEColor1(const QColor& color) { eColor1 = color; }
    QColor getCColor1() const {return cColor1;}
    void setCColor1(const QColor& color) { cColor1 = color; }
    QColor getEColor2() const {return eColor2;}
    void setEColor2(const QColor& color) { eColor2 = color; }
    QColor getCColor2() const {return cColor2;}
    void setCColor2(const QColor& color) { cColor2 = color; }
    QColor getSliderColor() const {return sliderColor;}
    void setSliderColor(const QColor& color) { sliderColor = color; }
    QColor getHandleColor() const {return handleColor;}
    void setHandleColor(const QColor& color) { handleColor = color; }
private:
    QColor eColor1{255,117,0}, eColor2{0xff,0xff,0x00}, cColor1{0xbc,0xbc,0xbc}, cColor2{0,117,158};
    QColor sliderColor{0, 174, 236, 100};
    QColor handleColor{0, 162, 219};
};

class ClickSliderStyle : public QProxyStyle
{
    Q_OBJECT
    friend class ClickSlider;
public:
    explicit ClickSliderStyle(ClickSlider *slider, QStyle* style = nullptr);
    ~ClickSliderStyle() { if (_colorStyle) _colorStyle->deleteLater(); }
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;

private:
    ClickSlider *_slider{nullptr};
    ClickSliderColorStyle *_colorStyle{nullptr};
    mutable QStyle::State _lastState{0};
    mutable qreal _circleRadius{0};
    mutable int _sliderLeft{0};
    mutable int _sliderWidth{0};
    void _startRadiusAnimation(qreal startRadius, qreal endRadius, QWidget* widget) const;
};


#endif // CLICKSLIDER_H

#ifndef SMOOTHSCROLLBAR_H
#define SMOOTHSCROLLBAR_H

#include <QScrollBar>
#include <QPropertyAnimation>
class SmoothScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    SmoothScrollBar(QWidget *parent=nullptr);

private:
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

    QPropertyAnimation *m_scrollAni;
    int m_targetValue_v;
public slots:
    void setValue(int value);
    void scroll(int value);
signals:
};

#endif // SMOOTHSCROLLBAR_H

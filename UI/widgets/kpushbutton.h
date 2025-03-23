#ifndef KPUSHBUTTON_H
#define KPUSHBUTTON_H

#include <QPushButton>

class KPushButton : public QPushButton
{
    Q_OBJECT
public:
    using QPushButton::QPushButton;

protected:
    virtual void paintEvent(QPaintEvent* event) override;

    // QWidget interface
public:
    QSize sizeHint() const override;
};

#endif // KPUSHBUTTON_H

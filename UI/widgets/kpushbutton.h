#ifndef KPUSHBUTTON_H
#define KPUSHBUTTON_H

#include <QPushButton>

class KPushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit KPushButton(QWidget *parent = nullptr);
    explicit KPushButton(const QString &text, QWidget *parent = nullptr);
    KPushButton(const QIcon &icon, const QString &text, QWidget *parent = nullptr);

protected:
    virtual void paintEvent(QPaintEvent* event) override;

    // QWidget interface
public:
    QSize sizeHint() const override;
};

#endif // KPUSHBUTTON_H

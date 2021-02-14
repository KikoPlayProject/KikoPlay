#ifndef DIALOGTIP_H
#define DIALOGTIP_H
#include <QWidget>
#include <QTimer>
class QLabel;
class QGraphicsOpacityEffect;
class DialogTip : public QWidget
{
public:
    explicit DialogTip(QWidget *parent);
    void showMessage(const QString &msg, int type=0);
private:
    QLabel *infoText, *busyLabel;
    QTimer hideTimer;
    QWidget *bgDarkWidget;
    QGraphicsOpacityEffect *fadeoutEffect;
    const int animeDuration = 500;
    const int showDuration = 2500;
};
#endif // DIALOGTIP_H

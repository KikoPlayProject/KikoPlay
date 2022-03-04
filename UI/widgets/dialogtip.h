#ifndef DIALOGTIP_H
#define DIALOGTIP_H
#include <QWidget>
#include <QTimer>
class QLabel;
class QGraphicsOpacityEffect;
class QPropertyAnimation;
class BackgroundFadeWidget;
class DialogTip : public QWidget
{
public:
    explicit DialogTip(QWidget *parent);
    void showMessage(const QString &msg, int type=0);
private:
    QLabel *infoText;
    QWidget *busyWidget;
    QTimer hideTimer;
    BackgroundFadeWidget *bgDarkWidget;
    QPropertyAnimation *moveAnime;
    bool moveHide;
    const int animeDuration = 500;
    const int showDuration = 2500;
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};
#endif // DIALOGTIP_H

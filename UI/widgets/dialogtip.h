#ifndef DIALOGTIP_H
#define DIALOGTIP_H
#include <QWidget>
#include <QTimer>
class QLabel;
class QGraphicsOpacityEffect;
class QPropertyAnimation;
class BackgroundFadeWidget;
class QPushButton;
class DialogTip : public QWidget
{
    Q_OBJECT
public:
    explicit DialogTip(QWidget *parent);
    void showMessage(const QString &msg, int type=0);
signals:
    void cancelClicked();
private:
    QLabel *infoText;
    QWidget *busyWidget;
    QPushButton *cancelBtn;
    QTimer hideTimer;
    QColor backColor;
    BackgroundFadeWidget *bgDarkWidget;
    QPropertyAnimation *moveAnime;
    bool moveHide;
    const int animeDuration = 500;
    const int showDuration = 2500;

    QString elideMessage(const QString &msg);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};
#endif // DIALOGTIP_H

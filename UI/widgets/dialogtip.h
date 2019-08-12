#ifndef DIALOGTIP_H
#define DIALOGTIP_H
#include <QWidget>
#include <QTimer>
class QLabel;
class DialogTip : public QWidget
{
public:
    explicit DialogTip(QWidget *parent=nullptr);
    void showMessage(const QString &msg, int type=0);
private:
    QLabel *infoText;
    QTimer hideTimer;
};
#endif // DIALOGTIP_H

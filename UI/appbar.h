#ifndef APPBAR_H
#define APPBAR_H

#include <QScrollArea>
#include <QPushButton>
class QGraphicsOpacityEffect;
namespace Extension
{
    class KApp;
}
class AppBarBtn : public QPushButton
{
    Q_OBJECT
public:
    AppBarBtn(Extension::KApp *kApp, QWidget *parent = nullptr);
    Extension::KApp *getApp() const { return app; }
    void flash();
private:
    Extension::KApp *app;
    QGraphicsOpacityEffect *eff;
    int flashTimes;
};

class AppBar : public QScrollArea
{
    Q_OBJECT
public:
    AppBar(QWidget *parent = nullptr);
public:
    void addApp(Extension::KApp *app);
    void removeApp(Extension::KApp *app);
    void flashApp(Extension::KApp *app);
private:
    QVector<AppBarBtn *> appList;
    bool isAdding;

    // QWidget interface
protected:
    void wheelEvent(QWheelEvent *event);

    // QWidget interface
public:
    QSize sizeHint() const;
};

#endif // APPBAR_H

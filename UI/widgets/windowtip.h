#ifndef WINDOWTIP_H
#define WINDOWTIP_H

#include "Common/notifier.h"
#include <QWidget>
#include <QVector>
#include <QTimer>

class QLabel;
class QPushButton;
class QPropertyAnimation;
class TipWindowWidget;

class WindowTip : public QObject
{
    Q_OBJECT
public:
    explicit WindowTip(QWidget *parent);
    void addTip(const TipParams &param);
    void setTop(int top) { this->top = top; }
private:
    const int maxTipNum = 3;
    QVector<TipWindowWidget *> tips;
    QVector<TipWindowWidget *> unusedTips;
    const int timerInterval = 500;
    QTimer hideTimer;
    int top = 0;

    bool eventFilter(QObject* object, QEvent* event) override;
    void onParentDestroyed();
    void layoutTips();
};

class TipWindowWidget : public QWidget
{
    friend class WindowTip;
    Q_OBJECT
    Q_PROPERTY(QPoint position READ pos WRITE move)
    Q_PROPERTY(float opacity READ windowOpacity WRITE setWindowOpacity)

public:
    explicit TipWindowWidget(const TipParams& params, QWidget* parent = 0);
    void reset(const TipParams& param);
signals:
    void closeClick();
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    TipParams tipParams;
    int timeout = 4000;
    QPropertyAnimation* positionAnimation = nullptr;
    QPropertyAnimation* opacityAnimation = nullptr;
    QLabel *titleLabel, *messageLabel;
    QPushButton *closeButton;
};

#endif // WINDOWTIP_H

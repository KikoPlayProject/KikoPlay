#include "kpushbutton.h"
#include "UI/ela/ElaTheme.h"
#include <QPainter>
#include "globalobjects.h"

KPushButton::KPushButton(QWidget *parent) : QPushButton(parent)
{
    QFont widgetFont(GlobalObjects::normalFont);
    widgetFont.setPixelSize(qRound(GlobalObjects::fontSize(15)));
    setFont(widgetFont);
}

KPushButton::KPushButton(const QString &text, QWidget *parent) : KPushButton(parent)
{
    setText(text);
}

KPushButton::KPushButton(const QIcon &icon, const QString &text, QWidget *parent) : KPushButton(text, parent)
{
    setIcon(icon);
}

void KPushButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing | QPainter::TextAntialiasing);
    eTheme->drawEffectShadow(&painter, rect(), 3, 4);
}

QSize KPushButton::sizeHint() const
{
    return QPushButton::sizeHint() + QSize(8, 0);
}

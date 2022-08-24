#include "fonticonbutton.h"
#include "globalobjects.h"
#include <QStyle>
#include <QApplication>
#include <QStyleOptionButton>
#include <QPainter>
#include <QResizeEvent>

FontIconButton::FontIconButton(QChar iconChar, const QString &content, int iconSize, int fontSize, int iconTextSpace, QWidget *parent):
    QPushButton(parent), icon(iconChar), iconFontSize(iconSize), textFontSize(fontSize), iconSpace(iconTextSpace), text(content)
{
    if(iconSpace==-1) iconSpace = 2;
    GlobalObjects::iconfont->setPointSize(iconFontSize);
    setFont(*GlobalObjects::iconfont);
    preferredWidth = sizeHint().width();
    QObject::connect(this,&QPushButton::toggled,[this](bool toggled){
        fontColor=toggled?hoverColor : normColor;
        update();
    });
}

void FontIconButton::setText(const QString &text)
{
    this->text = text;
    sHint = QSize();
    preferredWidth = sizeHint().width();
}

void FontIconButton::hideText(bool on)
{
    sHint = QSize();
    hideTextOn = on;
    sHint = sizeHint();
}

void FontIconButton::setNormColor(const QColor &color)
{
    normColor = color;
    if(!isChecked()) fontColor = color;
}

void FontIconButton::setHoverColor(const QColor &color)
{
    hoverColor = color;
    if(isChecked()) fontColor = color;
}

QSize FontIconButton::sizeHint() const
{
    if(sHint.isValid()) return sHint;
    ensurePolished();
    QFontMetrics fmIcon(font()), fmText(QFont(GlobalObjects::normalFont, textFontSize));
    QSize iconSize = fmIcon.size(Qt::TextShowMnemonic, icon);
    QSize textSize = fmText.size(Qt::TextShowMnemonic, text);
    int w = 0, h = 0;
    const int marginLeft = 4*logicalDpiX()/96, marginRight = 4*logicalDpiX()/96,
            marginTop = 4*logicalDpiY()/96, marginBottom = 4*logicalDpiY()/96;
    if(!hideTextOn) w += textSize.width() + iconSpace*logicalDpiX()/96;
    w += iconSize.width();
    h = qMax(iconSize.height(), textSize.height());
    w += marginLeft + marginRight;
    h += marginTop + marginBottom;
    QStyleOptionButton opt;
    initStyleOption(&opt);
    opt.rect.setSize(QSize(w, h));
    sHint = style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
                      expandedTo(QApplication::globalStrut());
    return sHint;
}

void FontIconButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);
    QPainter painter(this);
    painter.setPen(QPen(fontColor));

    painter.setFont(QFont(GlobalObjects::normalFont, textFontSize));
    QSize textSize(painter.fontMetrics().size(Qt::TextShowMnemonic, text));
    painter.setFont(font());
    QSize fontIconSize(painter.fontMetrics().size(Qt::TextShowMnemonic, icon));

    if(hideTextOn)
    {
        painter.drawText(rect(),Qt::AlignCenter,icon);
    }
    else
    {
        int contentWidth = fontIconSize.width()+textSize.width()+iconSpace*logicalDpiX()/96;
        int x = (width()-contentWidth)/2;

        QRect textRect(x,0,width(),height());
        painter.setPen(QPen(fontColor));
        painter.drawText(textRect,Qt::AlignVCenter|Qt::AlignLeft,icon);

        x += fontIconSize.width() + iconSpace*logicalDpiX()/96;
        textRect.setRect(x,0, width()-x,height());
        painter.setPen(QPen(fontColor));
        painter.setFont(QFont(GlobalObjects::normalFont, textFontSize));
        painter.drawText(textRect,Qt::AlignVCenter|Qt::AlignLeft,text);
    }

    painter.end();
}

void FontIconButton::resizeEvent(QResizeEvent *event)
{
    if(autoHideText)
    {
        if (event->size().width() < preferredWidth)
        {
            if(!hideTextOn)
            {
                hideTextOn = true;
                emit textHidden(true);
            }
        }
        else if (hideTextOn)
        {
            hideTextOn = false;
            emit textHidden(false);
        }
    }
    sHint = QSize();
    QPushButton::resizeEvent(event);
}

void FontIconButton::enterEvent(QEvent *)
{
    fontColor = hoverColor;
    update();
}

void FontIconButton::leaveEvent(QEvent *)
{
    if(!isChecked())
    {
        fontColor = normColor;
        update();
    }
}

void FontIconButton::mousePressEvent(QMouseEvent *e)
{
    fontColor = hoverColor;
    QPushButton::mousePressEvent(e);
}

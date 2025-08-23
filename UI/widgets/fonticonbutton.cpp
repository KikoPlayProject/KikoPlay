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
    setAttribute(Qt::WA_StyledBackground, true);
    if (iconSpace==-1) iconSpace = 2;
    GlobalObjects::iconfont->setPointSize(iconFontSize);
    setFont(*GlobalObjects::iconfont);
    setContentsMargins(4, 4, 4, 4);
    preferredWidth = sizeHint().width();
    QObject::connect(this, &QPushButton::toggled, this, [=](bool toggled){
        fontColor = toggled ? selectColor : normColor;
        update();
    });
}

void FontIconButton::setText(const QString &text)
{
    this->text = text;
    sHint = QSize();
    preferredWidth = sizeHint().width();
    update();
    updateGeometry();
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
    update();
}

void FontIconButton::setHoverColor(const QColor &color)
{
    hoverColor = color;
    update();
}

void FontIconButton::setSelectColor(const QColor &color)
{
    selectColor = color;
    update();
}

QSize FontIconButton::sizeHint() const
{
    if(sHint.isValid()) return sHint;
    ensurePolished();
    QFontMetrics fmIcon(font()), fmText(QFont(GlobalObjects::normalFont, textFontSize));
    QSize iconSize = fmIcon.size(Qt::TextShowMnemonic, icon);
    QSize textSize = fmText.size(Qt::TextShowMnemonic, text);
    int w = 0, h = 0;
    auto margin = contentsMargins();
    if(!hideTextOn) w += textSize.width() + iconSpace;
    w += iconSize.width();
    h = qMax(iconSize.height(), textSize.height());
    w += margin.left() + margin.right();
    h += margin.top() + margin.bottom();
    QStyleOptionButton opt;
    initStyleOption(&opt);
    opt.rect.setSize(QSize(w, h));
    sHint = style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this);
    return sHint;
}

void FontIconButton::setContentsMargins(int left, int top, int right, int bottom)
{
    QPushButton::setContentsMargins(left, top, right, bottom);
    sHint = QSize();
    style()->polish(this);
}

void FontIconButton::setTextAlignment(int align)
{
    alignment = align;
    update();
}

void FontIconButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);
    QPainter painter(this);

    QColor penColor = normColor;
    if (isChecked()) penColor = selectColor;
    if (underMouse()) penColor = hoverColor;

    painter.setPen(penColor);

    painter.setFont(QFont(GlobalObjects::normalFont, textFontSize));
    QSize textSize(painter.fontMetrics().size(Qt::TextShowMnemonic, text));
    painter.setFont(font());
    QSize fontIconSize(painter.fontMetrics().size(Qt::TextShowMnemonic, icon));

    if (hideTextOn || text.isEmpty())
    {
        painter.drawText(rect(), Qt::AlignCenter, icon);
    }
    else
    {
        const int contentWidth = fontIconSize.width() + textSize.width() + iconSpace;
        int x = (alignment & Qt::AlignLeft) ? contentsMargins().left() : (width() - contentWidth) / 2;

        QRect textRect(x, 0, width(), height());
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, icon);

        x += fontIconSize.width() + iconSpace;
        textRect.setRect(x, 0, width() - x, height());
        painter.setFont(QFont(GlobalObjects::normalFont, textFontSize));
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);
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



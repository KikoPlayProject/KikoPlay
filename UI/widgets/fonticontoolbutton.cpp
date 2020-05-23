#include <QLabel>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStyle>
#include "fonticontoolbutton.h"
#include "globalobjects.h"

FontIconToolButton::FontIconToolButton(QChar iconChar, const QString &text, int iconSize, int fontSize,
                                       int iconTextSpace, QWidget *parent):QToolButton(parent), autoHideText(false)
{
    if(iconTextSpace==-1) iconTextSpace = 2*logicalDpiX()/96;
    iconLabel=new QLabel(this);
    textLabel=new QLabel(this);
    GlobalObjects::iconfont.setPointSize(iconSize);
    iconLabel->setFont(GlobalObjects::iconfont);
    iconLabel->setText(iconChar);
    iconLabel->setMaximumWidth(iconLabel->height()+iconTextSpace);
    textLabel->setFont(QFont("Microsoft Yahei", fontSize));
    textLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    textLabel->setText(text);
    QHBoxLayout *btnHLayout=new QHBoxLayout(this);
    btnHLayout->setSpacing(0);
    btnHLayout->addStretch(1);
    btnHLayout->addWidget(iconLabel);
    btnHLayout->addSpacing(iconTextSpace);
    btnHLayout->addWidget(textLabel);
    btnHLayout->addStretch(1);
    QObject::connect(this,&FontIconToolButton::toggled,[this](bool toggled){
        if(toggled)setHoverState();
        else setNormalState();
    });
    preferredWidth = btnHLayout->sizeHint().width();
}

void FontIconToolButton::setCheckable(bool checkable)
{
    if(checkable)
    {
        QObject::disconnect(this,&FontIconToolButton::pressed,this,&FontIconToolButton::setHoverState);
        QObject::disconnect(this,&FontIconToolButton::released,this,&FontIconToolButton::setNormalState);
    }
    QToolButton::setCheckable(checkable);
}

void FontIconToolButton::setText(const QString &text)
{
    textLabel->setText(text);
    preferredWidth = layout()->sizeHint().width();
}

void FontIconToolButton::setAutoHideText(bool on)
{
    autoHideText = on;
}

void FontIconToolButton::hideText(bool hide)
{
    hide?textLabel->hide():textLabel->show();
}

QSize FontIconToolButton::iconSize() const
{
    return iconLabel->size();
}

QColor FontIconToolButton::getHoverColor() const
{
    return hoverColor;
}

void FontIconToolButton::setHoverColor(const QColor &color)
{
    hoverStyleSheet=QString("*{color:#%1;}").arg(color.rgba(),0,16);
    if(isChecked()) setHoverState();
}

QColor FontIconToolButton::getNormColor() const
{
    return normColor;
}

void FontIconToolButton::setNormColor(const QColor &color)
{
    normalStyleSheet=QString("*{color:#%1;}").arg(color.rgba(),0,16);
    if(!isChecked()) setNormalState();
}

void FontIconToolButton::setHoverState()
{
    if(hoverStyleSheet.isEmpty()) return;
    iconLabel->setStyleSheet(hoverStyleSheet);
    iconLabel->style()->unpolish(iconLabel);
    iconLabel->style()->polish(iconLabel);
    textLabel->setStyleSheet(hoverStyleSheet);
    textLabel->style()->unpolish(textLabel);
    textLabel->style()->polish(textLabel);
}

void FontIconToolButton::setNormalState()
{
    if(normalStyleSheet.isEmpty()) return;
    iconLabel->setStyleSheet(normalStyleSheet);
    iconLabel->style()->unpolish(iconLabel);
    iconLabel->style()->polish(iconLabel);
    textLabel->setStyleSheet(normalStyleSheet);
    textLabel->style()->unpolish(textLabel);
    textLabel->style()->polish(textLabel);
}

void FontIconToolButton::enterEvent(QEvent *)
{
    setHoverState();
}

void FontIconToolButton::leaveEvent(QEvent *)
{
    if(!isChecked())
        setNormalState();
}

void FontIconToolButton::mousePressEvent(QMouseEvent *e)
{
    setHoverState();
    QToolButton::mousePressEvent(e);
}

void FontIconToolButton::resizeEvent(QResizeEvent *event)
{
    if(autoHideText)
    {
        if (event->size().width() < preferredWidth)
        {
            if(!textLabel->isHidden())
            {
                textLabel->hide();
                emit iconHidden(true);
            }
        }
        else if (textLabel->isHidden())
        {
            textLabel->show();
            emit iconHidden(false);
        }
    }
	QWidget::resizeEvent(event);
}

QSize FontIconToolButton::sizeHint() const
{
    return layout()->sizeHint();
}

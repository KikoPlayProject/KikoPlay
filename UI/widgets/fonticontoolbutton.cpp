#include <QLabel>
#include <QHBoxLayout>
#include "fonticontoolbutton.h"
#include "globalobjects.h"

FontIconToolButton::FontIconToolButton(const FontIconToolButtonOptions &options, QWidget *parent):QToolButton(parent)
{
    iconLabel=new QLabel(this);
    textLabel=new QLabel(this);
    GlobalObjects::iconfont.setPointSize(options.iconSize);
    iconLabel->setFont(GlobalObjects::iconfont);
    iconLabel->setText(options.iconChar);
    iconLabel->setMaximumWidth(iconLabel->height()+options.iconTextSpace);
    textLabel->setFont(QFont("Microsoft Yahei",options.fontSize));
    textLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    QHBoxLayout *btnHLayout=new QHBoxLayout(this);
    btnHLayout->setSpacing(0);
    btnHLayout->addSpacing(options.leftMargin*logicalDpiX()/96);
    btnHLayout->addWidget(iconLabel);
    btnHLayout->addSpacing(10*logicalDpiX()/96);
    btnHLayout->addWidget(textLabel);
    btnHLayout->addStretch(1);
    QObject::connect(this,&FontIconToolButton::toggled,[this](bool toggled){
        if(toggled)setHoverState();
        else setNormalState();
    });
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
    textLabel->setStyleSheet(hoverStyleSheet);
}

void FontIconToolButton::setNormalState()
{
    if(normalStyleSheet.isEmpty()) return;
    iconLabel->setStyleSheet(normalStyleSheet);
    textLabel->setStyleSheet(normalStyleSheet);
}

void FontIconToolButton::enterEvent(QEvent *)
{
    setHoverState();
}

QSize FontIconToolButton::sizeHint() const
{
    return layout()->sizeHint();
}

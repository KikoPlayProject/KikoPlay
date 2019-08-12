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
    normalStyleSheet=QString("*{color:#%1;}").arg(options.normalColor,0,16);
    hoverStyleSheet=QString("*{color:#%1;}").arg(options.hoverColor,0,16);
    setNormalState();
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

void FontIconToolButton::setHoverState()
{
    iconLabel->setStyleSheet(hoverStyleSheet);
    textLabel->setStyleSheet(hoverStyleSheet);
}

void FontIconToolButton::setNormalState()
{
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

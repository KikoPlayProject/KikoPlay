#include "dialogtip.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include "globalobjects.h"
DialogTip::DialogTip(QWidget *parent):QWidget(parent)
{
    infoText=new QLabel(this);
    infoText->setObjectName(QStringLiteral("DialogTipLabel"));
    infoText->setFont(QFont(GlobalObjects::normalFont,10));
    infoText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    QHBoxLayout *infoBarHLayout=new QHBoxLayout(this);
    infoBarHLayout->addWidget(infoText);
    QObject::connect(&hideTimer,&QTimer::timeout,this,[this](){
        QPropertyAnimation *moveAnime = new QPropertyAnimation(this, "pos");
        QPoint endPos(this->x(),-this->height()), startPos(this->x(),this->y());
        moveAnime->setDuration(500);
        moveAnime->setEasingCurve(QEasingCurve::OutExpo);
        moveAnime->setStartValue(startPos);
        moveAnime->setEndValue(endPos);
        QObject::connect(moveAnime,&QPropertyAnimation::finished,this,&QWidget::hide);
        moveAnime->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void DialogTip::showMessage(const QString &msg, int type)
{
    if(type==0) setStyleSheet(QStringLiteral("border:none;background-color:#169fe6;"));
    else setStyleSheet(QStringLiteral("border:none;background-color:#f52941;"));
    bool isShow=hideTimer.isActive();
    if(isShow) hideTimer.stop();
    hideTimer.setSingleShot(true);
    hideTimer.start(2500);
    infoText->setText(msg);
    infoText->adjustSize();
    adjustSize();
    move((parentWidget()->width()-width())/2,y());
    if(!isShow)
    {
        show();
        QPropertyAnimation *moveAnime = new QPropertyAnimation(this, "pos");
        QPoint endPos(this->x(),0), startPos(this->x(),-this->height());
        moveAnime->setDuration(500);
        moveAnime->setEasingCurve(QEasingCurve::OutExpo);
        moveAnime->setStartValue(startPos);
        moveAnime->setEndValue(endPos);
        moveAnime->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

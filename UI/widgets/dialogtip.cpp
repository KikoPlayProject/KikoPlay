#include "dialogtip.h"
#include <QLabel>
#include <QHBoxLayout>
#include "loadingicon.h"
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include "globalobjects.h"
#include "Common/notifier.h"

DialogTip::DialogTip(QWidget *parent):QWidget(parent)
{
    infoText=new QLabel(this);
    infoText->setObjectName(QStringLiteral("DialogTipLabel"));
    infoText->setFont(QFont(GlobalObjects::normalFont,10));
    infoText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);

    bgDarkWidget = new QWidget(parent);
    bgDarkWidget->setStyleSheet("background-color:black;border-style:solid; border-width:1px;");
    bgDarkWidget->hide();
    fadeoutEffect = new QGraphicsOpacityEffect(bgDarkWidget);

    busyWidget = new LoadingIcon(Qt::white, this);
    static_cast<LoadingIcon *>(busyWidget)->hide();

    QHBoxLayout *infoBarHLayout=new QHBoxLayout(this);
    infoBarHLayout->addWidget(busyWidget);
    infoBarHLayout->addWidget(infoText);
    QObject::connect(&hideTimer,&QTimer::timeout,this,[this](){
        QPropertyAnimation *moveAnime = new QPropertyAnimation(this, "pos");
        QPoint endPos(this->x(),-this->height()), startPos(this->x(),this->y());
        moveAnime->setDuration(animeDuration);
        moveAnime->setEasingCurve(QEasingCurve::OutExpo);
        moveAnime->setStartValue(startPos);
        moveAnime->setEndValue(endPos);
        if(!bgDarkWidget->isHidden()) bgDarkWidget->hide();
        QObject::connect(moveAnime,&QPropertyAnimation::finished,this,&QWidget::hide);
        moveAnime->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void DialogTip::showMessage(const QString &msg, int type)
{
    if(type & NM_ERROR) setStyleSheet(QStringLiteral("border:none;background-color:#f52941;"));
    else setStyleSheet(QStringLiteral("border:none;background-color:#169fe6;"));

    if(type & NM_PROCESS)
    {
        static_cast<LoadingIcon *>(busyWidget)->show();
    }
    else
    {
        static_cast<LoadingIcon *>(busyWidget)->hide();
    }
    bool isShow=hideTimer.isActive();
    if(hideTimer.isActive()) hideTimer.stop();
    if(type & NM_HIDE)
    {
        hideTimer.setSingleShot(true);
        hideTimer.start(showDuration);
    }

    if(type & NM_DARKNESS_BACK)
    {
        if(bgDarkWidget->isHidden())
        {
            bgDarkWidget->move(0, 0);
            bgDarkWidget->resize(parentWidget()->width(), parentWidget()->height());
            bgDarkWidget->setGraphicsEffect(fadeoutEffect);
            fadeoutEffect->setOpacity(0);
            bgDarkWidget->show();
            bgDarkWidget->raise();
            QPropertyAnimation *bgFadeout = new QPropertyAnimation(fadeoutEffect,"opacity");
            bgFadeout->setDuration(animeDuration);
            bgFadeout->setStartValue(0);
            bgFadeout->setEndValue(0.7);
            bgFadeout->setEasingCurve(QEasingCurve::OutExpo);
            bgFadeout->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
    else
    {
        bgDarkWidget->hide();
    }

    infoText->setText(msg);
    infoText->adjustSize();
    adjustSize();
    move((parentWidget()->width()-width())/2,y());
    raise();

    if(!isShow)
    {
        show();
        QPropertyAnimation *moveAnime = new QPropertyAnimation(this, "pos");
        QPoint endPos(this->x(),0), startPos(this->x(),-this->height());
        moveAnime->setDuration(animeDuration);
        moveAnime->setEasingCurve(QEasingCurve::OutExpo);
        moveAnime->setStartValue(startPos);
        moveAnime->setEndValue(endPos);        
        moveAnime->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

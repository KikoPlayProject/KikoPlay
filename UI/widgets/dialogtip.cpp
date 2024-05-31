#include "dialogtip.h"
#include <QLabel>
#include <QHBoxLayout>
#include "loadingicon.h"
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>
#include "Common/notifier.h"
#include "backgroundfadewidget.h"

DialogTip::DialogTip(QWidget *parent):QWidget(parent), moveHide(false)
{
    infoText=new QLabel(this);
    infoText->setObjectName(QStringLiteral("DialogTipLabel"));
    infoText->setOpenExternalLinks(true);
    infoText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);

    bgDarkWidget = new BackgroundFadeWidget(parent);
    bgDarkWidget->hide();
    parent->installEventFilter(this);
    moveAnime = new QPropertyAnimation(this, "pos");
    moveAnime->setDuration(animeDuration);
    moveAnime->setEasingCurve(QEasingCurve::OutExpo);
    QObject::connect(moveAnime,&QPropertyAnimation::finished,this, [=](){
        if (moveHide) hide();
    });


    busyWidget = new LoadingIcon(Qt::white, this);
    static_cast<LoadingIcon *>(busyWidget)->hide();

    QHBoxLayout *infoBarHLayout=new QHBoxLayout(this);
    infoBarHLayout->addWidget(busyWidget);
    infoBarHLayout->addWidget(infoText);
    QObject::connect(&hideTimer,&QTimer::timeout,this,[this](){
        QPoint endPos(this->x(),-this->height()), startPos(this->x(),this->y());
        moveAnime->setStartValue(startPos);
        moveAnime->setEndValue(endPos);
        if(!bgDarkWidget->isHidden()) bgDarkWidget->hide();
        moveHide = true;
        moveAnime->start();
    });
}

void DialogTip::showMessage(const QString &msg, int type)
{
    if (type & NM_ERROR) setStyleSheet(QStringLiteral("border:none;background-color:#f52941;"));
    else setStyleSheet(QStringLiteral("border:none;background-color:#169fe6;"));

    if (type & NM_PROCESS)
    {
        static_cast<LoadingIcon *>(busyWidget)->show();
    }
    else
    {
        static_cast<LoadingIcon *>(busyWidget)->hide();
    }
    if (hideTimer.isActive()) hideTimer.stop();
    if (type & NM_HIDE)
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
            bgDarkWidget->fadeIn();
        }
    }
    else
    {
        bgDarkWidget->hide();
    }
    infoText->setMaximumWidth(parentWidget()->width());
    infoText->setText(msg);
    infoText->adjustSize();
    adjustSize();
    if (moveAnime->state() == QAbstractAnimation::Running)
    {
        moveAnime->stop();
        hide();
    }

    move((parentWidget()->width() - width()) / 2, y());
    raise();
    if (isHidden())
    {
        show();
        QPoint endPos(this->x(),0), startPos(this->x(),-this->height());
        moveAnime->setStartValue(startPos);
        moveAnime->setEndValue(endPos);        
        moveHide = false;
        moveAnime->start();
    }
}

bool DialogTip::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == parent() && event->type() == QEvent::Resize)
    {
        QResizeEvent *ev = static_cast<QResizeEvent *>(event);
        if (!bgDarkWidget->isHidden())
            bgDarkWidget->resize(ev->size());
        if (!isHidden())
        {
            int nx = (ev->size().width() - width()) / 2;
            if (moveAnime->state() == QAbstractAnimation::Running)
            {
                moveAnime->setStartValue(QPoint(nx, y()));
                moveAnime->setEndValue(QPoint(nx, 0));
            }
            else
            {
                move(nx, y());
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

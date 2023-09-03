#include "appbar.h"
#include "Extension/App/appmanager.h"
#include "Extension/App/kapp.h"
#include "globalobjects.h"
#include <QHBoxLayout>
#include <QAction>
#include <QScrollBar>
#include <QCoreApplication>
#include <QWheelEvent>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include "widgets/smoothscrollbar.h"

AppBar::AppBar(QWidget *parent) : QScrollArea(parent)
{
    setObjectName(QStringLiteral("AppBar"));
    QWidget *container = new QWidget(this);
    container->setMinimumHeight(20*logicalDpiY()/96);
    setWidget(container);
    QHBoxLayout *hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);
    hLayout->addStretch(1);
    setAlignment(Qt::AlignCenter);
    setWidgetResizable(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBar(new SmoothScrollBar);
    QObject::connect(GlobalObjects::appManager, &AppManager::appLaunched, this, &AppBar::addApp);
    QObject::connect(GlobalObjects::appManager, &AppManager::appTerminated, this, &AppBar::removeApp);
    QObject::connect(GlobalObjects::appManager, &AppManager::appFlash, this, &AppBar::flashApp);
    QObject::connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this](int min, int max){
        if (this->isAdding)
        {
            this->horizontalScrollBar()->setValue(max);
            this->isAdding = false;
        }
    });
}

void AppBar::addApp(Extension::KApp *app)
{
    auto iter = std::find_if(appList.begin(), appList.end(), [=](const AppBarBtn *btn){
      return btn->getApp() == app;
    });
    if (iter != appList.end()) return;
    AppBarBtn *appBtn = new AppBarBtn(app, this->widget());
    appList.append(appBtn);
    this->widget()->layout()->addWidget(appBtn);
    this->isAdding = true;
    this->updateGeometry();
}

void AppBar::removeApp(Extension::KApp *app)
{
    auto iter = std::find_if(appList.begin(), appList.end(), [=](const AppBarBtn *btn){
      return btn->getApp() == app;
    });
    if (iter == appList.end()) return;
    AppBarBtn *appBtn = *iter;
    appList.erase(iter);
    appBtn->deleteLater();
    this->updateGeometry();
}

void AppBar::flashApp(Extension::KApp *app)
{
    auto iter = std::find_if(appList.begin(), appList.end(), [=](const AppBarBtn *btn){
      return btn->getApp() == app;
    });
    if (iter == appList.end()) return;
    AppBarBtn *appBtn = *iter;
    ensureWidgetVisible(appBtn);
    appBtn->flash();
}

void AppBar::wheelEvent(QWheelEvent *event)
{
    static_cast<SmoothScrollBar *>(horizontalScrollBar())->scroll(event->angleDelta().y());
}

QSize AppBar::sizeHint() const
{
    return this->widget()->sizeHint();
}

AppBarBtn::AppBarBtn(Extension::KApp *kApp, QWidget *parent) : QPushButton(parent), app(kApp), eff(nullptr), flashTimes(0)
{
    setIcon(app->icon());
    setToolTip(app->name());
    QObject::connect(this, &AppBarBtn::clicked, this, [this](){
        this->app->show();
    });
    setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction *terminateApp = new QAction(tr("Terminate"), this);
    QObject::connect(terminateApp, &QAction::triggered, this, [=](){
        this->app->stop();
    });
    addAction(terminateApp);
}

void AppBarBtn::flash()
{
    if (!eff)
    {
        eff = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(eff);
    }
    const int maxFlashTimes = 3;
    if (flashTimes <= 0)
    {
        eff->setEnabled(true);
        QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
        a->setDuration(1000);
        a->setStartValue(1);
        a->setKeyValueAt(0.5, 0);
        a->setEndValue(1);
        a->setEasingCurve(QEasingCurve::InOutCubic);
        a->start();
        QObject::connect(a, &QPropertyAnimation::finished, this, [=](){
            bool inFlashState = --flashTimes > 0;
            if (inFlashState) a->start();
            else a->deleteLater();
            eff->setEnabled(inFlashState);
        });
    }
    flashTimes = maxFlashTimes;
}


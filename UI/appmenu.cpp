#include "appmenu.h"
#include <QGridLayout>
#include <QPushButton>
#include <QListView>
#include "globalobjects.h"
#include "Extension/App/appmanager.h"
#include "widgets/dialogtip.h"

AppMenu::AppMenu(QWidget *p, QWidget *parent)
    : QMenu{parent}, popupFromWidget(p)
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName(QStringLiteral("AppMenu"));

    dialogTip = new DialogTip(this);
    dialogTip->raise();
    dialogTip->hide();
    Notifier::getNotifier()->addNotify(Notifier::APP_MENU_NOTIFY, this);

    QListView *appView = new QListView(this);
    appView->setContentsMargins(0,0,0,0);
    appView->setObjectName(QStringLiteral("AppView"));
    appView->setViewMode(QListView::IconMode);
    appView->setProperty("cScrollStyle", true);
    appView->setUniformItemSizes(true);
    appView->setResizeMode(QListView::ResizeMode::Adjust);
    appView->setModel(GlobalObjects::appManager);
    appView->setFont(QFont(GlobalObjects::normalFont, 8, QFont::Light));

    const int appItemWidth = 64*logicalDpiX()/96;
    const int appItemHeight = 64*logicalDpiY()/96 + appView->fontMetrics().height();
    appView->setGridSize(QSize(appItemWidth, appItemHeight));
    appView->setMinimumWidth(appItemWidth * 4 + 8*logicalDpiX()/96);
    appView->setMinimumHeight(appItemHeight * 2 + 4*logicalDpiX()/96);

    QPushButton *refreshApp = new QPushButton(tr("Refresh"), this);
    refreshApp->setObjectName(QStringLiteral("AppRefreshBtn"));

    QObject::connect(refreshApp, &QPushButton::clicked, this, [=](){
        refreshApp->setEnabled(false);
        appView->setEnabled(false);
        GlobalObjects::appManager->refresh(true);
        refreshApp->setEnabled(true);
        appView->setEnabled(true);
    });

    QObject::connect(appView, &QListView::clicked, this, [=](const QModelIndex &index){
       GlobalObjects::appManager->start(index);
    });

    QGridLayout *appGLayout = new QGridLayout(this);
    appGLayout->addWidget(refreshApp, 0, 1);
    appGLayout->addWidget(appView, 1, 0, 1, 2);
    appGLayout->setColumnStretch(0, 1);
}

void AppMenu::showEvent(QShowEvent *event)
{
    const QPoint p = this->pos();
    this->move(p.x() + popupFromWidget->width() - this->width(), p.y());
}

void AppMenu::showMessage(const QString &content, int flag, const QVariant &)
{
    dialogTip->showMessage(content, flag);
}

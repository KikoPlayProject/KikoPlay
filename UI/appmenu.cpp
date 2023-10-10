#include "appmenu.h"
#include <QGridLayout>
#include <QPushButton>
#include <QListView>
#include <QPainter>
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
    appView->setItemDelegate(new AppItemDelegate(appItemWidth, appItemHeight, appView));
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

AppItemDelegate::AppItemDelegate(int w, int h, QObject *parent) : QStyledItemDelegate(parent), itemWidth(w), itemHeight(h)
{

}

void AppItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption,index);

    if (viewOption.state.testFlag(QStyle::State_MouseOver))
    {
        painter->fillRect(option.rect, QColor(255, 255, 255, 60));
    }
    else
    {
        painter->fillRect(option.rect, Qt::transparent);
    }

    painter->save();
    painter->translate(option.rect.x(), option.rect.y());

    int textHeight = painter->fontMetrics().height();
    const QPixmap icon = index.data(Qt::DecorationRole).value<QPixmap>();

    const int imgRectW = option.rect.width() * 0.8;
    const int imgRectH = imgRectW;

    int imgW = imgRectW * 0.7, imgH = imgRectH * 0.7;
    if (icon.width() >= icon.height())
    {
        imgH = (float)icon.height() / icon.width() * imgW;
    }
    else
    {
        imgW = (float)icon.width() / icon.height() * imgH;
    }

    const int imgX = (option.rect.width() - imgW) / 2;
    const int imgY = (option.rect.height() - imgH - textHeight) / 2;

    painter->drawPixmap(QRect(imgX, imgY, imgW, imgH), icon);

    const int textY = (option.rect.height() - imgRectH - textHeight) / 2 + imgRectH;
    const QString text = painter->fontMetrics().elidedText(index.data(Qt::DisplayRole).toString(), Qt::TextElideMode::ElideRight, option.rect.width());
    painter->drawText(0, textY, option.rect.width(), textHeight, Qt::AlignCenter, text);

    painter->restore();
}

QSize AppItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(itemWidth, itemHeight);
}

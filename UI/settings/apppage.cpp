#include "apppage.h"
#include <QTreeView>
#include <QGridLayout>
#include <QApplication>
#include <QSettings>
#include <QLabel>
#include <QPainter>
#include <QHeaderView>
#include "Extension/App/appmanager.h"
#include "globalobjects.h"

AppPage::AppPage(QWidget *parent)
{
    appView = new QTreeView(this);
    appView->setRootIsDecorated(false);
    appView->setSelectionMode(QAbstractItemView::SingleSelection);
    appView->setModel(GlobalObjects::appManager);
    appView->setAlternatingRowColors(true);
    appView->setFont(QFont(GlobalObjects::normalFont, 11));
    appView->setItemDelegate(new AppPageItemDelegate(qMax<int>(appView->fontMetrics().height() * 1.6, 32), this));
    QVariant headerState(GlobalObjects::appSetting->value("HeaderViewState/AppPageView"));
    if(!headerState.isNull())
        appView->header()->restoreState(headerState.toByteArray());

    QLabel *tip=new QLabel(QString("<a style='color: rgb(96, 208, 252);' href = \"https://github.com/KikoPlayProject/KikoPlayApp\">%1</a>").arg(tr("More")),this);
    tip->setOpenExternalLinks(true);

    QGridLayout *scriptGLayout = new QGridLayout(this);
    scriptGLayout->addWidget(tip, 0, 3);
    scriptGLayout->addWidget(appView, 1, 0, 1, 4);
    scriptGLayout->setRowStretch(1, 1);
    scriptGLayout->setColumnStretch(2, 1);
    scriptGLayout->setContentsMargins(0, 0, 0, 0);
}

void AppPage::onAccept()
{
    GlobalObjects::appSetting->setValue("HeaderViewState/AppPageView", appView->header()->saveState());
}

void AppPage::onClose()
{
    GlobalObjects::appSetting->setValue("HeaderViewState/AppPageView", appView->header()->saveState());
}

QSize AppPageItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(48, rowHeight);
}

void AppPageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption,index);
    viewOption.decorationSize = QSize(rowHeight * 0.8, rowHeight * 0.8);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QRect itemRect = option.rect;

    if(index.column() == 0) itemRect.setLeft(0);
    painter->setBrush(QColor(255, 255, 255, 40));
    painter->setPen(Qt::NoPen);

    if (option.state & QStyle::State_Selected)
    {
        painter->drawRect(itemRect);
    }
    else
    {
        if (option.state & QStyle::State_MouseOver)
        {
            painter->drawRect(itemRect);
        }
    }
    painter->restore();

    QStyle *style = option.widget? option.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &viewOption, painter, option.widget);
}

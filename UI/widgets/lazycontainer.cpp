#include "lazycontainer.h"
#include <QLayout>
#include <QPainter>

LazyContainer::LazyContainer(QWidget *parent, QLayout *l, InitFunc initFunc) : QWidget(parent), contentInitFunc(initFunc), contentLayout(l)
{

}

void LazyContainer::init()
{
    if (!inited)
    {
        inited = true;
        QWidget *content = contentInitFunc();
        if (contentLayout)
        {
            contentLayout->replaceWidget(this, content);
        }
        this->deleteLater();
    }
}

void LazyContainer::showEvent(QShowEvent *event)
{
    if (!inited)
    {
        inited = true;
        QWidget *content = contentInitFunc();
        if (contentLayout)
        {
            contentLayout->replaceWidget(this, content);
        }
        this->deleteLater();
    }
    return QWidget::showEvent(event);
}

void LazyContainer::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 150));
}

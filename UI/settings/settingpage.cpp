#include "settingpage.h"
#include <QLabel>
#include <QVBoxLayout>
#include "globalobjects.h"

SettingItemArea::SettingItemArea(const QString &title, QWidget *parent) : QWidget(parent)
{
    QLabel *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName(QStringLiteral("SettingItemAreaTitle"));
    container = new QWidget(this);
    container->setObjectName(QStringLiteral("SettingItemAreaContainer"));
    container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(4);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(titleLabel);
    vLayout->addWidget(container);

    QVBoxLayout *cVLayout = new QVBoxLayout(container);
    cVLayout->setSpacing(4);
    cVLayout->setContentsMargins(12, 8, 12, 8);
}

QSize SettingItemArea::sizeHint() const
{
    return layout()->sizeHint();
}

void SettingItemArea::addItem(const QString &name, QWidget *item, const QString &nameTip)
{
    addItem(name, QVector<QWidget *>{item}, nameTip);
}

void SettingItemArea::addItem(const QString &name, const QVector<QWidget *> &items, const QString &nameTip)
{
    QLabel *nameLabel = new QLabel(name, this);
    nameLabel->setFont(QFont(GlobalObjects::normalFont, 12));
    nameLabel->setMinimumHeight(36);
    nameLabel->setToolTip(nameTip);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(nameLabel);
    hLayout->addStretch(1);
    hLayout->setSpacing(8);
    for (auto item : items)
    {
        hLayout->addWidget(item);
    }
    static_cast<QVBoxLayout *>(container->layout())->addLayout(hLayout);
}

void SettingItemArea::addItem(QWidget *item, Qt::Alignment align)
{
    static_cast<QVBoxLayout *>(container->layout())->addWidget(item, 0, align);
}

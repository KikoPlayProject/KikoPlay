﻿#include "ElaMenu.h"

#include <QCloseEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QVBoxLayout>
#include <QStyleFactory>
#include <QSettings>

#include "DeveloperComponents/ElaMenuStyle.h"
#include "private/ElaMenuPrivate.h"
#include "globalobjects.h"

int ElaMenu::_showMenuAnimation = -1;
#define SETTING_KEY_SHOW_MENU_ANIME "UI/ShowMenuAnimation"

ElaMenu::ElaMenu(QWidget* parent)
    : QMenu(parent), d_ptr(new ElaMenuPrivate())
{
    Q_D(ElaMenu);
    d->q_ptr = this;
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("ElaMenu");
    d->_menuStyle = new ElaMenuStyle(style());
    setStyle(d->_menuStyle);
    d->_pAnimationImagePosY = 0;
    d->_isSubMenu = dynamic_cast<QMenu*>(parent) != nullptr;

    if (_showMenuAnimation < 0)
    {
        _showMenuAnimation = GlobalObjects::appSetting->value(SETTING_KEY_SHOW_MENU_ANIME, true).toBool();
    }
}

ElaMenu::ElaMenu(const QString& title, QWidget* parent)
    : ElaMenu(parent)
{
    setTitle(title);
}

ElaMenu::~ElaMenu()
{
}

void ElaMenu::setMenuItemHeight(int menuItemHeight)
{
    Q_D(ElaMenu);
    d->_menuStyle->setMenuItemHeight(menuItemHeight);
}

int ElaMenu::getMenuItemHeight() const
{
    Q_D(const ElaMenu);
    return d->_menuStyle->getMenuItemHeight();
}

QAction* ElaMenu::addMenu(QMenu* menu)
{
    return QMenu::addMenu(menu);
}

ElaMenu* ElaMenu::addMenu(const QString& title)
{
    ElaMenu* menu = new ElaMenu(title, this);
    QMenu::addAction(menu->menuAction());
    return menu;
}

ElaMenu* ElaMenu::addMenu(const QIcon& icon, const QString& title)
{
    ElaMenu* menu = new ElaMenu(title, this);
    menu->setIcon(icon);
    QMenu::addAction(menu->menuAction());
    return menu;
}

ElaMenu* ElaMenu::addMenu(ElaIconType::IconName icon, const QString& title)
{
    ElaMenu* menu = new ElaMenu(title, this);
    QMenu::addAction(menu->menuAction());
    menu->menuAction()->setProperty("ElaIconType", QChar((unsigned short)icon));
    return menu;
}

QAction* ElaMenu::addElaIconAction(ElaIconType::IconName icon, const QString& text)
{
    QAction* action = new QAction(text, this);
    action->setProperty("ElaIconType", QChar((unsigned short)icon));
    QMenu::addAction(action);
    return action;
}

QAction* ElaMenu::addElaIconAction(ElaIconType::IconName icon, const QString& text, const QKeySequence& shortcut)
{
    QAction* action = new QAction(text, this);
    action->setShortcut(shortcut);
    action->setProperty("ElaIconType", QChar((unsigned short)icon));
    QMenu::addAction(action);
    return action;
}

bool ElaMenu::isHasChildMenu() const
{
    QList<QAction*> actionList = this->actions();
    for (auto action : actionList)
    {
        if (action->isSeparator())
        {
            continue;
        }
        if (action->menu())
        {
            return true;
        }
    }
    return false;
}

bool ElaMenu::isHasIcon() const
{
    QList<QAction*> actionList = this->actions();
    for (auto action : actionList)
    {
        if (action->isSeparator())
        {
            continue;
        }
        QMenu* menu = action->menu();
        if (menu && (!menu->icon().isNull() || !menu->property("ElaIconType").toString().isEmpty()))
        {
            return true;
        }
        if (!action->icon().isNull() || !action->property("ElaIconType").toString().isEmpty())
        {
            return true;
        }
    }
    return false;
}

void ElaMenu::setShowMenuAnimation(bool on)
{
    _showMenuAnimation = on;
    GlobalObjects::appSetting->setValue(SETTING_KEY_SHOW_MENU_ANIME, on);
}

void ElaMenu::showEvent(QShowEvent* event)
{
    Q_EMIT menuShow();
    Q_D(ElaMenu);
    //消除阴影偏移
    if (d->_isSubMenu && this->pos().x() < QCursor::pos().x())
        move(this->pos().x() + 8, this->pos().y());
    if (!d->_animationPix.isNull())
    {
        d->_animationPix = QPixmap();
    }
    if (!_showMenuAnimation)
    {
        QMenu::showEvent(event);
        return;
    }
    d->_animationPix = this->grab(this->rect());
    QPropertyAnimation* posAnimation = new QPropertyAnimation(d, "pAnimationImagePosY");
    connect(posAnimation, &QPropertyAnimation::finished, this, [=]() {
        d->_animationPix = QPixmap();
        update();
    });
    connect(posAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
        update();
    });
    posAnimation->setEasingCurve(QEasingCurve::OutCubic);
    posAnimation->setDuration(400);
    int targetPosY = height();
    if (targetPosY > 160)
    {
        if (targetPosY < 320)
        {
            targetPosY = 160;
        }
        else
        {
            targetPosY /= 2;
        }
    }

    if (pos().y() + d->_menuStyle->getMenuItemHeight() + 9 >= QCursor::pos().y())
    {
        posAnimation->setStartValue(-targetPosY);
    }
    else
    {
        posAnimation->setStartValue(targetPosY);
    }

    posAnimation->setEndValue(0);
    posAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    QMenu::showEvent(event);
}

void ElaMenu::paintEvent(QPaintEvent* event)
{
    Q_D(ElaMenu);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    if (!d->_animationPix.isNull())
    {
        painter.drawPixmap(QRect(0, d->_pAnimationImagePosY, width(), height()), d->_animationPix);
    }
    else
    {
        QMenu::paintEvent(event);
    }
}

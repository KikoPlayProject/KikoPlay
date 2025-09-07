#include "ElaAppBarPrivate.h"

#include "../ElaToolButton.h"
#ifdef Q_OS_WIN
#include <Windows.h>
#endif
#include <QGuiApplication>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScreen>
#include <QVBoxLayout>
#include <QWidget>

#include "../ElaAppBar.h"
#include "../ElaIconButton.h"
//#include "../ElaText.h"
ElaAppBarPrivate::ElaAppBarPrivate(QObject* parent)
    : QObject{parent}
{
}

ElaAppBarPrivate::~ElaAppBarPrivate()
{
}

void ElaAppBarPrivate::onMinButtonClicked()
{
    Q_Q(ElaAppBar);
    q->window()->showMinimized();
}

void ElaAppBarPrivate::onMaxButtonClicked()
{
    Q_Q(ElaAppBar);
    bool isMaximized = q->window()->isMaximized();
    isMaximized ? q->window()->showNormal() : q->window()->showMaximized();
#ifndef Q_OS_WIM
    _changeMaxButtonAwesome(!isMaximized);
#endif
}

void ElaAppBarPrivate::onCloseButtonClicked()
{
    Q_Q(ElaAppBar);
    if (_pIsDefaultClosed)
    {
        q->window()->close();
    }
    else
    {
        Q_EMIT q_ptr->closeButtonClicked();
    }
}

void ElaAppBarPrivate::onStayTopButtonClicked()
{
#ifdef Q_OS_WIN
    HWND hwnd = (HWND)_currentWinID;
    ::SetWindowPos(hwnd, _pIsStayTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#else
    Q_Q(const ElaAppBar);
    bool isVisible = q->window()->isVisible();
    q->window()->setWindowFlag(Qt::WindowStaysOnTopHint, _pIsStayTop);
    if (isVisible)
    {
        q->window()->show();
    }
#endif
    if (_stayTopButton)
    {
        _stayTopButton->setIsSelected(_pIsStayTop);
        _stayTopButton->update();
    }
}

void ElaAppBarPrivate::_changeMaxButtonAwesome(bool isMaximized)
{
    if (!_maxButton) return;
    if (isMaximized)
    {
        _maxButton->setAwesome(ElaIconType::WindowRestore);
    }
    else
    {
        _maxButton->setAwesome(ElaIconType::Square);
    }
}

void ElaAppBarPrivate::_showSystemMenu(QPoint point)
{
    Q_Q(const ElaAppBar);
    if (_controlMode != ElaAppBarControlType::Main) return;
#ifdef Q_OS_WIN
    QScreen* screen = qApp->screenAt(QCursor::pos());
    if (!screen)
    {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen)
    {
        return;
    }
    const QPoint origin = screen->geometry().topLeft();
    auto nativePos = QPointF(QPointF(point - origin) * screen->devicePixelRatio()).toPoint() + origin;
    HWND hwnd = reinterpret_cast<HWND>(q->window()->winId());
    const HMENU hMenu = ::GetSystemMenu(hwnd, FALSE);
    if (q->window()->isMaximized() || q->window()->isFullScreen())
    {
        ::EnableMenuItem(hMenu, SC_MOVE, MFS_DISABLED);
        ::EnableMenuItem(hMenu, SC_RESTORE, MFS_ENABLED);
    }
    else
    {
        ::EnableMenuItem(hMenu, SC_MOVE, MFS_ENABLED);
        ::EnableMenuItem(hMenu, SC_RESTORE, MFS_DISABLED);
    }
    if (!_pIsFixedSize && !q->window()->isMaximized() && !q->window()->isFullScreen())
    {
        ::EnableMenuItem(hMenu, SC_SIZE, MFS_ENABLED);
        ::EnableMenuItem(hMenu, SC_MAXIMIZE, MFS_ENABLED);
    }
    else
    {
        ::EnableMenuItem(hMenu, SC_SIZE, MFS_DISABLED);
        ::EnableMenuItem(hMenu, SC_MAXIMIZE, MFS_DISABLED);
    }
    const int result = ::TrackPopupMenu(hMenu, (TPM_RETURNCMD | (QGuiApplication::isRightToLeft() ? TPM_RIGHTALIGN : TPM_LEFTALIGN)), nativePos.x(),
                                        nativePos.y(), 0, hwnd, nullptr);
    if (result != FALSE)
    {
        ::PostMessageW(hwnd, WM_SYSCOMMAND, result, 0);
    }
#endif
}

void ElaAppBarPrivate::_updateCursor(int edges)
{
    Q_Q(const ElaAppBar);
    switch (edges)
    {
    case 0:
    {
        q->window()->setCursor(Qt::ArrowCursor);
        break;
    }
    case Qt::LeftEdge:
    case Qt::RightEdge:
    {
        q->window()->setCursor(Qt::SizeHorCursor);
        break;
    }
    case Qt::TopEdge:
    case Qt::BottomEdge:
    {
        q->window()->setCursor(Qt::SizeVerCursor);
        break;
    }
    case Qt::LeftEdge | Qt::TopEdge:
    case Qt::RightEdge | Qt::BottomEdge:
    {
        q->window()->setCursor(Qt::SizeFDiagCursor);
        break;
    }
    case Qt::RightEdge | Qt::TopEdge:
    case Qt::LeftEdge | Qt::BottomEdge:
    {
        q->window()->setCursor(Qt::SizeBDiagCursor);
        break;
    }
    default:
    {
        break;
    }
    }
}

bool ElaAppBarPrivate::_containsCursorToItem(QWidget* item)
{
    Q_Q(const ElaAppBar);
    if (!item || !item->isVisible())
    {
        return false;
    }
    auto point = item->window()->mapFromGlobal(QCursor::pos());
    QRectF rect = QRectF(item->mapTo(item->window(), QPoint(0, 0)), item->size());
    if (item == q)
    {
        if (_containsCursorToItem(_iconButton) || _containsCursorToItem(_minButton) || _containsCursorToItem(_appButton) ||
            _containsCursorToItem(_maxButton) || _containsCursorToItem(_closeButton))
        {
            return false;
        }
        for (QWidget *w : _customWidgets)
        {
            if (_containsCursorToItem(w))
            {
                return false;
            }
        }
    }
    if (rect.contains(point))
    {
        return true;
    }
    return false;
}

int ElaAppBarPrivate::_calculateMinimumWidth()
{
    Q_Q(ElaAppBar);
    int width = 5;
    if (_titleLabel && _titleLabel->isVisible())
    {
        width += _titleLabel->width();
        width += 10;
    }
    if (_iconButton && _iconButton->isVisible())
    {
        width += _iconButton->width();
        width += 10;
    }
    QList<QAbstractButton*> buttonList = q->findChildren<QAbstractButton*>();
    for (auto button : buttonList)
    {
        if (button->isVisible())
        {
            width += button->width();
        }
    }
    return width;
}

QVBoxLayout* ElaAppBarPrivate::_createVLayout(QWidget* widget)
{
    if (!widget)
    {
        return nullptr;
    }
    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    if (widget == _iconLabel || widget == _titleLabel)
    {
        vLayout->addSpacing(6);
    }
    vLayout->addWidget(widget);
    vLayout->addStretch();
    return vLayout;
}

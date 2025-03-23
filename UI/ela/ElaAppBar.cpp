#include "ElaAppBar.h"

#include <QApplication>
#include <QDebug>

//#include "ElaText.h"
#include "ElaToolButton.h"
#include "ElaWinShadowHelper.h"
#ifndef Q_OS_WIN
#include <QDateTime>
#include <QWindow>
#endif
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include "Def.h"
#include "ElaEventBus.h"
#include "ElaIconButton.h"
#include "ElaTheme.h"
#include "private/ElaAppBarPrivate.h"
Q_PROPERTY_CREATE_Q_CPP(ElaAppBar, bool, IsStayTop)
Q_PROPERTY_CREATE_Q_CPP(ElaAppBar, bool, IsDefaultClosed)
Q_PROPERTY_CREATE_Q_CPP(ElaAppBar, bool, IsOnlyAllowMinAndClose)

ElaAppBar::ElaAppBar(QWidget* parent)
    : QWidget{parent}, d_ptr(new ElaAppBarPrivate())
{
    Q_D(ElaAppBar);
    d->_buttonFlags = ElaAppBarType::TitleHint | ElaAppBarType::IconButtonHint | ElaAppBarType::MinimizeButtonHint | ElaAppBarType::MaximizeButtonHint | ElaAppBarType::CloseButtonHint;
    window()->setAttribute(Qt::WA_Mapped);
    d->_pAppBarHeight = 46;
    setFixedHeight(d->_pAppBarHeight);
    window()->setContentsMargins(0, this->height(), 0, 0);
    d->q_ptr = this;
    d->_pIsStayTop = false;
    d->_pIsFixedSize = false;
    d->_pIsDefaultClosed = true;
    d->_pIsOnlyAllowMinAndClose = false;
    d->_pCustomWidget = nullptr;

    window()->installEventFilter(this);
#ifdef Q_OS_WIN
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 3) && QT_VERSION <= QT_VERSION_CHECK(6, 6, 1))
    window()->setWindowFlags((window()->windowFlags()) | Qt::WindowMinimizeButtonHint | Qt::FramelessWindowHint);
#endif
#else
    window()->setWindowFlags((window()->windowFlags()) | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
#endif
    setMouseTracking(true);
    setObjectName("ElaAppBar");
    setStyleSheet("#ElaAppBar{background-color:transparent;}");

    const QColor btnColor(220, 220, 220);
    const QColor hoverColor(255, 255, 255, 25);

    d->_stayTopButton = new ElaToolButton(this);
    d->_stayTopButton->setElaIcon(ElaIconType::ArrowUpToArc);
    d->_stayTopButton->setFixedSize(40, 30);
    connect(d->_stayTopButton, &ElaIconButton::clicked, this, [=]() { this->setIsStayTop(!this->getIsStayTop()); });
    connect(this, &ElaAppBar::pIsStayTopChanged, d, &ElaAppBarPrivate::onStayTopButtonClicked);

    d->_iconButton = new ElaToolButton(this);
    d->_iconButton->setIconSize(QSize(18, 18));
    d->_iconButton->setMinimumWidth(48);
    ElaSelfTheme *iconSelfTheme = new ElaSelfTheme(d->_iconButton);
    iconSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicText, btnColor);
    iconSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicText, btnColor);
    iconSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicHoverAlpha, hoverColor);
    d->_iconButton->setSelfTheme(iconSelfTheme);
    if (parent->windowIcon().isNull())
    {
        d->_iconButton->setVisible(false);
    }
    else
    {
        d->_iconButton->setIcon(parent->windowIcon().pixmap(18, 18));
    }
    connect(parent, &QWidget::windowIconChanged, this, [=](const QIcon& icon) {
        d->_iconButton->setIcon(icon.pixmap(18, 18));
        d->_iconButton->setVisible(icon.isNull() ? false : true);
    });

    d->_titleLabel = new QLabel(this);
    d->_titleLabel->setFont(QFont(d->_titleLabel->font().family(), 13));
    if (parent->windowTitle().isEmpty())
    {
        d->_titleLabel->setVisible(false);
    }
    else
    {
        d->_titleLabel->setText(parent->windowTitle());
    }
    connect(parent, &QWidget::windowTitleChanged, this, [=](const QString& title) {
        d->_titleLabel->setText(title);
        d->_titleLabel->setVisible(title.isEmpty() ? false : true);
    });

    d->_appButton = new ElaToolButton(this);
    d->_appButton->setElaIcon(ElaIconType::Grid2);
    d->_appButton->setIconSize(QSize(24, 24));
    d->_appButton->setBorderRadius(0);
    d->_appButton->setFixedSize(45, 45);
    ElaSelfTheme *appSelfTheme = new ElaSelfTheme(d->_appButton);
    appSelfTheme->setThemeColor(ElaThemeType::Light, ElaThemeType::BasicText, btnColor);
    appSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicText, btnColor);
    appSelfTheme->setThemeColor(ElaThemeType::Dark, ElaThemeType::BasicHoverAlpha, hoverColor);
    d->_appButton->setSelfTheme(appSelfTheme);

    d->_minButton = new ElaIconButton(ElaIconType::WindowMinimize, 16, 45, 45, this);
    d->_minButton->setLightIconColor(btnColor);
    d->_minButton->setLightHoverIconColor(btnColor);
    d->_minButton->setDarkHoverColor(hoverColor);
    connect(d->_minButton, &ElaIconButton::clicked, d, &ElaAppBarPrivate::onMinButtonClicked);

    d->_maxButton = new ElaIconButton(ElaIconType::Square, 16, 45, 45, this);
    d->_maxButton->setLightIconColor(btnColor);
    d->_maxButton->setLightHoverIconColor(btnColor);
    d->_maxButton->setDarkHoverColor(hoverColor);
    connect(d->_maxButton, &ElaIconButton::clicked, d, &ElaAppBarPrivate::onMaxButtonClicked);

    d->_closeButton = new ElaIconButton(ElaIconType::Xmark, 18, 45, 45, this);
    d->_closeButton->setLightHoverColor(QColor(0xE8, 0x11, 0x23));
    d->_closeButton->setDarkHoverColor(QColor(0xE8, 0x11, 0x23));
    d->_closeButton->setLightHoverIconColor(btnColor);
    d->_closeButton->setDarkHoverIconColor(btnColor);
    d->_closeButton->setLightIconColor(btnColor);

    connect(d->_closeButton, &ElaIconButton::clicked, d, &ElaAppBarPrivate::onCloseButtonClicked);

    d->_mainLayout = new QHBoxLayout(this);
    d->_mainLayout->setContentsMargins(12, 0, 0, 0);
    d->_mainLayout->setSpacing(0);
    d->_mainLayout->addWidget(d->_iconButton);
    d->_mainLayout->addWidget(d->_titleLabel);
    d->_mainLayout->addStretch();
    d->_mainLayout->addStretch();
    d->_mainLayout->addWidget(d->_stayTopButton);
    d->_mainLayout->addWidget(d->_appButton);
    d->_mainLayout->addWidget(d->_minButton);
    d->_mainLayout->addWidget(d->_maxButton);
    d->_mainLayout->addWidget(d->_closeButton);

#ifdef Q_OS_WIN
    for (int i = 0; i < qApp->screens().count(); i++)
    {
        connect(qApp->screens().at(i), &QScreen::logicalDotsPerInchChanged, this, [=] {
            if (d->_pIsFixedSize)
            {
                HWND hwnd = (HWND)(d->_currentWinID);
                SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
            }
        });
    }
    //主屏幕变更处理
    connect(qApp, &QApplication::primaryScreenChanged, this, [=]() {
        HWND hwnd = (HWND)(d->_currentWinID);
        ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        ::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    });
    d->_lastScreen = qApp->screenAt(window()->geometry().center());
#endif
}

ElaAppBar::~ElaAppBar()
{
}

void ElaAppBar::setAppBarHeight(int height)
{
    Q_D(ElaAppBar);
    d->_pAppBarHeight = height;
    setFixedHeight(d->_pAppBarHeight);
    window()->setContentsMargins(0, this->height(), 0, 0);
    Q_EMIT pAppBarHeightChanged();
}

int ElaAppBar::getAppBarHeight() const
{
    Q_D(const ElaAppBar);
    return d->_pAppBarHeight;
}

void ElaAppBar::setCustomWidget(ElaAppBarType::CustomArea customArea, QWidget* widget)
{
    Q_D(ElaAppBar);
    if (!widget || widget == this)
    {
        return;
    }
    widget->setMinimumHeight(0);
    widget->setMaximumHeight(height());
    widget->setParent(this);
    switch (customArea)
    {
    case ElaAppBarType::LeftArea:
    {
        d->_mainLayout->insertWidget(2, widget);
        break;
    }
    case ElaAppBarType::MiddleArea:
    {
        d->_mainLayout->insertWidget(3 + d->_customWidgets.size(), widget);
        break;
    }
    case ElaAppBarType::RightArea:
    {
        d->_mainLayout->insertWidget(4 + d->_customWidgets.size(), widget);
        break;
    }
    }
    d->_customWidgets.append(widget);
}

QWidget* ElaAppBar::getCustomWidget() const
{
    Q_D(const ElaAppBar);
    return d->_pCustomWidget;
}

void ElaAppBar::setCustomWidgetMaximumWidth(int width)
{
    Q_D(ElaAppBar);
    d->_pCustomWidgetMaximumWidth = width;
    if (d->_pCustomWidget)
    {
        d->_pCustomWidget->setMaximumWidth(width);
    }
    Q_EMIT pCustomWidgetMaximumWidthChanged();
}

int ElaAppBar::getCustomWidgetMaximumWidth() const
{
    Q_D(const ElaAppBar);
    return d->_pCustomWidgetMaximumWidth;
}

void ElaAppBar::setIsFixedSize(bool isFixedSize)
{
    Q_D(ElaAppBar);
    d->_pIsFixedSize = isFixedSize;
#ifdef Q_OS_WIN
    HWND hwnd = (HWND)d->_currentWinID;
    DWORD style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
    if (d->_pIsFixedSize)
    {
        //切换DPI处理
        style &= ~WS_THICKFRAME;
        ::SetWindowLongPtr(hwnd, GWL_STYLE, style);
    }
    else
    {
        ::SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME);
    }
#else
    bool isVisible = window()->isVisible();
    window()->setWindowFlags((window()->windowFlags()) | Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    if (!isFixedSize)
    {
        window()->setWindowFlag(Qt::WindowMaximizeButtonHint);
    }
    if (isVisible)
    {
        window()->show();
    }
#endif
    Q_EMIT pIsFixedSizeChanged();
}

bool ElaAppBar::getIsFixedSize() const
{
    Q_D(const ElaAppBar);
    return d->_pIsFixedSize;
}

void ElaAppBar::setWindowControlFlag(ElaAppBarType::WindowControlType buttonFlag, bool isEnable)
{
    Q_D(ElaAppBar);
    if (isEnable)
    {
        setWindowControlFlags(d->_buttonFlags | buttonFlag);
    }
    else
    {
        setWindowControlFlags(d->_buttonFlags & ~buttonFlag);
    }
}

void ElaAppBar::setWindowControlFlags(ElaAppBarType::ButtonFlags buttonFlags)
{
    Q_D(ElaAppBar);
    d->_buttonFlags = buttonFlags;
    d->_appButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::AppButtonHint));
    d->_minButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::MinimizeButtonHint));
    d->_maxButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::MaximizeButtonHint));
    d->_closeButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::CloseButtonHint));
    d->_stayTopButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::StayTopButtonHint));
    d->_titleLabel->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::TitleHint));
    d->_iconButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::IconButtonHint));
}

ElaAppBarType::ButtonFlags ElaAppBar::getWindowControlFlags() const
{
    return d_ptr->_buttonFlags;
}

void ElaAppBar::setIcon(const QIcon &icon)
{
    Q_D(ElaAppBar);
    d->_iconButton->setIcon(icon);
}

ElaToolButton *ElaAppBar::iconButton()
{
    Q_D(ElaAppBar);
    return d->_iconButton;
}

ElaToolButton *ElaAppBar::appButton()
{
    Q_D(ElaAppBar);
    return d->_appButton;
}

void ElaAppBar::hideAppBar(bool on, bool isFullScreen)
{
    Q_D(ElaAppBar);
    d->_isFullScreen = isFullScreen;
    if (on)
    {
        this->hide();
        window()->setContentsMargins(0, 0, 0, 0);
    }
    else
    {
        this->show();
        window()->setContentsMargins(0, this->height(), 0, 0);
    }
}

void ElaAppBar::setOnTop(bool on)
{
    Q_D(ElaAppBar);
    setIsStayTop(on);
    d->onStayTopButtonClicked();
}

void ElaAppBar::setScreenSave(bool on)
{
    Q_D(ElaAppBar);
    d->_screenSave = on;
}

void ElaAppBar::closeWindow()
{
    Q_D(ElaAppBar);
    QPropertyAnimation* closeOpacityAnimation = new QPropertyAnimation(window(), "windowOpacity");
    connect(closeOpacityAnimation, &QPropertyAnimation::finished, this, [=]() {
        window()->close();
    });
    closeOpacityAnimation->setStartValue(1);
    closeOpacityAnimation->setEndValue(0);
    closeOpacityAnimation->setEasingCurve(QEasingCurve::InOutSine);
    closeOpacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    if (window()->isMaximized() || window()->isFullScreen() || d->_pIsFixedSize)
    {
        return;
    }
    QPropertyAnimation* geometryAnimation = new QPropertyAnimation(window(), "geometry");
    QRect geometry = window()->geometry();
    geometryAnimation->setStartValue(geometry);
    qreal targetWidth = (geometry.width() - d->_lastMinTrackWidth) * 0.7 + d->_lastMinTrackWidth;
    qreal targetHeight = (geometry.height() - window()->minimumHeight()) * 0.7 + window()->minimumHeight();
    geometryAnimation->setEndValue(QRectF(geometry.center().x() - targetWidth / 2, geometry.center().y() - targetHeight / 2, targetWidth, targetHeight));
    geometryAnimation->setEasingCurve(QEasingCurve::InOutSine);
    geometryAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
int ElaAppBar::takeOverNativeEvent(const QByteArray& eventType, void* message, qintptr* result)
#else
int ElaAppBar::takeOverNativeEvent(const QByteArray& eventType, void* message, long* result)
#endif
{
    Q_D(ElaAppBar);
    if ((eventType != "windows_generic_MSG") || !message)
    {
        return 0;
    }
    const auto msg = static_cast<const MSG*>(message);
    const HWND hwnd = msg->hwnd;
    if (!hwnd || !msg)
    {
        return 0;
    }
    d->_currentWinID = (qint64)hwnd;
    const UINT uMsg = msg->message;
    const WPARAM wParam = msg->wParam;
    const LPARAM lParam = msg->lParam;
    switch (uMsg)
    {
    case WM_WINDOWPOSCHANGING:
    {
        WINDOWPOS* wp = reinterpret_cast<WINDOWPOS*>(lParam);
        if (wp != nullptr && (wp->flags & SWP_NOSIZE) == 0)
        {
            wp->flags |= SWP_NOCOPYBITS;
            *result = ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
            return 1;
        }
        return 0;
    }
    case WM_NCACTIVATE:
    {
        *result = TRUE;
        return 1;
    }
    case WM_SYSCOMMAND:
    {
        if (wParam == SC_SCREENSAVE && !d->_screenSave)
        {
            *result = 1;
            return 1;
        }
        return 0;
    }
    case WM_SIZE:
    {
        if (wParam == SIZE_RESTORED)
        {
            d->_changeMaxButtonAwesome(false);
        }
        else if (wParam == SIZE_MAXIMIZED)
        {
            d->_changeMaxButtonAwesome(true);
        }
        return 0;
    }
    case WM_NCCALCSIZE:
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 3) && QT_VERSION <= QT_VERSION_CHECK(6, 6, 1))
        if (wParam == FALSE)
        {
            return 0;
        }
        if (::IsZoomed(hwnd))
        {
            this->move(7, 7);
            window()->setContentsMargins(8, 8 + height(), 8, 8);
        }
        else
        {
            this->move(0, 0);
            window()->setContentsMargins(0, height(), 0, 0);
        }
        *result = 0;
        return 1;
#else
        if (wParam == FALSE)
        {
            return 0;
        }
        RECT* clientRect = &((NCCALCSIZE_PARAMS*)(lParam))->rgrc[0];
        if (!::IsZoomed(hwnd))
        {
            clientRect->top -= 1;
            if (!d->_isFullScreen)
                clientRect->bottom -= 1;
        }
        else
        {
            const LRESULT hitTestResult = ::DefWindowProcW(hwnd, WM_NCCALCSIZE, wParam, lParam);
            if ((hitTestResult != HTERROR) && (hitTestResult != HTNOWHERE))
            {
                *result = static_cast<long>(hitTestResult);
                return 1;
            }
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            auto geometry = window()->screen()->geometry();
#else
            QScreen* screen = qApp->screenAt(window()->geometry().center());
            QRect geometry;
            if (!screen)
            {
                screen = qApp->screenAt(QCursor::pos());
            }
            geometry = screen->geometry();
#endif
            clientRect->top = geometry.top();
        }
        *result = WVR_REDRAW;
        return 1;
#endif
    }
    case WM_MOVE:
    {
        QScreen* currentScreen = qApp->screenAt(window()->geometry().center());
        if (currentScreen && currentScreen != d->_lastScreen)
        {
            if (d->_lastScreen)
            {
                ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                ::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            d->_lastScreen = currentScreen;
        }
        break;
    }
    case WM_NCHITTEST:
    {
        if (d->_containsCursorToItem(d->_maxButton))
        {
            if (*result == HTNOWHERE)
            {
                if (!d->_isHoverMaxButton)
                {
                    d->_isHoverMaxButton = true;
                    d->_maxButton->setIsSelected(true);
                    d->_maxButton->update();
                }
                *result = HTZOOM;
            }
            return 1;
        }
        else
        {
            if (d->_isHoverMaxButton)
            {
                d->_isHoverMaxButton = false;
                d->_maxButton->setIsSelected(false);
                d->_maxButton->update();
            }
        }
        POINT nativeLocalPos{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ::ScreenToClient(hwnd, &nativeLocalPos);
        RECT clientRect{0, 0, 0, 0};
        ::GetClientRect(hwnd, &clientRect);
        auto clientWidth = clientRect.right - clientRect.left;
        auto clientHeight = clientRect.bottom - clientRect.top;
        bool left = nativeLocalPos.x < d->_margins;
        bool right = nativeLocalPos.x > clientWidth - d->_margins;
        bool top = nativeLocalPos.y < d->_margins;
        bool bottom = nativeLocalPos.y > clientHeight - d->_margins;
        *result = 0;
        if (!d->_pIsOnlyAllowMinAndClose && !d->_pIsFixedSize && !window()->isFullScreen() && !window()->isMaximized())
        {
            if (left && top)
            {
                *result = HTTOPLEFT;
            }
            else if (left && bottom)
            {
                *result = HTBOTTOMLEFT;
            }
            else if (right && top)
            {
                *result = HTTOPRIGHT;
            }
            else if (right && bottom)
            {
                *result = HTBOTTOMRIGHT;
            }
            else if (left)
            {
                *result = HTLEFT;
            }
            else if (right)
            {
                *result = HTRIGHT;
            }
            else if (top)
            {
                *result = HTTOP;
            }
            else if (bottom)
            {
                *result = HTBOTTOM;
            }
        }
        if (0 != *result)
        {
            return 1;
        }
        if (d->_containsCursorToItem(this))
        {
            *result = HTCAPTION;
            return 1;
        }
        *result = HTCLIENT;
        return 1;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* minmaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
        RECT rect;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
        d->_lastMinTrackWidth = d->_calculateMinimumWidth();
        minmaxInfo->ptMinTrackSize.x = d->_lastMinTrackWidth * qApp->devicePixelRatio();
        minmaxInfo->ptMinTrackSize.y = parentWidget()->minimumHeight() * qApp->devicePixelRatio();
        minmaxInfo->ptMaxPosition.x = rect.left;
        minmaxInfo->ptMaxPosition.y = rect.top;
        return 1;
    }
    case WM_LBUTTONDBLCLK:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMLBUTTONDBLCLK);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMLBUTTONDOWN);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        return 0;
    }
    case WM_LBUTTONUP:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMLBUTTONUP);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        return 0;
    }
    case WM_NCLBUTTONDOWN:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMNCLBUTTONDOWN);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        if (d->_containsCursorToItem(d->_maxButton) || d->_pIsOnlyAllowMinAndClose)
        {
            return 1;
        }
        break;
    }
    case WM_NCLBUTTONUP:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMNCLBUTTONDOWN);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        if (d->_containsCursorToItem(d->_maxButton) && !d->_pIsOnlyAllowMinAndClose)
        {
            d->onMaxButtonClicked();
            return 1;
        }
        break;
    }
    case WM_NCLBUTTONDBLCLK:
    {
        if (!d->_pIsOnlyAllowMinAndClose && !d->_pIsFixedSize)
        {
            return 0;
        }
        return 1;
    }
    case WM_NCRBUTTONDOWN:
    {
        if (wParam == HTCAPTION && !d->_pIsOnlyAllowMinAndClose)
        {
            d->_showSystemMenu(QCursor::pos());
        }
        break;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(VK_SPACE) & 0x8000) && !d->_pIsOnlyAllowMinAndClose)
        {
            auto pos = window()->geometry().topLeft();
            d->_showSystemMenu(QPoint(pos.x(), pos.y() + this->height()));
        }
        break;
    }
    }
    return -1;
}
#endif

bool ElaAppBar::eventFilter(QObject* obj, QEvent* event)
{
    Q_D(ElaAppBar);
    switch (event->type())
    {
    case QEvent::Resize:
    {
        QSize size = parentWidget()->size();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 3) && QT_VERSION <= QT_VERSION_CHECK(6, 6, 1))
        if (::IsZoomed((HWND)d->_currentWinID))
        {
            this->resize(size.width() - 14, this->height());
        }
        else
        {
            this->resize(size.width(), this->height());
        }
#else
        this->resize(size.width(), this->height());
#endif
        break;
    }
#ifdef Q_OS_WIN
    case QEvent::Show:
    {
        if (!d->_pIsFixedSize && !d->_pIsOnlyAllowMinAndClose)
        {
            HWND hwnd = (HWND)d->_currentWinID;
            DWORD style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
            ::SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME);
        }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 3) && QT_VERSION <= QT_VERSION_CHECK(6, 6, 1))
        HWND hwnd = (HWND)d->_currentWinID;
        setShadow(hwnd);
        DWORD style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
        bool hasCaption = (style & WS_CAPTION) == WS_CAPTION;
        if (!hasCaption)
        {
            ::SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_CAPTION);
        }
#endif
        break;
    }
#endif
    case QEvent::Close:
    {
        QCloseEvent* closeEvent = dynamic_cast<QCloseEvent*>(event);
        if (!d->_pIsDefaultClosed && closeEvent->spontaneous())
        {
            event->ignore();
            if (window()->isMinimized())
            {
                window()->showNormal();
            }
            d->onCloseButtonClicked();
        }
        return true;
    }
#ifndef Q_OS_WIN
    case QEvent::MouseButtonPress:
    {
        if (d->_edges != 0)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                d->_updateCursor(d->_edges);
                window()->windowHandle()->startSystemResize(Qt::Edges(d->_edges));
            }
        }
        else
        {
            if (d->_containsCursorToItem(this))
            {
                qint64 clickTimer = QDateTime::currentMSecsSinceEpoch();
                qint64 offset = clickTimer - d->_clickTimer;
                d->_clickTimer = clickTimer;
                if (offset > 300)
                {
                    window()->windowHandle()->startSystemMove();
                }
            }
        }
        break;
    }
    case QEvent::MouseButtonDblClick:
    {
        if (d->_containsCursorToItem(this))
        {
            if (window()->isMaximized())
            {
                window()->showNormal();
            }
            else
            {
                window()->showMaximized();
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease:
    {
        d->_edges = 0;
        break;
    }
    case QEvent::HoverMove:
    {
        if (window()->isMaximized() || window()->isFullScreen())
        {
            break;
        }
        if (d->_pIsFixedSize)
        {
            break;
        }
        QHoverEvent* mouseEvent = static_cast<QHoverEvent*>(event);
        QPoint p =
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            mouseEvent->pos();
#else
            mouseEvent->position().toPoint();
#endif
        if (p.x() >= d->_margins && p.x() <= (window()->width() - d->_margins) && p.y() >= d->_margins && p.y() <= (window()->height() - d->_margins))
        {
            if (d->_edges != 0)
            {
                d->_edges = 0;
                d->_updateCursor(d->_edges);
            }
            break;
        }
        d->_edges = 0;
        if (p.x() < d->_margins)
        {
            d->_edges |= Qt::LeftEdge;
        }
        if (p.x() > (window()->width() - d->_margins))
        {
            d->_edges |= Qt::RightEdge;
        }
        if (p.y() < d->_margins)
        {
            d->_edges |= Qt::TopEdge;
        }
        if (p.y() > (window()->height() - d->_margins))
        {
            d->_edges |= Qt::BottomEdge;
        }
        d->_updateCursor(d->_edges);
        break;
    }
#endif
    default:
    {
        break;
    }
    }
    return QObject::eventFilter(obj, event);
}

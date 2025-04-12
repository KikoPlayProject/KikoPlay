#include "appframelessdialog.h"
#include <QApplication>
#include <QPoint>
#include <QSize>
#include <QKeyEvent>
#include "UI/widgets/dialogtip.h"
#include "UI/widgets/loadingicon.h"
#ifdef Q_OS_WIN

#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <objidl.h> // Fixes error C2504: 'IUnknown' : base class undefined
#include <gdiplus.h>
#include <GdiPlusColor.h>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include "globalobjects.h"
#pragma comment (lib,"Dwmapi.lib") // Adds missing library, fixes error LNK2019: unresolved external symbol __imp__DwmExtendFrameIntoClientArea
#pragma comment (lib,"user32.lib")

AppFramelessDialog::AppFramelessDialog(const QString &titleStr, QWidget *parent)
    : QDialog(parent), m_borderWidth(5), m_bJustMaximized(false), m_bResizeable(true),
      inited(false), isBusy(false), isPin(false)
{

    setWindowFlags((windowFlags() | Qt::Dialog | Qt::FramelessWindowHint) & ~Qt::WindowSystemMenuHint);
    setObjectName(QStringLiteral("framelessDialog"));
    backWidget = new QWidget(this);
    titleBar = new QWidget(backWidget);

    title=new QLabel(titleStr, titleBar);
    title->setFont(QFont(GlobalObjects::normalFont, 13));
    title->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    title->setOpenExternalLinks(true);

    GlobalObjects::iconfont->setPointSize(title->font().pointSize());

    QFontMetrics fm(*GlobalObjects::iconfont);
    int btnH = qMax(fm.horizontalAdvance(QChar(0xe60b)), fm.horizontalAdvance(QChar(0xe680)));
    btnH = qMax(btnH, fm.height())*1.6;
    QSize btnSize(btnH, btnH);

    closeButton=new QPushButton(titleBar);
    closeButton->setObjectName(QStringLiteral("DialogCloseButton"));
    closeButton->setFixedSize(btnSize);
    closeButton->setFont(*GlobalObjects::iconfont);
    closeButton->setText(QChar(0xe60b));
    QObject::connect(closeButton,&QPushButton::clicked,this,&AppFramelessDialog::reject);
    QObject::connect(this, &AppFramelessDialog::rejected, this, &AppFramelessDialog::onClose);
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    hideButton=new QPushButton(titleBar);
    hideButton->setObjectName(QStringLiteral("DialogHideButton"));
    hideButton->setFixedSize(btnSize);
    hideButton->setFont(*GlobalObjects::iconfont);
    hideButton->setText(QChar(0xe651));
    QObject::connect(hideButton,&QPushButton::clicked,this,&AppFramelessDialog::onHide);
    hideButton->setDefault(false);
    hideButton->setAutoDefault(false);

    pinButton=new QPushButton(titleBar);
    pinButton->setObjectName(QStringLiteral("DialogHideButton"));
    pinButton->setFixedSize(btnSize);
    pinButton->setFont(*GlobalObjects::iconfont);
    pinButton->setText(QChar(0xe864));
    QObject::connect(pinButton,&QPushButton::clicked,this,&AppFramelessDialog::onPin);
    pinButton->setDefault(false);
    pinButton->setAutoDefault(false);

    busyLabel = new LoadingIcon(QColor(153, 153, 153), this);
    busyLabel->setFixedSize(QSize(btnH*0.9, btnH*0.9));
    busyLabel->hide();

    QHBoxLayout *titleHBLayout=new QHBoxLayout(titleBar);
    titleHBLayout->setContentsMargins(12, 8, 8, 8);
    titleHBLayout->addWidget(title, 0, Qt::AlignVCenter);
    titleHBLayout->addWidget(busyLabel);
    titleHBLayout->addSpacing(btnH * 0.2);
    titleHBLayout->addWidget(pinButton);
    titleHBLayout->addSpacing(btnH * 0.1);
    titleHBLayout->addWidget(hideButton);
    titleHBLayout->addSpacing(btnH * 0.1);
    titleHBLayout->addWidget(closeButton);

    QVBoxLayout *vbLayout = new QVBoxLayout(backWidget);
    vbLayout->addWidget(titleBar);
    vbLayout->addStretch(1);
    vbLayout->setContentsMargins(0, 0, 0, 0);
    addIgnoreWidget(titleBar);
    addIgnoreWidget(title);

    dialogTip = new DialogTip(this);
    dialogTip->hide();
}

void AppFramelessDialog::setResizeable(bool resizeable)
{
    bool visible = isVisible();
    m_bResizeable = resizeable;

    HWND hwnd = (HWND)this->winId();
    DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
    ::SetWindowLong(hwnd, GWL_STYLE, style  | WS_THICKFRAME | WS_CAPTION);

    const MARGINS shadow = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(HWND(winId()), &shadow);

    setVisible(visible);
}

void AppFramelessDialog::setResizeableAreaWidth(int width)
{
    if (1 > width) width = 1;
    m_borderWidth = width;
}

void AppFramelessDialog::onHide()
{
    if (onHideCallback)
    {
        if (!onHideCallback())
        {
            return;
        }
    }
    this->hide();
}

void AppFramelessDialog::onClose()
{
    if (onCloseCallback)
    {
        if (!onCloseCallback())
        {
            return;
        }
    }
    // this->done(0);
}

void AppFramelessDialog::onPin()
{
    isPin = !isPin;
    HWND hwnd = (HWND)this->winId();
    if(isPin)
    {
        SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
        pinButton->setText(QChar(0xe863));
    }
    else
    {
        SetWindowPos(hwnd,HWND_NOTOPMOST ,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
        pinButton->setText(QChar(0xe864));
    }
}

void AppFramelessDialog::addIgnoreWidget(QWidget* widget)
{
    if (!widget) return;
    if (m_whiteList.contains(widget)) return;
    m_whiteList.append(widget);
}

bool AppFramelessDialog::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG* msg = (MSG *)message;
    switch (msg->message)
    {
    case WM_NCCALCSIZE:
    {
        //this kills the window frame and title bar we added with WS_THICKFRAME and WS_CAPTION
        *result = 0;
        return true;
    }
    case WM_NCHITTEST:
    {
        *result = 0;

        const LONG border_width = m_borderWidth;
        RECT winrect;
        GetWindowRect(HWND(winId()), &winrect);

        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        if(m_bResizeable)
        {

            bool resizeWidth = minimumWidth() != maximumWidth();
            bool resizeHeight = minimumHeight() != maximumHeight();

            if(resizeWidth)
            {
                //left border
                if (x >= winrect.left && x < winrect.left + border_width)
                {
                    *result = HTLEFT;
                }
                //right border
                if (x < winrect.right && x >= winrect.right - border_width)
                {
                    *result = HTRIGHT;
                }
            }
            if(resizeHeight)
            {
                //bottom border
                if (y < winrect.bottom && y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOM;
                }
                //top border
                if (y >= winrect.top && y < winrect.top + border_width)
                {
                    *result = HTTOP;
                }
            }
            if(resizeWidth && resizeHeight)
            {
                //bottom left corner
                if (x >= winrect.left && x < winrect.left + border_width &&
                        y < winrect.bottom && y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOMLEFT;
                }
                //bottom right corner
                if (x < winrect.right && x >= winrect.right - border_width &&
                        y < winrect.bottom && y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOMRIGHT;
                }
                //top left corner
                if (x >= winrect.left && x < winrect.left + border_width &&
                        y >= winrect.top && y < winrect.top + border_width)
                {
                    *result = HTTOPLEFT;
                }
                //top right corner
                if (x < winrect.right && x >= winrect.right - border_width &&
                        y >= winrect.top && y < winrect.top + border_width)
                {
                    *result = HTTOPRIGHT;
                }
            }
        }
        if (0!=*result) return true;

        //*result still equals 0, that means the cursor locate OUTSIDE the frame area
        //but it may locate in titlebar area
        //if (!m_titlebar) return false;

        //support highdpi
        double dpr = this->devicePixelRatioF();
        QPoint pos = mapFromGlobal(QPoint(x/dpr,y/dpr));

        if (!rect().contains(pos)) return false;
        QWidget* child = childAt(pos);
        if (!child)
        {
            *result = HTCAPTION;
            return true;
        }else{
            if (m_whiteList.contains(child))
            {
                *result = HTCAPTION;
                return true;
            }
        }
        return false;
    } //end case WM_NCHITTEST
    default:
        return QDialog::nativeEvent(eventType, message, result);
    }
}

void AppFramelessDialog::setContentsMargins(const QMargins &margins)
{
    QDialog::setContentsMargins(margins+m_frames);
    m_margins = margins;
}
void AppFramelessDialog::setContentsMargins(int left, int top, int right, int bottom)
{
    QDialog::setContentsMargins(left+m_frames.left(), top+m_frames.top(), right+m_frames.right(), bottom+m_frames.bottom());
    m_margins.setLeft(left);
    m_margins.setTop(top);
    m_margins.setRight(right);
    m_margins.setBottom(bottom);
}
QMargins AppFramelessDialog::contentsMargins() const
{
    QMargins margins = QDialog::contentsMargins();
    margins -= m_frames;
    return margins;
}
void AppFramelessDialog::getContentsMargins(int *left, int *top, int *right, int *bottom) const
{
    QMargins margin = QDialog::contentsMargins();
    // QDialog::getContentsMargins(left, top, right, bottom);
    *left = margin.left();
    *right = margin.right();
    *top = margin.top();
    *bottom = margin.bottom();
    if (!(left && top && right && bottom)) return;
    if (isMaximized())
    {
        *left -= m_frames.left();
        *top -= m_frames.top();
        *right -= m_frames.right();
        *bottom -= m_frames.bottom();
    }
}

void AppFramelessDialog::setPin(bool pin)
{
    isPin = !pin;
    onPin();
}

void AppFramelessDialog::showEvent(QShowEvent *)
{
    if(!inited)
    {
        setResizeable(m_bResizeable);
        inited=true;
    }
    setContentsMargins(8*logicalDpiX()/96,titleBar->height(),8*logicalDpiX()/96,8*logicalDpiY()/96);
    resize(width(),height());
}

void AppFramelessDialog::resizeEvent(QResizeEvent *)
{
    backWidget->setGeometry(0,0,width(),height());
    dialogTip->move((width()-dialogTip->width())/2,dialogTip->y());
}

void AppFramelessDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
        return;
    QDialog::keyPressEvent(event);
}


void AppFramelessDialog::showBusyState(bool busy)
{
    isBusy = busy;
    if(busy)
    {
        busyLabel->show();
        hideButton->setEnabled(false);
        closeButton->setEnabled(false);
    }
    else
    {
        busyLabel->hide();
        hideButton->setEnabled(true);
        closeButton->setEnabled(true);
    }
}

void AppFramelessDialog::setTitle(const QString &text)
{
    title->setText(text);
    setWindowTitle(text);
}

void AppFramelessDialog::showMessage(const QString &msg, int type)
{
    dialogTip->showMessage(msg, type);
}

void AppFramelessDialog::setCloseCallback(const std::function<bool ()> &func)
{
    onCloseCallback = func;
}

void AppFramelessDialog::setHideCallback(const std::function<bool ()> &func)
{
    onHideCallback = func;
}

QRect AppFramelessDialog::contentsRect() const
{
    QRect rect = QDialog::contentsRect();
    int width = rect.width();
    int height = rect.height();
    rect.setLeft(rect.left() - m_frames.left());
    rect.setTop(rect.top() - m_frames.top());
    rect.setWidth(width);
    rect.setHeight(height);
    return rect;
}
#else
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
AppFramelessDialog::AppFramelessDialog(const QString &titleStr, QWidget *parent)
    : QDialog(parent), m_borderWidth(5), m_bJustMaximized(false), m_bResizeable(true),
      isBusy(false), isMousePressed(false),resizeMouseDown(false)
{

    setWindowFlags((Qt::Dialog| Qt::FramelessWindowHint));
    setObjectName(QStringLiteral("framelessDialog_O"));
    setMouseTracking(true);

    titleBar = new QWidget(this);
    titleBar->setMouseTracking(true);
    titleBar->installEventFilter(this);

    backWidget = new QWidget(this);
    backWidget->setMouseTracking(true);
    backWidget->installEventFilter(this);

    title=new QLabel(titleStr, titleBar);
    title->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    title->setOpenExternalLinks(true);

    GlobalObjects::iconfont->setPointSize(title->font().pointSize());

    QFontMetrics fm(*GlobalObjects::iconfont);
    int btnH = qMax(fm.horizontalAdvance(QChar(0xe60b)), fm.horizontalAdvance(QChar(0xe680)));
    btnH = qMax(btnH, fm.height())*2;
    QSize btnSize(btnH, btnH);

    closeButton=new QPushButton(titleBar);
    closeButton->setObjectName(QStringLiteral("DialogCloseButton"));
    closeButton->setFixedSize(btnSize);
    closeButton->setFont(*GlobalObjects::iconfont);
    closeButton->setText(QChar(0xe60b));
    QObject::connect(closeButton,&QPushButton::clicked,this,&AppFramelessDialog::reject);
    QObject::connect(this, &AppFramelessDialog::rejected, this, &AppFramelessDialog::onClose);
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    hideButton=new QPushButton(titleBar);
    hideButton->setObjectName(QStringLiteral("DialogHideButton"));
    hideButton->setFixedSize(btnSize);
    hideButton->setFont(*GlobalObjects::iconfont);
    hideButton->setText(QChar(0xe651));
    QObject::connect(hideButton,&QPushButton::clicked,this,&AppFramelessDialog::onHide);
    hideButton->setDefault(false);
    hideButton->setAutoDefault(false);

    pinButton=new QPushButton(titleBar);
    pinButton->setObjectName(QStringLiteral("DialogHideButton"));
    pinButton->setFixedSize(btnSize);
    pinButton->setFont(*GlobalObjects::iconfont);
    pinButton->setText(QChar(0xe864));
    QObject::connect(pinButton,&QPushButton::clicked,this,&AppFramelessDialog::onPin);
    pinButton->setDefault(false);
    pinButton->setAutoDefault(false);

    busyLabel = new LoadingIcon(QColor(153, 153, 153), this);
    busyLabel->setFixedSize(btnSize);
    busyLabel->hide();

    QHBoxLayout *titleHBLayout=new QHBoxLayout(titleBar);
    titleHBLayout->setContentsMargins(8*logicalDpiX()/96, 8*logicalDpiY()/96, 8*logicalDpiX()/96, 8*logicalDpiY()/96);
    titleHBLayout->addWidget(title, 0, Qt::AlignVCenter);
    titleHBLayout->addWidget(busyLabel);
    titleHBLayout->addSpacing(btnH * 0.2);
    titleHBLayout->addWidget(pinButton);
    titleHBLayout->addSpacing(btnH * 0.1);
    titleHBLayout->addWidget(hideButton);
    titleHBLayout->addSpacing(btnH * 0.1);
    titleHBLayout->addWidget(closeButton);

    QVBoxLayout *vbLayout = new QVBoxLayout(backWidget);
    vbLayout->addWidget(titleBar);
    vbLayout->addStretch(1);
    vbLayout->setContentsMargins(0, 0, 0, 0);

    dialogTip = new DialogTip(this);
    dialogTip->hide();
}

void AppFramelessDialog::setResizeable(bool resizeable)
{
    m_bResizeable = resizeable;
}

void AppFramelessDialog::onHide()
{
    if (onHideCallback)
    {
        if (!onHideCallback())
        {
            return;
        }
    }
    this->hide();
}

void AppFramelessDialog::onPin()
{
    if (this->windowFlags() & Qt::WindowStaysOnTopHint)
    {
        this->setWindowFlag(Qt::WindowStaysOnTopHint, false);
        pinButton->setText(QChar(0xe864));
    }
    else
    {
        this->setWindowFlag(Qt::WindowStaysOnTopHint, true);
        pinButton->setText(QChar(0xe863));
    }
    show();
}

void AppFramelessDialog::onClose()
{
    if (onCloseCallback)
    {
        if (!onCloseCallback())
        {
            return;
        }
    }
    //this->done(0);
}

void AppFramelessDialog::setContentsMargins(const QMargins &margins)
{
    QDialog::setContentsMargins(margins+m_frames);
    m_margins = margins;
}
void AppFramelessDialog::setContentsMargins(int left, int top, int right, int bottom)
{
    QDialog::setContentsMargins(left+m_frames.left(),\
                                    top+m_frames.top(), \
                                    right+m_frames.right(), \
                                    bottom+m_frames.bottom());
    m_margins.setLeft(left);
    m_margins.setTop(top);
    m_margins.setRight(right);
    m_margins.setBottom(bottom);
}
QMargins AppFramelessDialog::contentsMargins() const
{
    QMargins margins = QDialog::contentsMargins();
    margins -= m_frames;
    return margins;
}
void AppFramelessDialog::getContentsMargins(int *left, int *top, int *right, int *bottom) const
{
    QDialog::getContentsMargins(left,top,right,bottom);
    if (!(left&&top&&right&&bottom)) return;
    if (isMaximized())
    {
        *left -= m_frames.left();
        *top -= m_frames.top();
        *right -= m_frames.right();
        *bottom -= m_frames.bottom();
    }
}

void AppFramelessDialog::showEvent(QShowEvent *e)
{
    setContentsMargins(8*logicalDpiX()/96,titleBar->height(),8*logicalDpiX()/96,8*logicalDpiY()/96);
    QDialog::showEvent(e);
}

bool AppFramelessDialog::eventFilter(QObject *obj, QEvent *e)
{
    if(obj==titleBar)
    {
        if(e->type() == QEvent::MouseButtonPress)
        {
            if(left || right || top)
            {
                QApplication::sendEvent(this, e);
                return true;
            }
            QMouseEvent *event = static_cast<QMouseEvent*>(e);
            if(event->button() == Qt::LeftButton)
            {
                isMousePressed = true;
                mousePressPos = event->globalPos() - this->pos();
                return true;
            }
        }
        else if(e->type() == QEvent::MouseMove)
        {
            QApplication::sendEvent(this, e);
            return true;
        }
    }
    else if(obj==backWidget)
    {
        if(e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease ||
           e->type() == QEvent::MouseMove)
        {
            QApplication::sendEvent(this, e);
            return true;
        }
    }
    return QObject::eventFilter(obj,e);
}

void AppFramelessDialog::mouseMoveEvent(QMouseEvent *e)
{
    if(isMousePressed)
    {
        this->move(e->globalPos() - mousePressPos);
        e->accept();
        return;
    }
    if(!m_bResizeable)
    {
        QDialog::mouseMoveEvent(e);
        return;
    }
    int x = e->x();
    int y = e->y();
    if (resizeMouseDown)
    {
        int dx = x - oldPos.x();
        int dy = y - oldPos.y();

        QRect g = geometry();
        if (left)
            g.setLeft(g.left() + dx);
        if (right)
            g.setRight(g.right() + dx);
        if (top)
            g.setTop(g.top() + dy);
        if (bottom)
            g.setBottom(g.bottom() + dy);
        setGeometry(g);
        oldPos = QPoint(left? oldPos.x() : e->x(), top? oldPos.y() : e->y());
    }
    else
    {
        QRect r = rect();
        left = qAbs(x - r.left()) <= 5;
        right = qAbs(x - r.right()) <= 5;
        top = qAbs(y - r.top()) <= 5;
        bottom = qAbs(y - r.bottom()) <= 5;
        bool hor = left | right, ver = top | bottom;
        qDebug() << "l,r,t,b: " << left << right << top << bottom;

        if(hor && bottom)
        {
            setCursor(left? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor);
        }
        else if(hor && top)
        {
            setCursor(left? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
        }
        else if(hor)
        {
            setCursor(Qt::SizeHorCursor);
        }
        else if(ver)
        {
            setCursor(Qt::SizeVerCursor);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void AppFramelessDialog::mousePressEvent(QMouseEvent *e)
{
    oldPos = e->pos();
    resizeMouseDown = e->button() == Qt::LeftButton;
    grabMouse();
    QDialog::mousePressEvent(e);
}

void AppFramelessDialog::mouseReleaseEvent(QMouseEvent *e)
{
    isMousePressed=false;
    resizeMouseDown = false;
    releaseMouse();
    QDialog::mouseReleaseEvent(e);
}

void AppFramelessDialog::resizeEvent(QResizeEvent *)
{
    // backWidget->setGeometry(8*logicalDpiX()/96,0,width()-16*logicalDpiX()/96,height()-8*logicalDpiY()/96);
    backWidget->setGeometry(0,0,width(),height());
    dialogTip->move((width()-dialogTip->width())/2,dialogTip->y());
}
void AppFramelessDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape && isBusy)
        return;
    QDialog::keyPressEvent(event);
}
void AppFramelessDialog::showBusyState(bool busy)
{
    isBusy = busy;
    if(busy)
    {
        busyLabel->show();
        hideButton->setEnabled(false);
        closeButton->setEnabled(false);

    }
    else
    {
        busyLabel->hide();
        hideButton->setEnabled(true);
        closeButton->setEnabled(true);
    }
}
void AppFramelessDialog::setTitle(const QString &text)
{
    title->setText(text);
}
void AppFramelessDialog::showMessage(const QString &msg, int type)
{
    dialogTip->showMessage(msg, type);
    dialogTip->raise();
}

void AppFramelessDialog::setCloseCallback(const std::function<bool ()> &func)
{
    onCloseCallback = func;
}

void AppFramelessDialog::setHideCallback(const std::function<bool ()> &func)
{
    onHideCallback = func;
}

void AppFramelessDialog::setPin(bool pin)
{
    if (!pin)
    {
        this->setWindowFlag(Qt::WindowStaysOnTopHint, false);
        pinButton->setText(QChar(0xe864));
    }
    else
    {
        this->setWindowFlag(Qt::WindowStaysOnTopHint, true);
        pinButton->setText(QChar(0xe863));
    }
    show();
}

QRect AppFramelessDialog::contentsRect() const
{
    QRect rect = QDialog::contentsRect();
    int width = rect.width();
    int height = rect.height();
    rect.setLeft(rect.left() - m_frames.left());
    rect.setTop(rect.top() - m_frames.top());
    rect.setWidth(width);
    rect.setHeight(height);
    return rect;
}
#endif



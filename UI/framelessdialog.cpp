#include "framelessdialog.h"
#include <QApplication>
#include <QPoint>
#include <QSize>
#include "widgets/dialogtip.h"
#include "widgets/loadingicon.h"
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
#include "Play/Video/mpvplayer.h"
#pragma comment (lib,"Dwmapi.lib") // Adds missing library, fixes error LNK2019: unresolved external symbol __imp__DwmExtendFrameIntoClientArea
#pragma comment (lib,"user32.lib")

CFramelessDialog::CFramelessDialog(const QString &titleStr, QWidget *parent, bool showAccept, bool showClose, bool autoPauseVideo)
    : QDialog(parent), m_borderWidth(5), m_bJustMaximized(false), m_bResizeable(true),
      inited(false), restorePlayState(false), isBusy(false)
{

    setWindowFlags((windowFlags() | Qt::Dialog | Qt::FramelessWindowHint) & ~Qt::WindowSystemMenuHint);
    setObjectName(QStringLiteral("framelessDialog"));
    backWidget = new QWidget(this);
    titleBar = new QWidget(backWidget);

    title=new QLabel(titleStr, titleBar);
    title->setFont(QFont(GlobalObjects::normalFont,10));
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
    QObject::connect(closeButton,&QPushButton::clicked,this,&CFramelessDialog::onClose);
    closeButton->setVisible(showClose);
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    acceptButton=new QPushButton(titleBar);
    acceptButton->setObjectName(QStringLiteral("DialogAcceptButton"));
    acceptButton->setFixedSize(btnSize);
    acceptButton->setFont(*GlobalObjects::iconfont);
    acceptButton->setText(QChar(0xe680));
    QObject::connect(acceptButton,&QPushButton::clicked,this,&CFramelessDialog::onAccept);
    acceptButton->setVisible(showAccept);
    acceptButton->setDefault(false);
    acceptButton->setAutoDefault(false);

    busyLabel = new LoadingIcon(QColor(153, 153, 153), this);
    busyLabel->setFixedSize(QSize(btnH*0.9, btnH*0.9));
    busyLabel->hide();

    QHBoxLayout *titleHBLayout=new QHBoxLayout(titleBar);
    titleHBLayout->setContentsMargins(8*logicalDpiX()/96, 8*logicalDpiY()/96, 8*logicalDpiX()/96, 8*logicalDpiY()/96);
    titleHBLayout->addWidget(title, 0, Qt::AlignVCenter);
    titleHBLayout->addWidget(busyLabel);
    titleHBLayout->addSpacing(btnH * 0.2);
    titleHBLayout->addWidget(acceptButton);
    titleHBLayout->addSpacing(btnH * 0.1);
    titleHBLayout->addWidget(closeButton);

    QVBoxLayout *vbLayout = new QVBoxLayout(backWidget);
    vbLayout->addWidget(titleBar);
    vbLayout->addStretch(1);
    vbLayout->setContentsMargins(0, 0, 0, 0);
    addIgnoreWidget(titleBar);
    addIgnoreWidget(title);

    if (autoPauseVideo && GlobalObjects::mpvplayer->needPause())
	{
		restorePlayState = true;
		GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
	}

    dialogTip = new DialogTip(this);
    dialogTip->hide();
}

void CFramelessDialog::setResizeable(bool resizeable)
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

void CFramelessDialog::setResizeableAreaWidth(int width)
{
    if (1 > width) width = 1;
    m_borderWidth = width;
}

void CFramelessDialog::onAccept()
{
    accept();
    if(restorePlayState)
        GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    if(!sizeSettingKey.isEmpty())
        GlobalObjects::appSetting->setValue(sizeSettingKey,size());
    for(auto &func : onCloseCallback)
        func();
}

void CFramelessDialog::onClose()
{
    reject();
    if(restorePlayState)
        GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    if(!sizeSettingKey.isEmpty())
        GlobalObjects::appSetting->setValue(sizeSettingKey,size());
    for(auto &func : onCloseCallback)
        func();
}

void CFramelessDialog::addIgnoreWidget(QWidget* widget)
{
    if (!widget) return;
    if (m_whiteList.contains(widget)) return;
    m_whiteList.append(widget);
}

bool CFramelessDialog::nativeEvent(const QByteArray &eventType, void *message, long *result)
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

void CFramelessDialog::setContentsMargins(const QMargins &margins)
{
    QDialog::setContentsMargins(margins+m_frames);
    m_margins = margins;
}
void CFramelessDialog::setContentsMargins(int left, int top, int right, int bottom)
{
    QDialog::setContentsMargins(left+m_frames.left(), top+m_frames.top(), right+m_frames.right(), bottom+m_frames.bottom());
    m_margins.setLeft(left);
    m_margins.setTop(top);
    m_margins.setRight(right);
    m_margins.setBottom(bottom);
}
QMargins CFramelessDialog::contentsMargins() const
{
    QMargins margins = QDialog::contentsMargins();
    margins -= m_frames;
    return margins;
}
void CFramelessDialog::getContentsMargins(int *left, int *top, int *right, int *bottom) const
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

void CFramelessDialog::showEvent(QShowEvent *)
{
    if(!inited)
    {
        setResizeable(m_bResizeable);
        inited=true;
    }
    setContentsMargins(8*logicalDpiX()/96,titleBar->height(),8*logicalDpiX()/96,8*logicalDpiY()/96);
    resize(width(),height());
}

void CFramelessDialog::resizeEvent(QResizeEvent *)
{
    backWidget->setGeometry(0,0,width(),height());
    dialogTip->move((width()-dialogTip->width())/2,dialogTip->y());
}

void CFramelessDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape && isBusy)
        return;
    QDialog::keyPressEvent(event);
}


void CFramelessDialog::showBusyState(bool busy)
{
    isBusy = busy;
    if(busy)
    {
        busyLabel->show();
        acceptButton->setEnabled(false);
        closeButton->setEnabled(false);
    }
    else
    {
        busyLabel->hide();
        acceptButton->setEnabled(true);
        closeButton->setEnabled(true);
    }
}

void CFramelessDialog::setTitle(const QString &text)
{
    title->setText(text);
}

void CFramelessDialog::showMessage(const QString &msg, int type)
{
    dialogTip->showMessage(msg, type);
}

void CFramelessDialog::setSizeSettingKey(const QString &key, const QSize &initSize)
{
    sizeSettingKey = key;
    resize(GlobalObjects::appSetting->value(sizeSettingKey, initSize).toSize());
}

void CFramelessDialog::addOnCloseCallback(const std::function<void ()> &func)
{
    onCloseCallback.append(func);
}

void CFramelessDialog::reject()
{
    QDialog::reject();
    if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
}

QRect CFramelessDialog::contentsRect() const
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
CFramelessDialog::CFramelessDialog(QString titleStr, QWidget *parent, bool showAccept, bool showClose, bool autoPauseVideo)
    : QDialog(parent), m_borderWidth(5), m_bJustMaximized(false), m_bResizeable(true),
      restorePlayState(false),isBusy(false), isMousePressed(false),resizeMouseDown(false)
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
    QObject::connect(closeButton,&QPushButton::clicked,this,&CFramelessDialog::onClose);
    closeButton->setVisible(showClose);
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    acceptButton=new QPushButton(titleBar);
    acceptButton->setObjectName(QStringLiteral("DialogAcceptButton"));
    acceptButton->setFixedSize(btnSize);
    acceptButton->setFont(*GlobalObjects::iconfont);
    acceptButton->setText(QChar(0xe680));
    QObject::connect(acceptButton,&QPushButton::clicked,this,&CFramelessDialog::onAccept);
    acceptButton->setVisible(showAccept);
    acceptButton->setDefault(false);
    acceptButton->setAutoDefault(false);

    busyLabel = new LoadingIcon(QColor(153, 153, 153), this);
    busyLabel->setFixedSize(btnSize);
    busyLabel->hide();

    QHBoxLayout *titleHBLayout=new QHBoxLayout(titleBar);
    titleHBLayout->setContentsMargins(8*logicalDpiX()/96, 8*logicalDpiY()/96, 8*logicalDpiX()/96, 8*logicalDpiY()/96);
    titleHBLayout->addWidget(title, 0, Qt::AlignVCenter);
    titleHBLayout->addWidget(busyLabel);
    titleHBLayout->addSpacing(btnH * 0.1);
    titleHBLayout->addWidget(acceptButton);
    titleHBLayout->addSpacing(btnH * 0.1);
    titleHBLayout->addWidget(closeButton);

    QVBoxLayout *vbLayout = new QVBoxLayout(backWidget);
    vbLayout->addWidget(titleBar);
    vbLayout->addStretch(1);
    vbLayout->setContentsMargins(0, 0, 0, 0);

    if (autoPauseVideo && GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
    {
        restorePlayState = true;
        GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
    }

    dialogTip = new DialogTip(this);
    dialogTip->hide();
}

void CFramelessDialog::setResizeable(bool resizeable)
{
    m_bResizeable = resizeable;
}

void CFramelessDialog::onAccept()
{
    accept();
    if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    if(!sizeSettingKey.isEmpty())
        GlobalObjects::appSetting->setValue(sizeSettingKey,size());
    for(auto &func : onCloseCallback)
        func();
}

void CFramelessDialog::onClose()
{
    reject();
    if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    if(!sizeSettingKey.isEmpty())
        GlobalObjects::appSetting->setValue(sizeSettingKey,size());
    for(auto &func : onCloseCallback)
        func();

}

void CFramelessDialog::setContentsMargins(const QMargins &margins)
{
    QDialog::setContentsMargins(margins+m_frames);
    m_margins = margins;
}
void CFramelessDialog::setContentsMargins(int left, int top, int right, int bottom)
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
QMargins CFramelessDialog::contentsMargins() const
{
    QMargins margins = QDialog::contentsMargins();
    margins -= m_frames;
    return margins;
}
void CFramelessDialog::getContentsMargins(int *left, int *top, int *right, int *bottom) const
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

void CFramelessDialog::showEvent(QShowEvent *e)
{
    setContentsMargins(8*logicalDpiX()/96,titleBar->height(),8*logicalDpiX()/96,8*logicalDpiY()/96);
    QDialog::showEvent(e);
}

bool CFramelessDialog::eventFilter(QObject *obj, QEvent *e)
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

void CFramelessDialog::mouseMoveEvent(QMouseEvent *e)
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
        // qDebug() << "l,r,t,b: " << left << right << top << bottom;

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

void CFramelessDialog::mousePressEvent(QMouseEvent *e)
{
    oldPos = e->pos();
    resizeMouseDown = e->button() == Qt::LeftButton;
    grabMouse();
    QDialog::mousePressEvent(e);
}

void CFramelessDialog::mouseReleaseEvent(QMouseEvent *e)
{
    isMousePressed=false;
    resizeMouseDown = false;
    releaseMouse();
    QDialog::mouseReleaseEvent(e);
}

void CFramelessDialog::resizeEvent(QResizeEvent *)
{
    // backWidget->setGeometry(8*logicalDpiX()/96,0,width()-16*logicalDpiX()/96,height()-8*logicalDpiY()/96);
     backWidget->setGeometry(0,0,width(),height());
    dialogTip->move((width()-dialogTip->width())/2,dialogTip->y());
}
void CFramelessDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape && isBusy)
        return;
    QDialog::keyPressEvent(event);
}
void CFramelessDialog::showBusyState(bool busy)
{
    isBusy = busy;
    if(busy)
    {
        busyLabel->show();
        acceptButton->setEnabled(false);
        closeButton->setEnabled(false);

    }
    else
    {
        busyLabel->hide();
        acceptButton->setEnabled(true);
        closeButton->setEnabled(true);
    }
}
void CFramelessDialog::setTitle(const QString &text)
{
    title->setText(text);
}
void CFramelessDialog::showMessage(const QString &msg, int type)
{
    dialogTip->showMessage(msg, type);
    dialogTip->raise();
}
void CFramelessDialog::setSizeSettingKey(const QString &key, const QSize &initSize)
{
    sizeSettingKey = key;
    resize(GlobalObjects::appSetting->value(sizeSettingKey, initSize).toSize());
}
void CFramelessDialog::addOnCloseCallback(const std::function<void ()> &func)
{
    onCloseCallback.append(func);
}
void CFramelessDialog::reject()
{
    QDialog::reject();
    if(restorePlayState)GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
}

QRect CFramelessDialog::contentsRect() const
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


#ifndef ELAAPPBAR_H
#define ELAAPPBAR_H

#include <QWidget>

#include "Def.h"
#include "stdafx.h"

#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define Q_TAKEOVER_NATIVEEVENT_H virtual bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#else
#define Q_TAKEOVER_NATIVEEVENT_H virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
#endif
#else
#define Q_TAKEOVER_NATIVEEVENT_H
#endif

#ifdef Q_OS_WIN
#define ELAAPPBAR_HANDLE(ElaAppBar)                                           \
    if (ElaAppBar)                                                            \
    {                                                                         \
        int ret = ElaAppBar->takeOverNativeEvent(eventType, message, result); \
        if (ret == -1)                                                        \
        {                                                                     \
            return QWidget::nativeEvent(eventType, message, result);          \
        }                                                                     \
        return (bool)ret;                                                     \
    }                                                                         \
    return QWidget::nativeEvent(eventType, message, result);
#endif

#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define Q_TAKEOVER_NATIVEEVENT_CPP(CLASS, ElaAppBar)                                     \
    bool CLASS::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) \
    {                                                                                    \
        ELAAPPBAR_HANDLE(ElaAppBar)                                                      \
    }
#else
#define Q_TAKEOVER_NATIVEEVENT_CPP(CLASS, ElaAppBar)                                  \
    bool CLASS::nativeEvent(const QByteArray& eventType, void* message, long* result) \
    {                                                                                 \
        ELAAPPBAR_HANDLE(ElaAppBar)                                                   \
    }
#endif
#else
#define Q_TAKEOVER_NATIVEEVENT_CPP(CLASS, ElaAppBar)
#endif

class ElaAppBarPrivate;
class ElaToolButton;
class ElaIconButton;
class QPushButton;
class LoadingIcon;
class ElaAppBar : public QWidget
{
    Q_OBJECT
    Q_Q_CREATE(ElaAppBar)
    Q_PROPERTY_CREATE_Q_H(bool, IsStayTop)
    Q_PROPERTY_CREATE_Q_H(bool, IsFixedSize)
    Q_PROPERTY_CREATE_Q_H(bool, IsDefaultClosed)
    Q_PROPERTY_CREATE_Q_H(bool, IsOnlyAllowMinAndClose)
    Q_PROPERTY_CREATE_Q_H(int, AppBarHeight)
    Q_PROPERTY_CREATE_Q_H(int, CustomWidgetMaximumWidth)
public:
    explicit ElaAppBar(QWidget* parent = nullptr, ElaAppBarControlType::AppBarControlType controlType = ElaAppBarControlType::Main);
    ~ElaAppBar();

    void setCustomWidget(ElaAppBarType::CustomArea customArea, QWidget* customWidget);
    QWidget* getCustomWidget() const;

    void setWindowControlFlag(ElaAppBarType::WindowControlType buttonFlag, bool isEnable = true);
    void setWindowControlFlags(ElaAppBarType::ButtonFlags buttonFlags);
    ElaAppBarType::ButtonFlags getWindowControlFlags() const;

    void setIcon(const QIcon &icon);
    ElaToolButton *iconButton();
    ElaToolButton *appButton();
    QPushButton *dialogCloseButton();
    QPushButton *dialogAcceptButton();
    QPushButton *dialogHideButton();
    QPushButton *dialogPinButton();
    LoadingIcon *dialogBusyIcon();

    void hideAppBar(bool on, bool isFullScreen = false);
    void setOnTop(bool on);
    void setScreenSave(bool on);
    void closeWindow();
#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    int takeOverNativeEvent(const QByteArray& eventType, void* message, qintptr* result);
#else
    int takeOverNativeEvent(const QByteArray& eventType, void* message, long* result);
#endif
#endif
Q_SIGNALS:
    Q_SIGNAL void closeButtonClicked();
    Q_SIGNAL void acceptButtonClicked();
    Q_SIGNAL void hideButtonClicked();
    Q_SIGNAL void pinButtonClicked();

protected:
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

    void initMainControls();
    void initDialogControls();
    void initAppDialogControls();

};

#endif // ELAAPPBAR_H

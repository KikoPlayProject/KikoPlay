/****************************************************************************
**
** Copyright (C) 2016 Ivan Vizir <define-true-false@yandex.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWinExtras module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINFUNCTIONS_H
#define QWINFUNCTIONS_H

#if 0
#pragma qt_class(QtWin)
#endif

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qwidget.h>
#endif
#include <QObject>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

class QPixmap;
class QImage;
class QBitmap;
class QColor;
class QWindow;
class QString;
class QMargins;

namespace QtWin
{
    enum HBitmapFormat
    {
        HBitmapNoAlpha,
        HBitmapPremultipliedAlpha,
        HBitmapAlpha
    };

    enum WindowFlip3DPolicy
    {
        FlipDefault,
        FlipExcludeBelow,
        FlipExcludeAbove
    };

    enum WindowNonClientRenderingPolicy
    {
        NonClientRenderingUseWindowStyle,
        NonClientRenderingDisabled,
        NonClientRenderingEnabled
    };

    HBITMAP createMask(const QBitmap &bitmap);
     HBITMAP toHBITMAP(const QPixmap &p, HBitmapFormat format = HBitmapNoAlpha);
     QPixmap fromHBITMAP(HBITMAP bitmap, HBitmapFormat format = HBitmapNoAlpha);
     HICON toHICON(const QPixmap &p);
     HBITMAP imageToHBITMAP(const QImage &image, QtWin::HBitmapFormat format = HBitmapNoAlpha);
     QImage imageFromHBITMAP(HDC hdc, HBITMAP bitmap, int width, int height);
     QImage imageFromHBITMAP(HBITMAP bitmap, QtWin::HBitmapFormat format = HBitmapNoAlpha);
     QPixmap fromHICON(HICON icon);
     HRGN toHRGN(const QRegion &region);
     QRegion fromHRGN(HRGN hrgn);

     QString stringFromHresult(HRESULT hresult);
     QString errorStringFromHresult(HRESULT hresult);

     QColor colorizationColor(bool *opaqueBlend = nullptr);
     QColor realColorizationColor();

     void setWindowExcludedFromPeek(QWindow *window, bool exclude);
     bool isWindowExcludedFromPeek(QWindow *window);
     void setWindowDisallowPeek(QWindow *window, bool disallow);
     bool isWindowPeekDisallowed(QWindow *window);
     void setWindowFlip3DPolicy(QWindow *window, WindowFlip3DPolicy policy);
     WindowFlip3DPolicy windowFlip3DPolicy(QWindow *);
     void setWindowNonClientAreaRenderingPolicy(QWindow *window, WindowNonClientRenderingPolicy policy);
     WindowNonClientRenderingPolicy windowNonClientAreaRenderingPolicy(QWindow *);

     void extendFrameIntoClientArea(QWindow *window, int left, int top, int right, int bottom);
     void extendFrameIntoClientArea(QWindow *window, const QMargins &margins);
     void resetExtendedFrame(QWindow *window);

     void enableBlurBehindWindow(QWindow *window, const QRegion &region);
     void enableBlurBehindWindow(QWindow *window);
     void disableBlurBehindWindow(QWindow *window);

     bool isCompositionOpaque();

     void setCurrentProcessExplicitAppUserModelID(const QString &id);

     void markFullscreenWindow(QWindow *, bool fullscreen = true);

     void taskbarActivateTab(QWindow *);
     void taskbarActivateTabAlt(QWindow *);
     void taskbarAddTab(QWindow *);
     void taskbarDeleteTab(QWindow *);

#ifdef QT_WIDGETS_LIB
    inline void setWindowExcludedFromPeek(QWidget *window, bool exclude)
    {
        window->createWinId();
        setWindowExcludedFromPeek(window->windowHandle(), exclude);
    }

    inline bool isWindowExcludedFromPeek(QWidget *window)
    {
        auto handle = window->windowHandle();
        return handle && isWindowExcludedFromPeek(handle);
    }

    inline void setWindowDisallowPeek(QWidget *window, bool disallow)
    {
        window->createWinId();
        setWindowDisallowPeek(window->windowHandle(), disallow);
    }

    inline bool isWindowPeekDisallowed(QWidget *window)
    {
        auto handle = window->windowHandle();
        return handle && isWindowPeekDisallowed(handle);
    }

    inline void setWindowFlip3DPolicy(QWidget *window, WindowFlip3DPolicy policy)
    {
        window->createWinId();
        setWindowFlip3DPolicy(window->windowHandle(), policy);
    }

    inline WindowFlip3DPolicy windowFlip3DPolicy(QWidget *window)
    {
        auto handle = window->windowHandle();
        return handle ? windowFlip3DPolicy(handle) : FlipDefault;
    }

    inline void setWindowNonClientAreaRenderingPolicy(QWidget *window, WindowNonClientRenderingPolicy policy)
    {
        window->createWinId();
        setWindowNonClientAreaRenderingPolicy(window->windowHandle(), policy);
    }

    inline WindowNonClientRenderingPolicy windowNonClientAreaRenderingPolicy(QWidget *window)
    {
        auto handle = window->windowHandle();
        return handle ? windowNonClientAreaRenderingPolicy(handle) : NonClientRenderingUseWindowStyle;
    }

    inline void extendFrameIntoClientArea(QWidget *window, const QMargins &margins)
    {
        window->createWinId();
        extendFrameIntoClientArea(window->windowHandle(), margins);
    }

    inline void extendFrameIntoClientArea(QWidget *window, int left, int top, int right, int bottom)
    {
        window->createWinId();
        extendFrameIntoClientArea(window->windowHandle(), left, top, right, bottom);
    }

    inline void resetExtendedFrame(QWidget *window)
    {
        if (window->windowHandle())
            resetExtendedFrame(window->windowHandle());
    }

    inline void enableBlurBehindWindow(QWidget *window, const QRegion &region)
    {
        window->createWinId();
        enableBlurBehindWindow(window->windowHandle(), region);
    }

    inline void enableBlurBehindWindow(QWidget *window)
    {
        window->createWinId();
        enableBlurBehindWindow(window->windowHandle());
    }

    inline void disableBlurBehindWindow(QWidget *window)
    {
        if (window->windowHandle())
            disableBlurBehindWindow(window->windowHandle());
    }

    inline void markFullscreenWindow(QWidget *window, bool fullscreen = true)
    {
        window->createWinId();
        markFullscreenWindow(window->windowHandle(), fullscreen);
    }

    inline void taskbarActivateTab(QWidget *window)
    {
        window->createWinId();
        taskbarActivateTab(window->windowHandle());
    }

    inline void taskbarActivateTabAlt(QWidget *window)
    {
        window->createWinId();
        taskbarActivateTabAlt(window->windowHandle());
    }

    inline void taskbarAddTab(QWidget *window)
    {
        window->createWinId();
        taskbarAddTab(window->windowHandle());
    }

    inline void taskbarDeleteTab(QWidget *window)
    {
        window->createWinId();
        taskbarDeleteTab(window->windowHandle());
    }
#endif // QT_WIDGETS_LIB
}

QT_END_NAMESPACE

#endif // QWINFUNCTIONS_H

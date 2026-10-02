/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "elidedlabel.h"

#include <QPainter>
#include <QSizePolicy>
#include <QTextLayout>

ElidedLabel::ElidedLabel(QWidget *parent) : ElidedLabel("", parent)
{

}

ElidedLabel::ElidedLabel(const QString &text, QWidget *parent)
    : QFrame(parent), elided(false), content(text)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}


void ElidedLabel::setText(const QString &newText)
{
    content = newText;
    update();
}

void ElidedLabel::setFontColor(const QColor &color)
{
    _fontColor = color;
    update();
}

QSize ElidedLabel::minimumSizeHint() const
{
    const QFontMetrics fm(fontMetrics());
    return QSize(0, qMax(fm.height(), fm.lineSpacing()));
}


void ElidedLabel::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setPen(_fontColor);
    QFontMetrics fontMetrics = painter.fontMetrics();

    bool didElide = false;
    const int lineSpacing = qMax(1, fontMetrics.lineSpacing());
    const int maxLines = qMax(1, 1 + (height() - fontMetrics.height()) / lineSpacing);

    QTextLayout textLayout(content, painter.font());
    textLayout.beginLayout();
    for (int i = 0; i < maxLines; ++i)
    {
        QTextLine line = textLayout.createLine();

        if (!line.isValid())
            break;

        line.setLineWidth(width());
    }
    textLayout.endLayout();

    const int lineCount = textLayout.lineCount();
    if (lineCount > 0)
    {
        const QTextLine lastLine = textLayout.lineAt(lineCount - 1);
        const QString remainingText = content.mid(lastLine.textStart());
        const QString elidedLastLine = fontMetrics.elidedText(remainingText, Qt::ElideRight, width());
        didElide = lastLine.textStart() + lastLine.textLength() < content.size()
                   || elidedLastLine != remainingText;

        // Center the lines actually displayed, including the last line's descent.
        const int textHeight = fontMetrics.height() + (lineCount - 1) * lineSpacing;
        int y = qMax(0, (height() - textHeight) / 2);
        for (int i = 0; i < lineCount; ++i)
        {
            if (i == lineCount - 1 && didElide)
                painter.drawText(QPoint(0, y + fontMetrics.ascent()), elidedLastLine);
            else
                textLayout.lineAt(i).draw(&painter, QPoint(0, y));
            y += lineSpacing;
        }
    }

    if (didElide != elided)
    {
        elided = didElide;
        emit elisionChanged(didElide);
    }
}

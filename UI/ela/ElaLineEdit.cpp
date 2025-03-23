#include "ElaLineEdit.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>

#include "ElaEventBus.h"
#include "DeveloperComponents/ElaLineEditStyle.h"
#include "ElaMenu.h"
#include "ElaTheme.h"
#include "private/ElaLineEditPrivate.h"
Q_PROPERTY_CREATE_Q_CPP(ElaLineEdit, int, BorderRadius)

ElaLineEdit::ElaLineEdit(QWidget* parent)
    : QLineEdit(parent), d_ptr(new ElaLineEditPrivate())
{
    Q_D(ElaLineEdit);
    d->q_ptr = this;
    setObjectName("ElaLineEdit");
    d->_themeMode = eTheme->getThemeMode();
    d->_pBorderRadius = 6;
    d->_pExpandMarkWidth = 0;
    d->_pIsClearButtonEnable = true;
    setFocusPolicy(Qt::StrongFocus);

    d->_focusEvent = new ElaEvent("WMWindowClicked", "onWMWindowClickedEvent", d);
    d->_focusEvent->registerAndInit();
    setMouseTracking(true);
    QFont textFont = font();
    textFont.setLetterSpacing(QFont::AbsoluteSpacing, d->_textSpacing);
    setFont(textFont);
    setStyle(new ElaLineEditStyle(style()));

    QMargins textMargins = this->textMargins();
    textMargins.setLeft(10);
    textMargins.setRight(4);
    setTextMargins(textMargins);
    d->onThemeChanged(eTheme->getThemeMode());
    connect(eTheme, &ElaTheme::themeModeChanged, d, &ElaLineEditPrivate::onThemeChanged);
    setFixedHeight(34);

    QPalette palette = this->palette();
    palette.setColor(QPalette::PlaceholderText, ElaThemeColor(d->_themeMode, BasicTextNoFocus));
    setPalette(palette);
}

ElaLineEdit::ElaLineEdit(const QString &text, QWidget *parent) : ElaLineEdit(parent)
{
    setText(text);
}

ElaLineEdit::~ElaLineEdit()
{
}

void ElaLineEdit::setIsClearButtonEnable(bool isClearButtonEnable)
{
    Q_D(ElaLineEdit);
    d->_pIsClearButtonEnable = isClearButtonEnable;
    setClearButtonEnabled(isClearButtonEnable);
    Q_EMIT pIsClearButtonEnableChanged();
}

bool ElaLineEdit::getIsClearButtonEnable() const
{
    Q_D(const ElaLineEdit);
    return d->_pIsClearButtonEnable;
}

void ElaLineEdit::focusInEvent(QFocusEvent* event)
{
    Q_D(ElaLineEdit);
    Q_EMIT focusIn(this->text());
    if (event->reason() == Qt::MouseFocusReason)
    {
        if (d->_pIsClearButtonEnable)
        {
            setClearButtonEnabled(true);
        }
        QPropertyAnimation* markAnimation = new QPropertyAnimation(d, "pExpandMarkWidth");
        connect(markAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
            update();
        });
        markAnimation->setDuration(300);
        markAnimation->setEasingCurve(QEasingCurve::InOutSine);
        markAnimation->setStartValue(d->_pExpandMarkWidth);
        markAnimation->setEndValue(width() / 2 - d->_pBorderRadius / 2 - 4);
        markAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    }
    QLineEdit::focusInEvent(event);
}

void ElaLineEdit::focusOutEvent(QFocusEvent* event)
{
    Q_D(ElaLineEdit);
    Q_EMIT focusOut(this->text());
    if (event->reason() != Qt::PopupFocusReason)
    {
        if (d->_pIsClearButtonEnable)
        {
            setClearButtonEnabled(false);
        }
        QPropertyAnimation* markAnimation = new QPropertyAnimation(d, "pExpandMarkWidth");
        connect(markAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
            update();
        });
        markAnimation->setDuration(300);
        markAnimation->setEasingCurve(QEasingCurve::InOutSine);
        markAnimation->setStartValue(d->_pExpandMarkWidth);
        markAnimation->setEndValue(0);
        markAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        Q_EMIT wmFocusOut(text());
    }
    QLineEdit::focusOutEvent(event);
}

void ElaLineEdit::paintEvent(QPaintEvent* event)
{
    Q_D(ElaLineEdit);
    QLineEdit::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(ElaThemeColor(d->_themeMode, PrimaryNormal));
    painter.drawRoundedRect(QRectF(width() / 2 - d->_pExpandMarkWidth, height() - 2.5, d->_pExpandMarkWidth * 2, 2.5), 2, 2);
}

void ElaLineEdit::contextMenuEvent(QContextMenuEvent* event)
{
    ElaMenu* menu = new ElaMenu(this);
    menu->setMenuItemHeight(27);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction* action = nullptr;
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::ArrowRotateLeft, tr("Undo"), QKeySequence::Undo);
        action->setEnabled(isUndoAvailable());
        connect(action, &QAction::triggered, this, &ElaLineEdit::undo);

        action = menu->addElaIconAction(ElaIconType::ArrowRotateRight, tr("Redo"), QKeySequence::Redo);
        action->setEnabled(isRedoAvailable());
        connect(action, &QAction::triggered, this, &ElaLineEdit::redo);
        menu->addSeparator();
    }
#ifndef QT_NO_CLIPBOARD
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::KnifeKitchen, tr("Cut"), QKeySequence::Cut);
        action->setEnabled(!isReadOnly() && hasSelectedText() && echoMode() == QLineEdit::Normal);
        connect(action, &QAction::triggered, this, &ElaLineEdit::cut);
    }

    action = menu->addElaIconAction(ElaIconType::Copy, tr("Copy"), QKeySequence::Copy);
    action->setEnabled(hasSelectedText() && echoMode() == QLineEdit::Normal);
    connect(action, &QAction::triggered, this, &ElaLineEdit::copy);

    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::Paste, tr("Paste"), QKeySequence::Paste);
        action->setEnabled(!isReadOnly() && !QGuiApplication::clipboard()->text().isEmpty());
        connect(action, &QAction::triggered, this, &ElaLineEdit::paste);
    }
#endif
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::DeleteLeft, tr("Delete"));
        action->setEnabled(!isReadOnly() && !text().isEmpty() && hasSelectedText());
        connect(action, &QAction::triggered, this, [=](bool checked) {
            if (hasSelectedText())
            {
                int startIndex = selectionStart();
                int endIndex = selectionEnd();
                setText(text().remove(startIndex, endIndex - startIndex));
            }
        });
    }
    if (!menu->isEmpty())
    {
        menu->addSeparator();
    }
    action = menu->addAction(tr("Select All"));
    action->setShortcut(QKeySequence::SelectAll);
    action->setEnabled(!text().isEmpty() && !(selectedText() == text()));
    connect(action, &QAction::triggered, this, &ElaLineEdit::selectAll);
    menu->popup(event->globalPos());
}

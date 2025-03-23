#include "klineedit.h"
#include "UI/ela/ElaMenu.h"
#include "qclipboard.h"
#include "qevent.h"
#include "qguiapplication.h"

KLineEdit::KLineEdit(QWidget *parent) : KLineEdit("", parent)
{

}

KLineEdit::KLineEdit(const QString &contents, QWidget *parent) : QLineEdit(contents, parent)
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::PlaceholderText, QColor(150, 150, 150));
    setPalette(palette);
}

void KLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    ElaMenu* menu = new ElaMenu(this);
    menu->setMenuItemHeight(27);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction* action = nullptr;
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::ArrowRotateLeft, tr("Undo"), QKeySequence::Undo);
        action->setEnabled(isUndoAvailable());
        connect(action, &QAction::triggered, this, &KLineEdit::undo);

        action = menu->addElaIconAction(ElaIconType::ArrowRotateRight, tr("Redo"), QKeySequence::Redo);
        action->setEnabled(isRedoAvailable());
        connect(action, &QAction::triggered, this, &KLineEdit::redo);
        menu->addSeparator();
    }
#ifndef QT_NO_CLIPBOARD
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::KnifeKitchen, tr("Cut"), QKeySequence::Cut);
        action->setEnabled(!isReadOnly() && hasSelectedText() && echoMode() == QLineEdit::Normal);
        connect(action, &QAction::triggered, this, &KLineEdit::cut);
    }

    action = menu->addElaIconAction(ElaIconType::Copy, tr("Copy"), QKeySequence::Copy);
    action->setEnabled(hasSelectedText() && echoMode() == QLineEdit::Normal);
    connect(action, &QAction::triggered, this, &KLineEdit::copy);

    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::Paste, tr("Paste"), QKeySequence::Paste);
        action->setEnabled(!isReadOnly() && !QGuiApplication::clipboard()->text().isEmpty());
        connect(action, &QAction::triggered, this, &KLineEdit::paste);
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
    connect(action, &QAction::triggered, this, &KLineEdit::selectAll);
    menu->popup(event->globalPos());
}

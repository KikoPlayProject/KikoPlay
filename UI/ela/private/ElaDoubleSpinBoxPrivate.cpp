#include "ElaDoubleSpinBoxPrivate.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QLineEdit>

#include "../ElaDoubleSpinBox.h"
#include "../ElaMenu.h"
ElaDoubleSpinBoxPrivate::ElaDoubleSpinBoxPrivate(QObject* parent)
    : QObject{parent}
{
}

ElaDoubleSpinBoxPrivate::~ElaDoubleSpinBoxPrivate()
{
}

ElaMenu* ElaDoubleSpinBoxPrivate::_createStandardContextMenu()
{
    Q_Q(ElaDoubleSpinBox);
    QLineEdit* lineEdit = q->lineEdit();
    ElaMenu* menu = new ElaMenu(q);
    menu->setMenuItemHeight(27);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction* action = nullptr;
    if (!lineEdit->isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::ArrowRotateLeft, tr("Undo"), QKeySequence::Undo);
        action->setEnabled(lineEdit->isUndoAvailable());
        connect(action, &QAction::triggered, lineEdit, &QLineEdit::undo);

        action = menu->addElaIconAction(ElaIconType::ArrowRotateRight, tr("Redo"), QKeySequence::Redo);
        action->setEnabled(lineEdit->isRedoAvailable());
        connect(action, &QAction::triggered, lineEdit, &QLineEdit::redo);
        menu->addSeparator();
    }
#ifndef QT_NO_CLIPBOARD
    if (!lineEdit->isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::KnifeKitchen, tr("Cut"), QKeySequence::Cut);
        action->setEnabled(!lineEdit->isReadOnly() && lineEdit->hasSelectedText() && lineEdit->echoMode() == QLineEdit::Normal);
        connect(action, &QAction::triggered, lineEdit, &QLineEdit::cut);
    }

    action = menu->addElaIconAction(ElaIconType::Copy, tr("Copy"), QKeySequence::Copy);
    action->setEnabled(lineEdit->hasSelectedText() && lineEdit->echoMode() == QLineEdit::Normal);
    connect(action, &QAction::triggered, lineEdit, &QLineEdit::copy);

    if (!lineEdit->isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::Paste, tr("Paste"), QKeySequence::Paste);
        action->setEnabled(!lineEdit->isReadOnly() && !QGuiApplication::clipboard()->text().isEmpty());
        connect(action, &QAction::triggered, lineEdit, &QLineEdit::paste);
    }
#endif
    if (!lineEdit->isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::DeleteLeft, tr("Delete"));
        action->setEnabled(!lineEdit->isReadOnly() && !lineEdit->text().isEmpty() && lineEdit->hasSelectedText());
        connect(action, &QAction::triggered, this, [=](bool checked) {
            if (lineEdit->hasSelectedText())
            {
                int startIndex = lineEdit->selectionStart();
                int endIndex = lineEdit->selectionEnd();
                lineEdit->setText(lineEdit->text().remove(startIndex, endIndex - startIndex));
            }
        });
    }
    if (!menu->isEmpty())
    {
        menu->addSeparator();
    }
    action = menu->addAction(tr("Select All"));
    action->setShortcut(QKeySequence::SelectAll);
    action->setEnabled(!lineEdit->text().isEmpty() && !(lineEdit->selectedText() == lineEdit->text()));
    connect(action, &QAction::triggered, q, &ElaDoubleSpinBox::selectAll);
    return menu;
}

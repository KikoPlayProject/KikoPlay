#include "kplaintextedit.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/fonticonbutton.h"
#include "UI/widgets/klineedit.h"
#include "qclipboard.h"
#include "qguiapplication.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QShortcut>

SearchWidget::SearchWidget(QPlainTextEdit* editor) : QWidget(editor), editor(editor)
{
    setAttribute(Qt::WA_StyledBackground, true);

    searchLine = new KLineEdit(this);
    searchLine->setClearButtonEnabled(true);

    prevButton = new FontIconButton(QChar(0xec1b), "", 10, 10, 0, this);
    prevButton->setObjectName(QStringLiteral("FontIconToolButton"));
    nextButton = new FontIconButton(QChar(0xe641), "", 10, 10, 0, this);
    nextButton->setObjectName(QStringLiteral("FontIconToolButton"));
    closeButton = new FontIconButton(QChar(0xe60b), "", 10, 10, 0, this);
    closeButton->setObjectName(QStringLiteral("FontIconCloseToolButton"));
    caseCheck = new FontIconButton(QChar(0xe642), "", 11, 10, 0, this);
    caseCheck->setCheckable(true);
    caseCheck->setObjectName(QStringLiteral("FontIconToolButton"));

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 6, 6);
    setMinimumWidth(120);

    layout->addWidget(searchLine);
    layout->addWidget(prevButton);
    layout->addWidget(nextButton);
    layout->addWidget(caseCheck);
    layout->addWidget(closeButton);

    connect(searchLine, &QLineEdit::textChanged, this, &SearchWidget::search);
    connect(searchLine, &QLineEdit::returnPressed, this, &SearchWidget::findNext);
    connect(prevButton, &QPushButton::clicked, this, &SearchWidget::findPrevious);
    connect(nextButton, &QPushButton::clicked, this, &SearchWidget::findNext);
    connect(closeButton, &QPushButton::clicked, this, &SearchWidget::hideSearch);

    hide();
}

void SearchWidget::show() 
{
    QWidget::show();
    searchLine->setFocus();
    adjustPosition();
}

void SearchWidget::adjustPosition()
{
    if (!editor) return;
    move(editor->rect().right() - width() - 10, 10);
}

void SearchWidget::search()
{
    highlightAllMatches();
    findNext();
}

void SearchWidget::findNext()
{
    findText(QTextDocument::FindFlags());
}

void SearchWidget::findPrevious()
{
    findText(QTextDocument::FindBackward);
}

void SearchWidget::findText(QTextDocument::FindFlags flags)
{
    if (!editor) return;

    QString searchString = searchLine->text();
    if (searchString.isEmpty()) return;

    if (caseCheck->isChecked())
    {
        flags |= QTextDocument::FindCaseSensitively;
    }

    bool found = editor->find(searchString, flags);

    if (!found)
    {
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(flags & QTextDocument::FindBackward ?
            QTextCursor::End : QTextCursor::Start);
        editor->setTextCursor(cursor);
        editor->find(searchString, flags);
    }
}

void SearchWidget::highlightAllMatches()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextCursor cursor(editor->document());
    QString searchString = searchLine->text();

    QColor color = QColor(255, 147, 8);

    while (!cursor.isNull() && !cursor.atEnd())
    {
        cursor = editor->document()->find(searchString, cursor,
            caseCheck->isChecked() ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags());

        if (!cursor.isNull())
        {
            QTextEdit::ExtraSelection extra;
            extra.format.setBackground(color);
            extra.cursor = cursor;
            extraSelections.append(extra);
        }
    }

    editor->setExtraSelections(extraSelections);
}

KPlainTextEdit::KPlainTextEdit(QWidget* parent, bool enableSearch) : QPlainTextEdit(parent)
{
    if (enableSearch)
    {
        searchWidget = new SearchWidget(this);
        QObject::connect(searchWidget, &SearchWidget::hideSearch, this, &KPlainTextEdit::hideSearch);
    }
}

void KPlainTextEdit::toggleSearch()
{
    if (searchWidget)
    {
        searchWidget->isVisible() ? hideSearch() : searchWidget->show();
    }
}

void KPlainTextEdit::hideSearch()
{
    if (searchWidget)
    {
        searchWidget->hide();
        setExtraSelections({});
        this->setFocus();
    }
}

void KPlainTextEdit::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    if (searchWidget) searchWidget->adjustPosition();
}

void KPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    ElaMenu* menu = new ElaMenu(this);
    menu->setMenuItemHeight(27);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction* action = nullptr;
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::ArrowRotateLeft, tr("Undo"), QKeySequence::Undo);
        action->setEnabled(isUndoRedoEnabled() ? document()->isUndoAvailable() : false);
        connect(action, &QAction::triggered, this, &KPlainTextEdit::undo);

        action = menu->addElaIconAction(ElaIconType::ArrowRotateRight, tr("Redo"), QKeySequence::Redo);
        action->setEnabled(isUndoRedoEnabled() ? document()->isRedoAvailable() : false);
        connect(action, &QAction::triggered, this, &KPlainTextEdit::redo);
        menu->addSeparator();
    }
#ifndef QT_NO_CLIPBOARD
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::KnifeKitchen, tr("Cut"), QKeySequence::Cut);
        action->setEnabled(!isReadOnly() && !textCursor().selectedText().isEmpty());
        connect(action, &QAction::triggered, this, &KPlainTextEdit::cut);
    }

    action = menu->addElaIconAction(ElaIconType::Copy, tr("Copy"), QKeySequence::Copy);
    action->setEnabled(!textCursor().selectedText().isEmpty());
    connect(action, &QAction::triggered, this, &KPlainTextEdit::copy);

    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::Paste, tr("Paste"), QKeySequence::Paste);
        action->setEnabled(!isReadOnly() && !QGuiApplication::clipboard()->text().isEmpty());
        connect(action, &QAction::triggered, this, &KPlainTextEdit::paste);
    }
#endif
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::DeleteLeft, tr("Delete"));
        action->setEnabled(!isReadOnly() && !toPlainText().isEmpty() && !textCursor().selectedText().isEmpty());
        connect(action, &QAction::triggered, this, [=](bool checked) {
            if (!textCursor().selectedText().isEmpty())
            {
                textCursor().deleteChar();
            }
        });
    }
    if (!menu->isEmpty())
    {
        menu->addSeparator();
    }
    action = menu->addAction(tr("Select All"));
    action->setShortcut(QKeySequence::SelectAll);
    action->setEnabled(!toPlainText().isEmpty() && !(textCursor().selectedText() == toPlainText()));
    connect(action, &QAction::triggered, this, &KPlainTextEdit::selectAll);
    menu->popup(event->globalPos());
    this->setFocus();
}

void KPlainTextEdit::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key == Qt::Key_F && event->modifiers() == Qt::ControlModifier)
    {
        toggleSearch();
    }
    else if (key == Qt::Key_Escape)
    {
        hideSearch();
    }
    else
    {
        QPlainTextEdit::keyPressEvent(event);
    }
}



void KTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    ElaMenu* menu = new ElaMenu(this);
    menu->setMenuItemHeight(27);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction* action = nullptr;
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::ArrowRotateLeft, tr("Undo"), QKeySequence::Undo);
        action->setEnabled(isUndoRedoEnabled() ? document()->isUndoAvailable() : false);
        connect(action, &QAction::triggered, this, &KTextEdit::undo);

        action = menu->addElaIconAction(ElaIconType::ArrowRotateRight, tr("Redo"), QKeySequence::Redo);
        action->setEnabled(isUndoRedoEnabled() ? document()->isRedoAvailable() : false);
        connect(action, &QAction::triggered, this, &KTextEdit::redo);
        menu->addSeparator();
    }
#ifndef QT_NO_CLIPBOARD
    if (!isReadOnly())
    {
        action = menu->addElaIconAction(ElaIconType::KnifeKitchen, tr("Cut"), QKeySequence::Cut);
        action->setEnabled(!isReadOnly() && !textCursor().selectedText().isEmpty());
        connect(action, &QAction::triggered, this, &KTextEdit::cut);
    }

    action = menu->addElaIconAction(ElaIconType::Copy, tr("Copy"), QKeySequence::Copy);
                                       action->setEnabled(!textCursor().selectedText().isEmpty());
        connect(action, &QAction::triggered, this, &KTextEdit::copy);

        if (!isReadOnly())
        {
            action = menu->addElaIconAction(ElaIconType::Paste, tr("Paste"), QKeySequence::Paste);
            action->setEnabled(!isReadOnly() && !QGuiApplication::clipboard()->text().isEmpty());
            connect(action, &QAction::triggered, this, &KTextEdit::paste);
        }
#endif
        if (!isReadOnly())
        {
            action = menu->addElaIconAction(ElaIconType::DeleteLeft, tr("Delete"));
            action->setEnabled(!isReadOnly() && !toPlainText().isEmpty() && !textCursor().selectedText().isEmpty());
            connect(action, &QAction::triggered, this, [=](bool checked) {
                if (!textCursor().selectedText().isEmpty())
                {
                    textCursor().deleteChar();
                }
            });
        }
        if (!menu->isEmpty())
        {
            menu->addSeparator();
        }
        action = menu->addAction(tr("Select All"));
        action->setShortcut(QKeySequence::SelectAll);
        action->setEnabled(!toPlainText().isEmpty() && !(textCursor().selectedText() == toPlainText()));
        connect(action, &QAction::triggered, this, &KTextEdit::selectAll);
        menu->popup(event->globalPos());
        this->setFocus();
}

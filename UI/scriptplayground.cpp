#include "scriptplayground.h"
#include "UI/widgets/kpushbutton.h"
#include <QTextBlock>
#include <QFile>
#include <QPainter>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QPushButton>
#include <QSplitter>
#include <QGridLayout>

namespace
{
class LuaHighLighter: public QSyntaxHighlighter
{
public:
    LuaHighLighter(QTextDocument *parent = 0) : QSyntaxHighlighter(parent)
    {
        HighlightingRule rule;

        //class
        classFormat.setForeground(QColor(206, 103, 0));
        rule.pattern = QRegularExpression("\\b[A-Za-z]+:\\b");
        rule.format = classFormat;
        highlightingRules.append(rule);
        rule.pattern = QRegularExpression("\\b[A-Za-z]+\\.\\b");
        rule.format = classFormat;
        highlightingRules.append(rule);

        //string
        quotationFormat.setForeground(QColor(154, 168, 58));
        stringRule.format = quotationFormat;

        //function
        functionFormat.setForeground(QColor(206, 103, 0));
        rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        //keyword
        QStringList keywords = {
            "and", "break", "do", "else", "elseif", "end", "false",
            "for", "function", "if", "in", "local", "nil", "not", "or",
            "repeat", "return", "then", "true", "unitl", "while", "goto"
        };
        keywordFormat.setForeground(QColor(136, 114, 162));
        QStringList keywordPatterns;
        for(int i=0; i<keywords.length(); i++)
        {
            QString pattern = "\\b" + keywords[i] + "\\b";
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        //comment
        singleLineCommentFormat.setForeground(QColor(155, 155, 155));
        singleLineCommentFormat.setFontItalic(true);
        rule.pattern = QRegularExpression("--[^\n]*");
        rule.format = singleLineCommentFormat;
        highlightingRules.append(rule);

        //multiline comment --[[xx]]
        multiLineCommentFormat.setForeground(QColor(155, 155, 155));
        multiLineCommentFormat.setFontItalic(true);
        commentStartExpression = QRegularExpression("--\\[\\[");
        commentEndExpression = QRegularExpression("\\]\\]");
    }

protected:
    void checkStringRule(const QString &text, QChar c)
    {
        int start = 0;
        bool stringStart = false;
        for(int i = 0; i < text.size(); ++i)
        {
            if(text[i] == c)
            {
                if(stringStart)
                {
                    if(i > 0 && text[i-1] != '\\')
                    {
                        setFormat(start, i - start + 1, stringRule.format);
                        stringStart = false;
                    }
                }
                else
                {
                    start = i;
                    stringStart = true;
                }
            }
        }
    }
    void highlightBlock(const QString &text) override
    {
        for(const HighlightingRule &rule : highlightingRules)
        {
            QRegularExpressionMatchIterator i = rule.pattern.globalMatch(text);
            while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                setFormat(match.capturedStart(0), match.capturedLength(0), rule.format);

            }
        }
        checkStringRule(text, '"');
        checkStringRule(text, '\'');

        setCurrentBlockState(0);

        int offset = 0;
        if (previousBlockState() != 1)
        {
            auto match = commentStartExpression.match(text);
            offset = match.hasMatch() ? match.capturedStart(0) : -1;
        }
        while (offset >= 0)
        {
            auto match = commentEndExpression.match(text, offset);
            int endIndex = match.hasMatch() ? match.capturedStart(0) : -1;
            int commentLength;
            if (endIndex == -1)
            {
                setCurrentBlockState(1);
                commentLength = text.length() - offset;
            }
            else
            {
                commentLength = endIndex - offset
                                + match.capturedLength(0);
            }
            setFormat(offset, commentLength, multiLineCommentFormat);
            match = commentStartExpression.match(text, offset + commentLength);
            offset = match.hasMatch() ? match.capturedStart(0) : -1;
        }
    }

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
    HighlightingRule stringRule;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineKey;
    QTextCharFormat singleLineValue;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
};
}

CodeEditor::CodeEditor(QWidget *parent) : KPlainTextEdit(parent)
{
    setObjectName(QStringLiteral("LuaCodeEditor"));
    hoverColor = QColor(Qt::transparent);
    lineNumberColor = QColor(40, 40, 40);
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int space = 20*logicalDpiX()/96 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    KPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    QTextCursor cursor = this->textCursor();
    int key = event->key();
    static QMap<int, QChar> keyPairLeft = {
        {Qt::Key_BraceLeft, '{'},
        {Qt::Key_BracketLeft, '['},
        {Qt::Key_ParenLeft, '('}
    };
    static QMap<int, QChar> keyPairRight = {

        {Qt::Key_BraceRight, '}'},
        {Qt::Key_BracketRight, ']'},
        {Qt::Key_ParenRight, ')'}
    };
    static QMap<QChar, QChar> pairChar = {
        {'{', '}'},
        {'[', ']'},
        {'(', ')'},
        {'\'', '\''},
        {'"','"'}
    };
    if(keyPairLeft.contains(key))
    {
        if(cursor.hasSelection())
        {
            int selStart = cursor.selectionStart(), selEnd = cursor.selectionEnd();
            cursor.beginEditBlock();
            cursor.setPosition(selStart);
            cursor.insertText(keyPairLeft[key]);
            cursor.setPosition(selEnd+1);
            cursor.insertText(pairChar[keyPairLeft[key]]);
            cursor.endEditBlock();
            return;
        }
        else
        {
            cursor.insertText(pairChar[keyPairLeft[key]]);
            moveCursor(QTextCursor::Left);
        }
    }
    else if(keyPairRight.contains(key))
    {
        int pos = cursor.positionInBlock();
        const QString text = cursor.block().text();
        if(pos < text.size() && text[pos] == keyPairRight[key])
        {
            moveCursor(QTextCursor::Right);
            return;
        }
    }
    if (key == Qt::Key_QuoteDbl || key == Qt::Key_Apostrophe)
    {
        QChar s = key == Qt::Key_QuoteDbl? '"' : '\'';
        if(cursor.hasSelection())
        {
            int selStart = cursor.selectionStart(), selEnd = cursor.selectionEnd();
            cursor.beginEditBlock();
            cursor.setPosition(selStart);
            cursor.insertText(s);
            cursor.setPosition(selEnd+1);
            cursor.insertText(s);
            cursor.endEditBlock();
            return;
        }
        else
        {
            int pos = cursor.positionInBlock();
            const QString text = cursor.block().text();
            if(pos < text.size() && text[pos] == s)
            {
                moveCursor(QTextCursor::Right);
                return;
            }
            cursor.insertText(s);
            moveCursor(QTextCursor::Left);
        }
    }

    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Equal)
    {
        QFont font(this->font());
        font.setPointSize(qMin(font.pointSize() + 1, 72));
        setFont(font);
        updateLineNumberAreaWidth(0);
    }

    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Minus)
    {
        QFont font(this->font());
        font.setPointSize(qMax(font.pointSize() - 1, 1));
        setFont(font);
        updateLineNumberAreaWidth(0);
    }

    QSet<int> keySet = {Qt::Key_Return, Qt::Key_Backspace, Qt::Key_Backtab, Qt::Key_Tab};

    if (!keySet.contains(key))
    {
        KPlainTextEdit::keyPressEvent(event);
        return;
    }

    if (key == Qt::Key_Backtab || key == Qt::Key_Tab)
    {
        QTextBlock block = cursor.block();
        QString text = block.text();
        int selStart = cursor.selectionStart(), selEnd = cursor.selectionEnd();
        bool multiLens = selEnd > block.position() + block.length() || selStart < block.position();
        if(cursor.hasSelection() && multiLens)
        {
            bool down = selEnd > block.position() + block.length();
            int lens = 1;
            if(down)
            {
                while(block.isValid() && selEnd > block.position() + block.length())
                {
                    block = block.next();
                    ++lens;
                }
            }
            else
            {
                while(block.isValid() && selStart < block.position())
                {
                    block = block.previous();
                    ++lens;
                }
            }
            cursor.movePosition(QTextCursor::MoveOperation::StartOfBlock);
            cursor.beginEditBlock();
            while(lens > 0)
            {
                int spacePos = 0;
                QString blockText = cursor.block().text();
                while(spacePos < blockText.size() && blockText[spacePos]==' ') ++spacePos;
                if(key == Qt::Key_Tab)
                {
                    cursor.insertText(QString(" ").repeated(4 - spacePos % 4));
                }
                else
                {
                    if(spacePos > 0)
                    {
                        int removeCount = spacePos % 4 == 0 ? 4 : spacePos % 4;
                        while(removeCount--) cursor.deleteChar();
                    }
                }
                cursor.movePosition(down ? QTextCursor::NextBlock : QTextCursor::PreviousBlock);
                --lens;
            }
            cursor.endEditBlock();
        }
        else
        {
            if(key == Qt::Key_Tab)
            {
                cursor.insertText(QString(" ").repeated(4 - cursor.positionInBlock() % 4));
            }
            else
            {
                int pos = cursor.positionInBlock() - 1;
                int spaceCount = 0;
                while(pos >= 0 && text[pos]==' ')
                {
                    --pos; ++spaceCount;
                }
                if(spaceCount > 0)
                {
                    int removeCount = spaceCount % 4 == 0 ? 4 : spaceCount % 4;
                    while(removeCount--) cursor.deletePreviousChar();
                }
            }
        }
    }
    else if (key == Qt::Key_Backspace)
    {
        int pos = cursor.positionInBlock();
        const QString text = cursor.block().text();
        if(pos > 0 && pos < text.size())
        {
            if(pairChar.contains(text[pos-1]) && text[pos]==pairChar[text[pos-1]])
            {
                cursor.deleteChar();
            }
        }
        QPlainTextEdit::keyPressEvent(event);

    }
    else if (key == Qt::Key_Return)
    {
        QTextBlock block = cursor.block();
        QString text = block.text();
        int indentation = 0;
        while (indentation < text.size() && text[indentation]==' ')
        {
            ++indentation;
        }
        static QStringList endsText = {"do", "then", "else", "{"};
        for(const QString &t : endsText)
        {
            if(text.endsWith(t))
            {
                indentation += 4;
                break;
            }
        }
        QPlainTextEdit::keyPressEvent(event);
        cursor.insertText(QString(" ").repeated(indentation));
    }
    else
    {
        QPlainTextEdit::keyPressEvent(event);
    }
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        selection.format.setBackground(hoverColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    int marginX = -10 * logicalDpiX()/96;

    painter.setPen(lineNumberColor);
    painter.setFont(font());
    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(marginX, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

ScriptPlayground::ScriptPlayground(QWidget *parent) :
    CFramelessDialog(tr("Script Playground"), parent, false, true, false), executor(new PlaygroundScript)
{
    KPushButton *run = new KPushButton(tr("Run"), this);
    CodeEditor *editor = new CodeEditor(this);
    editor->setFont(QFont("Consolas", 12));

    LuaHighLighter *highlighter = new LuaHighLighter();
    highlighter->setDocument(editor->document());
    QPlainTextEdit *outputView = new KPlainTextEdit(this);
    outputView->setReadOnly(true);
    QSplitter *splitter=new QSplitter(Qt::Vertical, this);
    splitter->setObjectName(QStringLiteral("NormalSplitter"));
    splitter->addWidget(editor);
    splitter->addWidget(outputView);
    splitter->setStretchFactor(0,4);
    splitter->setStretchFactor(1,1);
    splitter->setCollapsible(0,false);
    splitter->setCollapsible(1,true);

    QGridLayout *playgroundGlayout = new QGridLayout(this);
    playgroundGlayout->addWidget(run, 0, 0);
    playgroundGlayout->addWidget(splitter, 1, 0, 1, 4);
    playgroundGlayout->setRowStretch(1, 1);
    playgroundGlayout->setColumnStretch(3, 1);
    setSizeSettingKey("DialogSize/ScriptPlayground",QSize(700, 400));

    QObject::connect(run, &QPushButton::clicked, this, [=](){
        run->setEnabled(false);
        outputView->clear();
        executor->run(editor->toPlainText());
        run->setEnabled(true);
    });

    executor->setPrintCallback([=](const QString &content){
        QMetaObject::invokeMethod(outputView, [=](){
            outputView->appendPlainText(content);
            QTextCursor cursor = outputView->textCursor();
            cursor.movePosition(QTextCursor::End);
            outputView->setTextCursor(cursor);
        });
    });
    QFile scriptFile(":/res/scriptPlayground");
    scriptFile.open(QFile::ReadOnly);
    if(scriptFile.isOpen())
    {
        editor->setPlainText(scriptFile.readAll());
    }
}

#ifndef SCRIPTPLAYGROUND_H
#define SCRIPTPLAYGROUND_H
#include "framelessdialog.h"
#include <QPlainTextEdit>
class PlaygroundScript;
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
    Q_PROPERTY(QColor hoverColor READ getHoverColor WRITE setHoverColor)
    Q_PROPERTY(QColor lineNumberColor READ getLineNumberColor WRITE setLineNumberColor)

public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    QColor getHoverColor() const {return hoverColor;}
    void setHoverColor(const QColor& color) {hoverColor = color;}
    QColor getLineNumberColor() const {return lineNumberColor;}
    void setLineNumberColor(const QColor& color){lineNumberColor = color;}

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
    QColor hoverColor, lineNumberColor;
    int replace_tabs = 4;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

class ScriptPlayground : public CFramelessDialog
{
    Q_OBJECT
public:
    ScriptPlayground(QWidget *parent = nullptr);
private:
    QScopedPointer<PlaygroundScript> executor;
};

#endif // SCRIPTPLAYGROUND_H

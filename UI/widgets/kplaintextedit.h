#ifndef KPLAINTEXTEDIT_H
#define KPLAINTEXTEDIT_H

#include <QPlainTextEdit>
#include <QTextEdit>
class QLineEdit;
class QPushButton;

class SearchWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SearchWidget(QPlainTextEdit *editor);

    void show();
    void adjustPosition();

signals:
    void hideSearch();

protected:


    void search();

    void findNext();

    void findPrevious();

private:
    void findText(QTextDocument::FindFlags flags);

    void highlightAllMatches();

    QPlainTextEdit *editor;
    QLineEdit *searchLine;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *closeButton;
    QPushButton *caseCheck;
};

class KPlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    KPlainTextEdit(QWidget *parent = nullptr, bool enableSearch = true);

private slots:
    void toggleSearch();

    void hideSearch();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    SearchWidget *searchWidget{nullptr};

};

class KTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    using QTextEdit::QTextEdit;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

};

#endif // KPLAINTEXTEDIT_H

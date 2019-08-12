#ifndef FONTICONTOOLBUTTON_H
#define FONTICONTOOLBUTTON_H
#include <QToolButton>
class QLabel;
struct FontIconToolButtonOptions
{
    QChar iconChar;
    int fontSize;
    int iconSize;
    int leftMargin;
    int normalColor;
    int hoverColor;
    int iconTextSpace;
};
class FontIconToolButton : public QToolButton
{
public:
    explicit FontIconToolButton(const FontIconToolButtonOptions &options, QWidget *parent=nullptr);
    void setCheckable(bool checkable);
    void setText(const QString &text);
protected:
    QLabel *iconLabel,*textLabel;
    QString normalStyleSheet,hoverStyleSheet;
    virtual void setHoverState();
    virtual void setNormalState();

    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *)
    {
        if(!isChecked())
            setNormalState();
    }
    virtual void mousePressEvent(QMouseEvent *e)
    {
        setHoverState();
        QToolButton::mousePressEvent(e);
    }

    // QWidget interface
public:
    virtual QSize sizeHint() const;
};
#endif // FONTICONTOOLBUTTON_H

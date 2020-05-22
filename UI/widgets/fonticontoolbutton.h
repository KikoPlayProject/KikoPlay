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
    int iconTextSpace;
};
class FontIconToolButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(QColor hoverColor READ getHoverColor WRITE setHoverColor)
    Q_PROPERTY(QColor normColor READ getNormColor WRITE setNormColor)
public:
    explicit FontIconToolButton(const FontIconToolButtonOptions &options, QWidget *parent=nullptr);
    void setCheckable(bool checkable);
    void setText(const QString &text);

    QColor getHoverColor() const;
    void setHoverColor(const QColor& color);

    QColor getNormColor() const;
    void setNormColor(const QColor& color);
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
private:
    QColor hoverColor, normColor;
};
#endif // FONTICONTOOLBUTTON_H

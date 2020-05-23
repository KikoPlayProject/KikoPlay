#ifndef FONTICONTOOLBUTTON_H
#define FONTICONTOOLBUTTON_H
#include <QToolButton>
class QLabel;
class FontIconToolButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(QColor hoverColor READ getHoverColor WRITE setHoverColor)
    Q_PROPERTY(QColor normColor READ getNormColor WRITE setNormColor)
public:
    FontIconToolButton(QChar iconChar, const QString &text, int iconSize=12, int fontSize=10,
                       int iconTextSpace=-1, QWidget *parent=nullptr);
    void setCheckable(bool checkable);
    void setText(const QString &text);
    void setAutoHideText(bool on);
    void hideText(bool hide);
    QSize iconSize() const;

    QColor getHoverColor() const;
    void setHoverColor(const QColor& color);
    QColor getNormColor() const;
    void setNormColor(const QColor& color);
signals:
    void iconHidden(bool hide);
protected:
    QLabel *iconLabel,*textLabel;
    QString normalStyleSheet,hoverStyleSheet;
    QColor hoverColor, normColor;
    int preferredWidth;
    bool autoHideText;

    virtual void setHoverState();
    virtual void setNormalState();

    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void resizeEvent(QResizeEvent *event);

    // QWidget interface
public:
    virtual QSize sizeHint() const;    

};
#endif // FONTICONTOOLBUTTON_H

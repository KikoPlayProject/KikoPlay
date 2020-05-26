#ifndef FONTICONBUTTON_H
#define FONTICONBUTTON_H
#include <QPushButton>

class FontIconButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(QColor normColor READ getNormColor WRITE setNormColor)
    Q_PROPERTY(QColor hoverColor READ getHoverColor WRITE setHoverColor)
public:
    FontIconButton(QChar iconChar, const QString &content, int iconSize=12, int fontSize=10,
                       int iconTextSpace=-1, QWidget *parent=nullptr);
    void setText(const QString &text);
    void setAutoHideText(bool on) {autoHideText = on;}
    void hideText(bool on);

    QColor getNormColor() const {return normColor;}
    void setNormColor(const QColor& color);
    QColor getHoverColor() const {return hoverColor;}
    void setHoverColor(const QColor& color);

    // QWidget interface
public:
    virtual QSize sizeHint() const;
signals:
    void textHidden(bool hide);
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void mousePressEvent(QMouseEvent *e);

private:
    QChar icon;
    int iconFontSize, textFontSize;
    int iconSpace;
    QString text;
    bool hideTextOn = false;
    mutable QSize sHint, contentSize;
    QColor fontColor, normColor, hoverColor;
    bool autoHideText = false;
    int preferredWidth;
};

#endif // FONTICONBUTTON_H

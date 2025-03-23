#ifndef FONTICONBUTTON_H
#define FONTICONBUTTON_H
#include <QPushButton>

class FontIconButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(QColor normColor READ getNormColor WRITE setNormColor)
    Q_PROPERTY(QColor hoverColor READ getHoverColor WRITE setHoverColor)
    Q_PROPERTY(QColor selectColor READ getSelectColor WRITE setSelectColor)
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
    QColor getSelectColor() const {return selectColor;}
    void setSelectColor(const QColor& color);

    // QWidget interface
public:
    virtual QSize sizeHint() const;
    void setContentsMargins(int left, int top, int right, int bottom);
    void setTextAlignment(int align);
signals:
    void textHidden(bool hide);
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);


private:
    QChar icon;
    int iconFontSize, textFontSize;
    int iconSpace;
    QString text;
    bool hideTextOn = false;
    mutable QSize sHint;
    QColor fontColor, normColor, hoverColor, selectColor;
    bool autoHideText = false;
    int preferredWidth;
    int alignment{Qt::AlignCenter};
};

#endif // FONTICONBUTTON_H

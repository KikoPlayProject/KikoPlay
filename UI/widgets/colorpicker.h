#ifndef COLORPICKER_H
#define COLORPICKER_H
#include <QPushButton>
#include <QToolButton>
#include <QMenu>

class ColorButton : public QToolButton
{
    Q_OBJECT
public:
    ColorButton(const QColor &color, int w, int h, QWidget *parent = nullptr);
    const QColor color() const {return btnColor;}
    static QIcon createIcon(const QColor &color, int w, int h);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QColor btnColor;
};

class ColorMenu : public QMenu
{
    Q_OBJECT
public:
    ColorMenu(const QList<QColor> &colorList, QWidget *parent = nullptr);
    void addColor(const QColor &color);
    void setColor(const QColor &color);
signals:
    void colorChanged(const QColor &color);
private:
    ColorButton *contains(const QColor &color);
    void resetGrid();
    void clean();
    QButtonGroup *btnGroup;
    QPushButton *moreColor;
    QList<ColorButton *> buttons;
};

class ColorPicker : public QToolButton
{
    Q_OBJECT
public:
    ColorPicker(const QList<QColor> &colorList, QWidget *parent = nullptr);

    void setColor(const QColor &color);
    const QColor color() const {return curColor;}

    void addColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);

private:
    QColor curColor;
    ColorMenu *colorMenu;
};

#endif // COLORPICKER_H

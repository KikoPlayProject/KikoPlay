#ifndef STYLEEDITOR_H
#define STYLEEDITOR_H
#include "framelessdialog.h"
#include <QSlider>
#include <QStyle>
class QListWidget;
class QCheckBox;
class ColorSlider : public QSlider
{
    Q_OBJECT
public:
    explicit ColorSlider(QWidget *parent=nullptr):QSlider(Qt::Horizontal,parent),
        hoverControl(QStyle::SC_None), pressedControl(QStyle::SC_None)
    {
        setObjectName(QStringLiteral("ColorSlider"));
    }
    void setGradient(const QLinearGradient &g) {fillGradient = g; update();}
    void setEnabled(bool on);
signals:
    void sliderClick(int pos);
    void sliderUp(int pos);
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    bool event(QEvent *event) override;
private:
    QLinearGradient fillGradient;
    QStyle::SubControl hoverControl, pressedControl;
    void updateHoverControl(const QPoint &pos);
};

class StyleEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    StyleEditor(QWidget *parent = nullptr);
signals:
    void setBackground(const QString &path, const QColor &color=QColor());
    void setBgDarkness(int val);
    void setThemeColor(const QColor &color);
private:
    void setBgList(QListWidget *bgImgView);
    void updateSetting(const QString &path, bool add = true);
    QPixmap getThumb(const QString &path);
    void setSlide();
    static QHash<QString, QPixmap> bgThumb;
    const int maxBgCount = 6;
    QStringList historyBgs;
    QSize thumbSize;
    ColorSlider *sliderBgDarkness, *sliderHue, *sliderLightness;
    QCheckBox *enableBg, *customColor;
    QWidget *colorPreview;
protected:
    void onClose() override;
};

#endif // STYLEEDITOR_H

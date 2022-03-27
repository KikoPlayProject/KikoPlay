#ifndef STYLEPAGE_H
#define STYLEPAGE_H
#include "settingpage.h"
class ColorSlider;
class QListWidget;
class QCheckBox;
class QComboBox;
class StylePage : public SettingPage
{
    Q_OBJECT
public:
    StylePage(QWidget *parent = nullptr);
    virtual void onAccept() override;
    virtual void onClose() override;
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
    bool darkModeChanged = false, bgChanged = false, bgDarknessChanged = false, colorChanged = false;
    const int maxBgCount = 32;
    QStringList historyBgs;
    QSize thumbSize;
    ColorSlider *sliderBgDarkness, *sliderHue, *sliderLightness;
    QComboBox *hideToTrayCombo;
    QCheckBox *darkMode, *enableBg, *customColor;
    QWidget *colorPreview;
};

#endif // STYLEPAGE_H

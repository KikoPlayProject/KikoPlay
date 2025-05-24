#ifndef GENERALPAGE_H
#define GENERALPAGE_H
#include "settingpage.h"
class ColorSlider;
class QListWidget;
class QCheckBox;
class QComboBox;
class ColorPreview;
class BgImageSelector : public QWidget
{
    Q_OBJECT
public:
    explicit BgImageSelector(QWidget *parent = nullptr);

    void init();

    void addImage();

    bool eventFilter(QObject *watched, QEvent *event);
signals:
    void setBackground(const QString &path);
private:
    const int c_num = 6;
    ColorPreview *emptyBg{nullptr};
    ColorPreview *lastSelectBg{nullptr};
    QVector<ColorPreview *> candidates;
    static QHash<QString, QPixmap> bgThumb;

    QPixmap getThumb(const QString &path);
    void updateSetting();
};

class BgColorPicker : public QWidget
{
    Q_OBJECT
public:
    explicit BgColorPicker(const QColor &color, QWidget *parent = nullptr);
signals:
    void colorChanged(const QColor &color);

    // QWidget interface
public:
    QSize sizeHint() const;
};

class GeneralPage : public SettingPage
{
    Q_OBJECT
public:
    GeneralPage(QWidget *parent = nullptr);

};

#endif // GENERALPAGE_H

#include "generalpage.h"
#include <QListWidget>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QGridLayout>
#include <QSettings>
#include <QAction>
#include <QFileDialog>
#include <QPainter>
#include <QHoverEvent>
#include <QStylePainter>
#include <QComboBox>
#include "../mainwindow.h"
#include "../stylemanager.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaToggleSwitch.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "UI/widgets/colorslider.h"
#include "UI/widgets/colorpreview.h"
#include "Common/threadtask.h"
#include <QPainterPath>
#include <algorithm>


QHash<QString, QPixmap> BgImageSelector::bgThumb;

#define SETTING_KEY_KIKOPLAY_LANG "KikoPlay/Language"


GeneralPage::GeneralPage(QWidget *parent) : SettingPage(parent)
{
    SettingItemArea *generalArea = new SettingItemArea(tr("General"), this);

    ElaComboBox *langCombo = new ElaComboBox(this);
    const QVector<QPair<QString, QString>> langs = {
        {"English", "en-US"},
        {"Chinese Simplified", "zh-CN"},
    };
    int defalutIndex = -1, curIndex = 0;
    for (const auto &p : langs)
    {
        langCombo->addItem(p.first, p.second);
        if (p.second == GlobalObjects::context()->lang)
        {
            defalutIndex = curIndex;
        }
        ++curIndex;
    }
    langCombo->setCurrentIndex(defalutIndex);
    generalArea->addItem(tr("Language(Restart required)"), langCombo);

    ElaComboBox *hideToTrayCombo = new ElaComboBox(this);
    QStringList hideTypes{tr("None"), tr("Minimize"), tr("Close Window")};
    for (int i = 0; i < static_cast<int>(MainWindow::HideToTrayType::UNKNOWN); ++i)
    {
        hideToTrayCombo->addItem(hideTypes[i], i);
    }
    generalArea->addItem(tr("Hide To Tray"), hideToTrayCombo);


    SettingItemArea *appearanceArea = new SettingItemArea(tr("Appearance"), this);
    BgImageSelector *bgSelector = new BgImageSelector(this);
    appearanceArea->addItem(tr("Background Image"), bgSelector);

    ColorSlider *bgOpacitySlider = new ColorSlider(this);
    bgOpacitySlider->setMinimumWidth(240);
    bgOpacitySlider->setRange(0, 240);
    bgOpacitySlider->setValue(static_cast<MainWindow *>(GlobalObjects::mainWindow)->bgDarkness());
    QLinearGradient darknessGradient;
    darknessGradient.setColorAt(0, Qt::white);
    darknessGradient.setColorAt(1, Qt::black);
    bgOpacitySlider->setGradient(darknessGradient);
    appearanceArea->addItem(tr("Background Image Opacity"), bgOpacitySlider);


    ElaToggleSwitch *themeColorSwitch = new ElaToggleSwitch(this);
    themeColorSwitch->setIsToggled(StyleManager::getStyleManager()->enableThemeColor());
    appearanceArea->addItem(tr("Theme Color"), themeColorSwitch);

    BgColorPicker *bgColorPicker = new BgColorPicker(StyleManager::getStyleManager()->curThemeColor(), this);
    appearanceArea->addItem(bgColorPicker);
    bgColorPicker->setVisible(themeColorSwitch->getIsToggled());
    QObject::connect(bgColorPicker, &BgColorPicker::colorChanged, StyleManager::getStyleManager(), &StyleManager::setThemeColor);

    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    itemVLayout->addWidget(generalArea);
    itemVLayout->addWidget(appearanceArea);
    itemVLayout->addStretch(1);

    QTimer::singleShot(0, [=](){
        showBusyState(true);
        bgSelector->init();
        showBusyState(false);
    });

    QObject::connect(langCombo, (void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, this, [=](int index){
        const QString lang = langCombo->currentData().toString();
        if (!lang.isEmpty())
        {
            GlobalObjects::appSetting->setValue(SETTING_KEY_KIKOPLAY_LANG, lang);
        }
    });

    QObject::connect(bgSelector, &BgImageSelector::setBackground, this, [](const QString &path){
        MainWindow *mW = static_cast<MainWindow *>(GlobalObjects::mainWindow);
        mW->setBackground(path, true);
    });

    QObject::connect(themeColorSwitch, &ElaToggleSwitch::toggled, this, [=](bool toggled){
        bgColorPicker->setVisible(toggled);
        StyleManager::getStyleManager()->setThemeColor(toggled ? StyleManager::getStyleManager()->curThemeColor() : QColor());
    });

    QObject::connect(bgOpacitySlider, &QSlider::valueChanged, this, [](int val){
        MainWindow *mW = static_cast<MainWindow *>(GlobalObjects::mainWindow);
        mW->setBgDarkness(val);
    });
}

BgImageSelector::BgImageSelector(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *hLayout = new QHBoxLayout(this);
    for (int i = 0; i < c_num + 1; ++i)
    {
        ColorPreview *bg = new ColorPreview(this);
        if (i == 0) emptyBg = bg;
        else candidates << bg;
        bg->setObjectName(QStringLiteral("BgImageCandidate"));
        hLayout->addWidget(bg);
        bg->setFixedSize(36, 36);
        bg->installEventFilter(this);
        bg->hide();
    }
    emptyBg->setPixmap(QPixmap(":/res/images/empty-bg.svg"));
    KPushButton *browseImageBtn = new KPushButton(tr("Browse"), this);
    hLayout->addStretch(1);
    hLayout->addSpacing(8);
    hLayout->addWidget(browseImageBtn);
    QObject::connect(browseImageBtn, &KPushButton::clicked, this, &BgImageSelector::addImage);
}

void BgImageSelector::init()
{
    ThreadTask t(GlobalObjects::workThread);
    const QStringList historyBgs = GlobalObjects::appSetting->value("MainWindow/HistoryBackgrounds").toStringList();
    t.Run([this, &historyBgs](){
        for (int i = 0; i < c_num && i < historyBgs.size(); ++i)
        {
            const QString &path = historyBgs[i];
            getThumb(path);
        }
        return 0;
    });
    int i = 0;
    for (; i < c_num && i < historyBgs.size(); ++i)
    {
        const QString &path = historyBgs[i];
        candidates[i]->setPixmap(getThumb(path));
        candidates[i]->setProperty("_img_path", path);
        candidates[i]->show();
    }
    emptyBg->setVisible(i > 0);
    for (; i < c_num; ++i)
    {
        candidates[i]->hide();
    }
}

void BgImageSelector::addImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Image"), "",
                                                    "Image Files(*.jpg *.png);;All Files(*)");
    if (fileName.isEmpty()) return;
    const QString curBgPath = GlobalObjects::appSetting->value("MainWindow/Background").toString();
    if (fileName == curBgPath) return;
    QPixmap thumb(getThumb(fileName));
    if (thumb.isNull()) return;
    ColorPreview *lastBgLabel = candidates.last();
    lastBgLabel->setPixmap(getThumb(fileName));
    lastBgLabel->setProperty("_img_path", fileName);
    lastBgLabel->show();

    candidates.move(candidates.size() - 1, 0);
    QHBoxLayout *layout = static_cast<QHBoxLayout *>(this->layout());
    layout->removeWidget(lastBgLabel);
    layout->insertWidget(1, lastBgLabel);
    lastBgLabel->show();
    emptyBg->show();

    updateSetting();
    emit setBackground(fileName);
}

bool BgImageSelector::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        if (watched == emptyBg)
        {
            emit setBackground("");
        }
        else
        {
            for (int i = 0; i < candidates.size(); ++i)
            {
                if (candidates[i] == watched)
                {
                    ColorPreview *selectLabel = candidates[i];
                    const QString path = selectLabel->property("_img_path").toString();
                    if (path.isEmpty()) break;
                    if (path == GlobalObjects::appSetting->value("MainWindow/Background").toString()) break;
                    candidates.move(i, 0);
                    QHBoxLayout *layout = static_cast<QHBoxLayout *>(this->layout());
                    layout->removeWidget(selectLabel);
                    layout->insertWidget(1, selectLabel);
                    updateSetting();
                    emit setBackground(path);
                    break;
                }
            }
        }
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

QPixmap BgImageSelector::getThumb(const QString &path)
{
    if (bgThumb.contains(path)) return bgThumb[path];
    QImage img(path);
    if (img.isNull()) return QPixmap();
    QSize thumbSize{72 * logicalDpiX() / 96, 72 * logicalDpiY() / 96};
    QImage scaled(img.scaled(thumbSize,Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    QImage thumb(thumbSize, QImage::Format_RGB32);
    QPainter painter(&thumb);
    painter.drawImage(0, 0, scaled);
    painter.end();
    bgThumb[path]=QPixmap::fromImage(thumb);
    return bgThumb[path];
}

void BgImageSelector::updateSetting()
{
    QStringList bgs;
    for (int i = 0; i < candidates.size(); ++i)
    {
        if (candidates[i]->isVisible())
        {
            const QString path = candidates[i]->property("_img_path").toString();
            if (!path.isEmpty()) bgs << path;
        }
    }
    GlobalObjects::appSetting->setValue("MainWindow/HistoryBackgrounds", bgs);
}

BgColorPicker::BgColorPicker(const QColor &color, QWidget *parent)
{
    ColorPreview *colorPreview = new ColorPreview(this);
    colorPreview->setColor(color);
    colorPreview->setFixedSize(42, 42);
    ColorSlider *sliderHue = new ColorSlider(this);
    QLinearGradient hueGradient;
    int colorSeg = 10;
    for (int i=0; i <= colorSeg; ++i)
    {
        double pos = 1.0 / colorSeg * i;
        QColor colorHsl = QColor::fromHsv(pos* 359,255,255);
        hueGradient.setColorAt(pos, colorHsl);
    }
    sliderHue->setGradient(hueGradient);
    sliderHue->setRange(0, 359);
    sliderHue->setValue(color.hue());

    ColorSlider *sliderLightness = new ColorSlider(this);
    sliderLightness->setRange(100, 255);
    sliderLightness->setValue(color.value());
    QLinearGradient lightnessGradient;
    lightnessGradient.setColorAt(0, QColor::fromHsv(sliderHue->value(),255,100));
    lightnessGradient.setColorAt(1, QColor::fromHsv(sliderHue->value(),255,255));
    sliderLightness->setGradient(lightnessGradient);
    QObject::connect(sliderHue, &ColorSlider::valueChanged, this, [=](int val){
        QLinearGradient lightnessGradient;
        lightnessGradient.setColorAt(0, QColor::fromHsv(val,255,100));
        lightnessGradient.setColorAt(1, QColor::fromHsv(val,255,255));
        sliderLightness->setGradient(lightnessGradient);
        colorPreview->setColor(QColor::fromHsv(sliderHue->value(), 255, sliderLightness->value()));
    });
    QObject::connect(sliderHue, &ColorSlider::sliderUp, this, [=](int ){
        QColor color = QColor::fromHsv(sliderHue->value(), 255, sliderLightness->value());
        emit colorChanged(color);
    });

    QObject::connect(sliderLightness, &ColorSlider::valueChanged, this, [=](){
        colorPreview->setColor(QColor::fromHsv(sliderHue->value(), 255, sliderLightness->value()));
    });
    QObject::connect(sliderLightness, &ColorSlider::sliderUp, this, [=](int ){
        QColor color = QColor::fromHsv(sliderHue->value(), 255, sliderLightness->value());
        emit colorChanged(color);
    });

    QGridLayout *gLayout = new QGridLayout(this);
    gLayout->addWidget(colorPreview, 0, 0, 2, 1);
    gLayout->addWidget(sliderHue, 0, 1);
    gLayout->addWidget(sliderLightness, 1, 1);
}

QSize BgColorPicker::sizeHint() const
{
    return layout()->sizeHint();
}

#include "playerpage.h"
#include <QLabel>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QSettings>
#include <QTabWidget>
#include <QPushButton>
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaSlider.h"
#include "UI/ela/ElaSpinBox.h"
#include "UI/ela/ElaToggleSwitch.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "UI/dialogs/mpvconfediror.h"
#include "UI/widgets/colorpreview.h"


PlayerPage::PlayerPage(QWidget *parent) : SettingPage(parent)
{
    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    itemVLayout->addWidget(initBehaviorArea());
    itemVLayout->addWidget(initMpvArea());
    itemVLayout->addWidget(initSubArea());
    itemVLayout->addStretch(1);
}

SettingItemArea *PlayerPage::initBehaviorArea()
{
    SettingItemArea *behaviorArea = new SettingItemArea(tr("Behavior"), this);

    ElaComboBox *clickBehaviorCombo = new ElaComboBox(this);
    clickBehaviorCombo->addItems({tr("Play/Pause"), tr("Show/Hide PlayControl")});
    clickBehaviorCombo->setCurrentIndex(GlobalObjects::mpvplayer->getClickBehavior());
    behaviorArea->addItem(tr("Click Behavior"), clickBehaviorCombo);

    ElaComboBox *dbClickBehaviorCombo = new ElaComboBox(this);
    dbClickBehaviorCombo->addItems({tr("FullScreen"), tr("Play/Pause")});
    dbClickBehaviorCombo->setCurrentIndex(GlobalObjects::mpvplayer->getDbClickBehavior());
    behaviorArea->addItem(tr("Double Click Behavior"), dbClickBehaviorCombo);

    ElaToggleSwitch *showPreviewSwitch = new ElaToggleSwitch(this);
    showPreviewSwitch->setIsToggled(GlobalObjects::mpvplayer->getShowPreview());
    behaviorArea->addItem(tr("Show Preview Over ProgressBar(Restart required)"), showPreviewSwitch);

    ElaToggleSwitch *showRecentlySwitch = new ElaToggleSwitch(this);
    showRecentlySwitch->setIsToggled(GlobalObjects::mpvplayer->getShowRecent());
    behaviorArea->addItem(tr("Show Recently Played Files"), showRecentlySwitch);

    QObject::connect(clickBehaviorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [](int index){
        GlobalObjects::mpvplayer->setClickBehavior(index);
    });

    QObject::connect(dbClickBehaviorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [](int index){
        GlobalObjects::mpvplayer->setDbClickBehavior(index);
    });

    QObject::connect(showPreviewSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::mpvplayer->setShowPreview(checked);
    });

    QObject::connect(showRecentlySwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::mpvplayer->setShowRecent(checked);
    });

    return behaviorArea;
}

SettingItemArea *PlayerPage::initMpvArea()
{
    SettingItemArea *mpvArea = new SettingItemArea(tr("MPV"), this);

    KPushButton *confEditBtn = new KPushButton(tr("Edit"), this);
    mpvArea->addItem(tr("Player Configuration"), confEditBtn);


    QObject::connect(confEditBtn, &KPushButton::clicked, this, [=](){
        MPVConfEdiror editor(this);
        editor.exec();
    });

    return mpvArea;
}

SettingItemArea *PlayerPage::initSubArea()
{
    SettingItemArea* subArea = new SettingItemArea(tr("Subtitle"), this);

    ElaComboBox *subAutoCombo = new ElaComboBox(this);
    QStringList subAutoItems{"no", "exact", "fuzzy", "all"};
    subAutoCombo->addItems(subAutoItems);
    subAutoCombo->setCurrentIndex(subAutoItems.indexOf(GlobalObjects::mpvplayer->getSubAuto()));
    subArea->addItem(tr("Load Subtitle Files"), subAutoCombo);
    QLabel *descLabel = new QLabel(tr("  no: \tDon't automatically load external subtitle files\n  exact: \tLoad the media filename with subtitle file extension and possibly language suffixes\n  fuzzy: \tLoad all subs containing the media filename\n  all: \tLoad all subs in the current and --sub-file-paths directories"), subArea);
    descLabel->setObjectName(QStringLiteral("SettingDescLabel"));
    subArea->addItem(descLabel, Qt::AlignLeft);

    ElaComboBox *fontCombo = new ElaComboBox(this);
    const QStringList fontFamilies = QFontDatabase::families();
    fontCombo->addItems(fontFamilies);
    fontCombo->setEditable(true);
    fontCombo->setCurrentText(GlobalObjects::mpvplayer->getSubFont());
    subArea->addItem(tr("Font"), fontCombo);

    ElaSpinBox *subFontSpin = new ElaSpinBox(this);
    subFontSpin->setRange(1, 200);
    subFontSpin->setValue(GlobalObjects::mpvplayer->getSubFontSize());
    subArea->addItem(tr("Font Size"), subFontSpin);

    ElaToggleSwitch *boldSwitch = new ElaToggleSwitch(this);
    boldSwitch->setIsToggled(GlobalObjects::mpvplayer->getSubBold());
    subArea->addItem(tr("Bold"), boldSwitch);

    ColorPreview *subFontColorPreview = new ColorPreview(this);
    subFontColorPreview->setUseColorDialog(true);
    subFontColorPreview->setColor(QColor(QRgba64::fromArgb32(GlobalObjects::mpvplayer->getSubColor())));
    subFontColorPreview->setFixedSize(42, 42);
    subArea->addItem(tr("Color"), subFontColorPreview);

    ColorPreview *subOutlineColorPreview = new ColorPreview(this);
    subOutlineColorPreview->setUseColorDialog(true);
    subOutlineColorPreview->setColor(QColor(QRgba64::fromArgb32(GlobalObjects::mpvplayer->getSubOutlineColor())));
    subOutlineColorPreview->setFixedSize(42, 42);
    subArea->addItem(tr("Outline Color"), subOutlineColorPreview);

    ColorPreview *subBackColorPreview = new ColorPreview(this);
    subBackColorPreview->setUseColorDialog(true);
    subBackColorPreview->setColor(QColor(QRgba64::fromArgb32(GlobalObjects::mpvplayer->getSubBackColor())));
    subBackColorPreview->setFixedSize(42, 42);
    subArea->addItem(tr("Back Color"), subBackColorPreview);

    ElaSlider *outlineSizeSlider = new ElaSlider(Qt::Orientation::Horizontal, this);
    outlineSizeSlider->setMinimumWidth(200);
    outlineSizeSlider->setRange(0, 1000);
    outlineSizeSlider->setValue(GlobalObjects::mpvplayer->getSubOutlineSize() * 100);
    outlineSizeSlider->setToolTip(QString::number(GlobalObjects::mpvplayer->getSubOutlineSize()));
    subArea->addItem(tr("Outline Size"), outlineSizeSlider);

    ElaComboBox *subBorderStyleCombo = new ElaComboBox(this);
    QStringList subBorderStyleItems{"outline-and-shadow", "opaque-box", "background-box"};
    subBorderStyleCombo->addItems(subBorderStyleItems);
    subBorderStyleCombo->setCurrentIndex(subBorderStyleItems.indexOf(GlobalObjects::mpvplayer->getSubBorderStyle()));
    subArea->addItem(tr("Border Style"), subBorderStyleCombo);

    QObject::connect(subAutoCombo, &ElaComboBox::currentTextChanged, GlobalObjects::mpvplayer, &MPVPlayer::setSubAuto);
    QObject::connect(fontCombo, &ElaComboBox::currentTextChanged, GlobalObjects::mpvplayer, &MPVPlayer::setSubFont);
    QObject::connect(subFontSpin, QOverload<int>::of(&ElaSpinBox::valueChanged), GlobalObjects::mpvplayer, &MPVPlayer::setSubFontSize);
    QObject::connect(boldSwitch, &ElaToggleSwitch::toggled, GlobalObjects::mpvplayer, &MPVPlayer::setSubBold);

    QObject::connect(subFontColorPreview, &ColorPreview::colorChanged, GlobalObjects::mpvplayer, &MPVPlayer::setSubColor);
    QObject::connect(subOutlineColorPreview, &ColorPreview::colorChanged, GlobalObjects::mpvplayer, &MPVPlayer::setSubOutlineColor);
    QObject::connect(subBackColorPreview, &ColorPreview::colorChanged, GlobalObjects::mpvplayer, &MPVPlayer::setSubBackColor);

    QObject::connect(outlineSizeSlider, &QSlider::valueChanged, this, [=](int val){
        outlineSizeSlider->setToolTip(QString::number(val/100.));
        GlobalObjects::mpvplayer->setSubOutlineSize(val/100.);
    });
    QObject::connect(subBorderStyleCombo, &ElaComboBox::currentTextChanged, GlobalObjects::mpvplayer, &MPVPlayer::setSubBorderStyle);

    return subArea;

}


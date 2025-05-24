#include "danmupage.h"
#include <QLabel>
#include <QFontDatabase>
#include <QVBoxLayout>
#include "Play/Danmu/Render/danmurender.h"
#include "Play/Danmu/danmupool.h"
#include "UI/dialogs/blockeditor.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaSlider.h"
#include "UI/ela/ElaToggleSwitch.h"
#include "UI/ela/ElaSpinBox.h"
#include "Play/Danmu/Render/cacheworker.h"
#include "Play/Video/mpvplayer.h"


DanmuPage::DanmuPage(QWidget *parent)
{
    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    itemVLayout->addWidget(initStyleArea());
    itemVLayout->addWidget(initMergeArea());
    itemVLayout->addWidget(initOtherArea());
    itemVLayout->addStretch(1);
}

void DanmuPage::onAccept()
{

}

void DanmuPage::onClose()
{

}

void DanmuPage::updateDanmuPreview(DanmuStyle *danmuStyle)
{
    DanmuComment previewComment1;
    previewComment1.text = tr("KikoPlay Danmu Test");
    previewComment1.fontSizeLevel = DanmuComment::FontSizeLevel::Normal;
    previewComment1.color = 0xffffff;
    DanmuComment previewComment2;
    previewComment2.text = tr("Small Danmu");
    previewComment2.fontSizeLevel = DanmuComment::FontSizeLevel::Small;
    previewComment2.color = 0xff0000;
    DanmuComment previewComment3;
    previewComment3.text = tr("Large Danmu");
    previewComment3.fontSizeLevel = DanmuComment::FontSizeLevel::Large;
    previewComment3.color = 0xffff00;

    QImage imgs[] = {
        CacheWorker::createPreviewImage(danmuStyle, &previewComment1),
        CacheWorker::createPreviewImage(danmuStyle, &previewComment2),
        CacheWorker::createPreviewImage(danmuStyle, &previewComment3),
    };

    const float pxR = danmuStyle->devicePixelRatioF;
    QPixmap pixmap(previewLabel->size() * pxR);
    pixmap.setDevicePixelRatio(pxR);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 80));
    p.drawRoundedRect(QRect(0, 0, previewLabel->width(), previewLabel->height()), 4, 4);
    int py = 0;
    int totalHeight = 0;
    for (const QImage &img : imgs)
    {
        totalHeight += img.height() / pxR;
    }
    if (totalHeight < previewLabel->height())
    {
        py = (previewLabel->height() - totalHeight) / 2;
    }
    for (QImage &img : imgs)
    {
        int w = img.width();
        int h = img.height();
        p.drawImage(QRect((pixmap.width() - w) / pxR / 2, py, w / pxR, h / pxR), img, QRect(0, 0, w, h));
        py += img.height() / pxR;
    }
    p.end();
    previewLabel->setPixmap(pixmap);
}

SettingItemArea *DanmuPage::initStyleArea()
{
    SettingItemArea *styleArea = new SettingItemArea(tr("Style"), this);

    previewLabel = new QLabel(styleArea);
    previewLabel->setFixedSize(300, 130);
    previewLabel->setScaledContents(true);
    styleArea->addItem(previewLabel, Qt::AlignCenter);

    ElaComboBox *fontCombo = new ElaComboBox(this);
    const QStringList fontFamilies = QFontDatabase::families();
    fontCombo->addItems(fontFamilies);
    fontCombo->setCurrentIndex(fontFamilies.indexOf(GlobalObjects::danmuRender->getFontFamily()));
    styleArea->addItem(tr("Font"), fontCombo);

    ElaSlider *strokeWidthSlider = new ElaSlider(Qt::Orientation::Horizontal, this);
    strokeWidthSlider->setMinimumWidth(200);
    strokeWidthSlider->setRange(0, 80);
    strokeWidthSlider->setValue(GlobalObjects::danmuRender->getStrokeWidth() * 10);
    strokeWidthSlider->setToolTip(QString::number(GlobalObjects::danmuRender->getStrokeWidth()));
    styleArea->addItem(tr("Stroke Width"), strokeWidthSlider);

    ElaToggleSwitch *glowSwitch = new ElaToggleSwitch(this);
    glowSwitch->setIsToggled(GlobalObjects::danmuRender->isGlow());
    styleArea->addItem(tr("Glow"), glowSwitch);

    ElaToggleSwitch *boldSwitch = new ElaToggleSwitch(this);
    boldSwitch->setIsToggled(GlobalObjects::danmuRender->isBold());
    styleArea->addItem(tr("Bold"), boldSwitch);

    ElaToggleSwitch *randomSizeSwitch = new ElaToggleSwitch(this);
    randomSizeSwitch->setIsToggled(GlobalObjects::danmuRender->isRandomSize());
    styleArea->addItem(tr("Random Size"), randomSizeSwitch);

    ElaToggleSwitch *randomColorSwitch = new ElaToggleSwitch(this);
    randomColorSwitch->setIsToggled(GlobalObjects::danmuRender->isRandomColor());
    styleArea->addItem(tr("Random Color"), randomColorSwitch);

    static DanmuStyle danmuStyle;
    static int fontSizeTable[] = {20, 0, 0};
    fontSizeTable[1] = fontSizeTable[0] / 1.5;
    fontSizeTable[2] = fontSizeTable[0] * 1.5;
    danmuStyle.fontSizeTable = fontSizeTable;
    danmuStyle.fontFamily = GlobalObjects::danmuRender->getFontFamily();
    danmuStyle.strokeWidth = GlobalObjects::danmuRender->getStrokeWidth();
    danmuStyle.bold = GlobalObjects::danmuRender->isBold();
    danmuStyle.glow = GlobalObjects::danmuRender->isGlow();
    danmuStyle.glowRadius = 16;
    danmuStyle.randomSize = GlobalObjects::danmuRender->isRandomSize();
    danmuStyle.randomColor = GlobalObjects::danmuRender->isRandomColor();
    danmuStyle.devicePixelRatioF = GlobalObjects::context()->devicePixelRatioF;

    QObject::connect(fontCombo, &ElaComboBox::currentTextChanged, this, [=](const QString &fontFamily){
        danmuStyle.fontFamily = fontFamily;
        this->updateDanmuPreview(&danmuStyle);
        GlobalObjects::danmuRender->setFontFamily(danmuStyle.fontFamily);
    });

    QObject::connect(strokeWidthSlider, &QSlider::valueChanged, this, [=](int val){
        danmuStyle.strokeWidth = val / 10.0;
        strokeWidthSlider->setToolTip(QString::number(val/10.));
        this->updateDanmuPreview(&danmuStyle);
        GlobalObjects::danmuRender->setStrokeWidth(danmuStyle.strokeWidth);
    });

    QObject::connect(boldSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        danmuStyle.bold = checked;
        this->updateDanmuPreview(&danmuStyle);
        GlobalObjects::danmuRender->setBold(checked);
    });

    QObject::connect(glowSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        danmuStyle.glow = checked;
        this->updateDanmuPreview(&danmuStyle);
        GlobalObjects::danmuRender->setGlow(checked);
    });

    QObject::connect(randomSizeSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        danmuStyle.randomSize = checked;
        this->updateDanmuPreview(&danmuStyle);
        GlobalObjects::danmuRender->setRandomSize(checked);
    });

    QObject::connect(randomColorSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        danmuStyle.randomColor = checked;
        this->updateDanmuPreview(&danmuStyle);
        GlobalObjects::danmuRender->setRandomColor(checked);
    });

    updateDanmuPreview(&danmuStyle);

    return styleArea;
}

SettingItemArea *DanmuPage::initMergeArea()
{
    SettingItemArea *mergeArea = new SettingItemArea(tr("Merge"), this);

    ElaToggleSwitch *mergeSwitch = new ElaToggleSwitch(this);
    mergeSwitch->setIsToggled(GlobalObjects::danmuPool->isEnableMerge());
    mergeArea->addItem(tr("Enable Danmu Merge"), mergeSwitch);

    ElaToggleSwitch *enlargeMergedSwitch = new ElaToggleSwitch(this);
    enlargeMergedSwitch->setIsToggled(GlobalObjects::danmuRender->isEnlargeMerged());
    mergeArea->addItem(tr("Enlarge Merged Danmu"), enlargeMergedSwitch);

    ElaComboBox *mergeCountTipPosCombo = new ElaComboBox(this);
    mergeCountTipPosCombo->addItems({tr("Hidden"), tr("Forward"), tr("Backward")});
    mergeCountTipPosCombo->setCurrentIndex(GlobalObjects::danmuRender->getMergeCountPos());
    mergeArea->addItem(tr("Merge Count Tip Position"), mergeCountTipPosCombo);

    ElaSpinBox *minMergeCountSpin = new ElaSpinBox(this);
    minMergeCountSpin->setRange(1, 6);
    minMergeCountSpin->setValue(GlobalObjects::danmuPool->getMinMergeCount());
    mergeArea->addItem(tr("Minimum Danmu Required to Merge"), minMergeCountSpin);

    ElaSpinBox *mergeIntervalSpin = new ElaSpinBox(this);
    mergeIntervalSpin->setRange(1, 30);
    mergeIntervalSpin->setValue(GlobalObjects::danmuPool->getMergeInterval());
    mergeArea->addItem(tr("Merge Interval(s)"), mergeIntervalSpin);

    QObject::connect(mergeSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::danmuPool->setMergeEnable(checked);
    });

    QObject::connect(enlargeMergedSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::danmuRender->setEnlargeMerged(checked);
    });

    QObject::connect(mergeCountTipPosCombo, (void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, this, [](int index){
        GlobalObjects::danmuRender->setMergeCountPos(index);
    });

    QObject::connect(minMergeCountSpin, QOverload<int>::of(&ElaSpinBox::valueChanged), this, [=](int val){
        GlobalObjects::danmuPool->setMinMergeCount(val);
    });

    QObject::connect(mergeIntervalSpin, QOverload<int>::of(&ElaSpinBox::valueChanged), this, [=](int val){
        GlobalObjects::danmuPool->setMergeInterval(val*1000);
    });

    return mergeArea;
}

SettingItemArea *DanmuPage::initOtherArea()
{
    SettingItemArea *otherArea = new SettingItemArea(tr("Other"), this);


    ElaToggleSwitch *analyzeSwitch = new ElaToggleSwitch(this);
    analyzeSwitch->setIsToggled(GlobalObjects::danmuPool->isEnableAnalyze());
    otherArea->addItem(tr("Enable Danmu Event Analyze"), analyzeSwitch);

    ElaToggleSwitch *autoLoadLocalDanmuSwitch = new ElaToggleSwitch(this);
    autoLoadLocalDanmuSwitch->setIsToggled(GlobalObjects::danmuPool->isLoadLocalDanmu());
    otherArea->addItem(tr("Auto Load Local Danmu"), autoLoadLocalDanmuSwitch);

    ElaToggleSwitch *disableSample2DArraySwitch = new ElaToggleSwitch(this);
    disableSample2DArraySwitch->setIsToggled(!GlobalObjects::mpvplayer->getUseSample2DArray());
    otherArea->addItem(tr("Disable Sample2DArray(Restart required)"), disableSample2DArraySwitch, tr("If danmu display is abnormal on some AMD graphics cards, try enabling"));

    KPushButton *blockEditBtn = new KPushButton(tr("Edit"), this);
    otherArea->addItem(tr("Block Rules"), blockEditBtn);

    QObject::connect(analyzeSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::danmuPool->setAnalyzeEnable(checked);
    });

    QObject::connect(autoLoadLocalDanmuSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::danmuPool->setLoadLocalDanmu(checked);
    });

    QObject::connect(disableSample2DArraySwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::mpvplayer->setUseSample2DArray(!checked);
    });

    QObject::connect(blockEditBtn, &KPushButton::clicked, this, [=](){
        BlockEditor editor(this);
        editor.exec();
    });

    return otherArea;
}

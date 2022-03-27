#include "stylepage.h"
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
#include "globalobjects.h"
#include "../widgets/colorslider.h"
#include "Common/threadtask.h"
#include <algorithm>

QHash<QString, QPixmap> StylePage::bgThumb;
namespace  {
    class ColorPreview : public QWidget
    {
    public:
        using QWidget::QWidget;
        void setColor(const QColor &c)
        {
            color = c;
            update();
        }
    protected:
        void paintEvent(QPaintEvent *event) override
        {
            QPainter p(this);
            p.fillRect(rect(), color);
            QWidget::paintEvent(event);
        }
    private:
        QColor color;
    };
}
StylePage::StylePage(QWidget *parent) : SettingPage(parent)
{
    thumbSize = QSize(120*logicalDpiX()/96,120*logicalDpiY()/96);
    historyBgs = GlobalObjects::appSetting->value("MainWindow/HistoryBackgrounds").toStringList();
    QListWidget *bgImgView=new QListWidget(this);
    bgImgView->setSelectionMode(QAbstractItemView::SingleSelection);
    bgImgView->setIconSize(thumbSize);
    bgImgView->setViewMode(QListView::IconMode);
    bgImgView->setUniformItemSizes(true);
    bgImgView->setResizeMode(QListView::Adjust);
    bgImgView->setMovement(QListView::Static);

    setSlide();
    QLabel *hideToTrayTip = new QLabel(tr("Hide To Tray"), this);
    hideToTrayCombo = new QComboBox(this);
    QStringList hideTypes{tr("None"), tr("Minimize"), tr("Close Window")};
    for(int i = 0; i < static_cast<int>(MainWindow::HideToTrayType::UNKNOWN); ++i)
    {
        hideToTrayCombo->addItem(hideTypes[i], i);
    }
    darkMode = new QCheckBox(tr("Dark Mode"), this);
    enableBg = new QCheckBox(tr("Enable Background"), this);
    customColor = new QCheckBox(tr("Custom Color"), this);
    darkMode->setChecked(GlobalObjects::appSetting->value("MainWindow/DarkMode", false).toBool());
    enableBg->setChecked(!GlobalObjects::appSetting->value("MainWindow/Background").toString().isEmpty());
    customColor->setChecked(GlobalObjects::appSetting->value("MainWindow/CustomColor", false).toBool());

    QLabel *bgLightnessTip = new QLabel(tr("Background Darkness"), this);


    QGridLayout *styleGLayout = new QGridLayout(this);
    styleGLayout->setContentsMargins(0, 0, 0, 0);
    styleGLayout->addWidget(hideToTrayTip, 0, 0);
    styleGLayout->addWidget(hideToTrayCombo, 0, 1);
    styleGLayout->addWidget(darkMode, 1, 0, 1, 4);
    styleGLayout->addWidget(enableBg, 2, 0, 1, 4);
    styleGLayout->addWidget(bgLightnessTip, 3, 0);
    styleGLayout->addWidget(sliderBgDarkness, 3, 1, 1, 3);
    styleGLayout->addWidget(bgImgView, 5, 0, 1, 4);
    styleGLayout->addWidget(customColor, 6, 0, 1, 3);
    styleGLayout->addWidget(sliderHue, 7, 0, 1, 3);
    styleGLayout->addWidget(colorPreview, 7, 3, 2, 1);
    styleGLayout->addWidget(sliderLightness, 8, 0, 1, 3);
    styleGLayout->setRowStretch(1, 1);
    styleGLayout->setColumnStretch(2, 1);

    setBgList(bgImgView);
    bgImgView->setEnabled(enableBg->isChecked());
    customColor->setEnabled(enableBg->isChecked());
    sliderBgDarkness->setEnabled(enableBg->isChecked());
    sliderHue->setEnabled(customColor->isChecked() && enableBg->isChecked());
    sliderLightness->setEnabled(customColor->isChecked() && enableBg->isChecked());
    QObject::connect(enableBg, &QCheckBox::stateChanged, this, [this, bgImgView](int state){
        if(state==Qt::Checked)
        {
            bgImgView->setEnabled(true);
            sliderBgDarkness->setEnabled(true);
            customColor->setEnabled(true);
            sliderHue->setEnabled(true);
            sliderLightness->setEnabled(true);
            if(bgImgView->count()>0)
            {
                QString path(bgImgView->item(0)->data(Qt::UserRole).toString());
                QColor color;
                if(customColor->isChecked()) color=QColor::fromHsv(sliderHue->value(),255,sliderLightness->value());
                emit setBackground(path, color);
            }
        }
        else if(state==Qt::Unchecked)
        {
            emit setBackground("");
            bgImgView->setEnabled(false);
            sliderBgDarkness->setEnabled(false);
            customColor->setEnabled(false);
            sliderHue->setEnabled(false);
            sliderLightness->setEnabled(false);
        }
    });
    QObject::connect(sliderBgDarkness, &QSlider::valueChanged, this, [this](int val){
        bgDarknessChanged = true;
        emit setBgDarkness(val);
    });
    QObject::connect(customColor, &QCheckBox::stateChanged, this, [this](int state){
        colorChanged = true;
        if(state==Qt::Checked)
        {
            sliderHue->setEnabled(true);
            sliderLightness->setEnabled(true);
            emit setThemeColor(QColor::fromHsv(sliderHue->value(),255,sliderLightness->value()));
        }
        else if(state==Qt::Unchecked)
        {
            sliderHue->setEnabled(false);
            sliderLightness->setEnabled(false);
            emit setThemeColor(QColor());
        }
    });
    QObject::connect(darkMode, &QCheckBox::stateChanged, this, [this](int state){
        darkModeChanged = true;
        StyleManager::getStyleManager()->setCondVariable("DarkMode", state==Qt::Checked);
    });

    MainWindow *mW = static_cast<MainWindow *>(GlobalObjects::mainWindow);
    QObject::connect(this, &StylePage::setBackground, mW, (void (MainWindow::*)(const QString &, const QColor &))&MainWindow::setBackground);
    QObject::connect(this, &StylePage::setBgDarkness, mW, &MainWindow::setBgDarkness);
    QObject::connect(this, &StylePage::setThemeColor, mW, &MainWindow::setThemeColor);
    hideToTrayCombo->setCurrentIndex(static_cast<int>(mW->getHideToTrayType()));
}

void StylePage::onAccept()
{
    if(bgDarknessChanged) GlobalObjects::appSetting->setValue("MainWindow/BackgroundDarkness", sliderBgDarkness->value());
    if(bgChanged) GlobalObjects::appSetting->setValue("MainWindow/HistoryBackgrounds", historyBgs);
    if(darkModeChanged) GlobalObjects::appSetting->setValue("MainWindow/DarkMode", darkMode->isChecked());
    if(colorChanged)
    {
        GlobalObjects::appSetting->setValue("MainWindow/CustomColorHSV", QColor::fromHsv(sliderHue->value(), 255, sliderLightness->value()));
        GlobalObjects::appSetting->setValue("MainWindow/CustomColor", customColor->isChecked());
    }
    static_cast<MainWindow *>(GlobalObjects::mainWindow)->setHideToTrayType(
                static_cast<MainWindow::HideToTrayType>(hideToTrayCombo->currentData().toInt()));
}

void StylePage::onClose()
{
    onAccept();
}

void StylePage::setBgList(QListWidget *bgImgView)
{
    QObject::connect(bgImgView, &QListWidget::itemDoubleClicked, this, [this, bgImgView](QListWidgetItem *item){
        QString path(item->data(Qt::UserRole).toString());
        if(path != GlobalObjects::appSetting->value("MainWindow/Background").toString())
        {
            bgImgView->insertItem(0, bgImgView->takeItem(bgImgView->row(item)));
            emit setBackground(path);
            updateSetting(path);
        }
    });
    QAction* actAdd = new QAction(tr("Add"),this);
    QObject::connect(actAdd, &QAction::triggered, this, [this, bgImgView](bool)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Image"), "",
                                                        "Image Files(*.jpg *.png);;All Files(*)");
        if(!fileName.isEmpty())
        {
            QPixmap thumb(getThumb(fileName));
            if(thumb.isNull()) return ;
            QListWidgetItem *item = new QListWidgetItem(QIcon(thumb), nullptr);
            item->setData(Qt::UserRole, fileName);
            item->setToolTip(fileName);
            bgImgView->insertItem(0, item);
            updateSetting(fileName);
            if(bgImgView->count()>maxBgCount)
            {
                QScopedPointer<QListWidgetItem> last(bgImgView->item(bgImgView->count()-1));
                bgThumb.remove(last->data(Qt::UserRole).toString());
            }
            QColor color;
            if(customColor->isChecked()) color=QColor::fromHsv(sliderHue->value(),255,sliderLightness->value());
            emit setBackground(fileName, color);
        }
    });
    QAction* actRemove = new QAction(tr("Remove"), this);
    QObject::connect(actRemove, &QAction::triggered, this, [bgImgView, this]()
    {
        QScopedPointer<QListWidgetItem> curItem(bgImgView->currentItem());
        if(!curItem) return;
        QString path(curItem->data(Qt::UserRole).toString());
        updateSetting(path, false);
        bgThumb.remove(path);
        if(path == GlobalObjects::appSetting->value("MainWindow/Background").toString())
        {
            QColor color;
            if(customColor->isChecked()) color=QColor::fromHsv(sliderHue->value(),255,sliderLightness->value());
            emit setBackground(bgImgView->count()>1?
                                   bgImgView->item(1)->data(Qt::UserRole).toString():"", color);
        }
    });
    bgImgView->setContextMenuPolicy(Qt::ActionsContextMenu);
    bgImgView->addAction(actAdd);
    bgImgView->addAction(actRemove);

    QTimer::singleShot(0, this, [this, bgImgView](){
        ThreadTask t(GlobalObjects::workThread);
        showBusyState(true);
        t.Run([this](){
            QStringList historyBgs = GlobalObjects::appSetting->value("MainWindow/HistoryBackgrounds").toStringList();
            for_each(historyBgs.begin(), historyBgs.end(), std::bind(&StylePage::getThumb, this, std::placeholders::_1));
            return 0;
        });
        QStringList historyBgs = GlobalObjects::appSetting->value("MainWindow/HistoryBackgrounds").toStringList();
        for(auto &path : historyBgs)
        {
            QPixmap thumb(getThumb(path));
            if(thumb.isNull()) continue;
            QListWidgetItem *item = new QListWidgetItem(QIcon(thumb), nullptr);
            item->setData(Qt::UserRole, path);
            item->setToolTip(path);
            bgImgView->addItem(item);
        }
        showBusyState(false);
    });
}

void StylePage::updateSetting(const QString &path, bool add)
{
    historyBgs.removeOne(path);
    if(add) historyBgs.insert(0, path);
    if(historyBgs.count()>maxBgCount) historyBgs.removeLast();
    bgChanged = true;
}

QPixmap StylePage::getThumb(const QString &path)
{
    if(bgThumb.contains(path)) return bgThumb[path];
    QImage img(path);
    if(img.isNull()) return QPixmap();
    QImage scaled(img.scaled(thumbSize,Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    QImage thumb(thumbSize,QImage::Format_RGB32);
    QPainter painter(&thumb);
    painter.drawImage(0,0,scaled);
    painter.end();
    bgThumb[path]=QPixmap::fromImage(thumb);
    return bgThumb[path];
}

void StylePage::setSlide()
{
    colorPreview = new ColorPreview(this);
    colorPreview->setFixedSize(40*logicalDpiX()/96, 40*logicalDpiX()/96);
    sliderHue = new ColorSlider(this);
    QLinearGradient hueGradient;
    int colorSeg = 8;
    for(int i=0; i<= colorSeg; ++i)
    {
        double pos = 1.0 / colorSeg * i;
        QColor colorHsl = QColor::fromHsv(pos* 359,255,255);
        hueGradient.setColorAt(pos, colorHsl);
    }
    sliderHue->setGradient(hueGradient);
    sliderHue->setRange(0,359);
    QColor customColorHSL = GlobalObjects::appSetting->value("MainWindow/CustomColorHSV", QColor::fromHsv(180, 255, 100)).value<QColor>();
    sliderHue->setValue(customColorHSL.hue());

    sliderLightness = new ColorSlider(this);
    sliderLightness->setRange(100,255);
    sliderLightness->setValue(customColorHSL.value());
    QLinearGradient lightnessGradient;
    lightnessGradient.setColorAt(0, QColor::fromHsv(sliderHue->value(),255,100));
    lightnessGradient.setColorAt(1, QColor::fromHsv(sliderHue->value(),255,255));
    sliderLightness->setGradient(lightnessGradient);
    QObject::connect(sliderHue, &ColorSlider::valueChanged, this, [this](int val){
        QLinearGradient lightnessGradient;
        lightnessGradient.setColorAt(0, QColor::fromHsv(val,255,100));
        lightnessGradient.setColorAt(1, QColor::fromHsv(val,255,255));
        sliderLightness->setGradient(lightnessGradient);
        static_cast<ColorPreview *>(colorPreview)->setColor(QColor::fromHsv(sliderHue->value(),255,sliderLightness->value()));
        colorChanged = true;
    });
    QObject::connect(sliderHue, &ColorSlider::sliderUp, this, [this](int ){
        emit setThemeColor(QColor::fromHsv(sliderHue->value(),255,sliderLightness->value()));
    });

    QObject::connect(sliderLightness, &ColorSlider::valueChanged, this, [this](){
        static_cast<ColorPreview *>(colorPreview)->setColor(QColor::fromHsv(sliderHue->value(),255,sliderLightness->value()));
        colorChanged = true;
    });
    QObject::connect(sliderLightness, &ColorSlider::sliderUp, this, [this](int ){
        emit setThemeColor(QColor::fromHsv(sliderHue->value(),255,sliderLightness->value()));
    });
    static_cast<ColorPreview *>(colorPreview)->setColor(QColor::fromHsv(sliderHue->value(),255,sliderLightness->value()));

    sliderBgDarkness = new ColorSlider(this);
    sliderBgDarkness->setRange(0, 240);
    sliderBgDarkness->setValue(GlobalObjects::appSetting->value("MainWindow/BackgroundDarkness", 100).toInt());
    QLinearGradient darknessGradient;
    darknessGradient.setColorAt(0, Qt::white);
    darknessGradient.setColorAt(1, Qt::black);
    sliderBgDarkness->setGradient(darknessGradient);
}

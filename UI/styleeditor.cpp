#include "styleeditor.h"
#include <QListWidget>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QGridLayout>
#include <QSettings>
#include <QAction>
#include <QFileDialog>
#include <QPainter>
#include <QEvent>
#include <QHoverEvent>
#include <QStylePainter>
#include "globalobjects.h"

QHash<QString, QPixmap> StyleEditor::bgThumb;
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
StyleEditor::StyleEditor(QWidget *parent):CFramelessDialog(tr("Edit Style"), parent)
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

    enableBg = new QCheckBox(tr("Enable Background"), this);
    customColor = new QCheckBox(tr("Custom Color"), this);
    enableBg->setChecked(!GlobalObjects::appSetting->value("MainWindow/Background").toString().isEmpty());
    customColor->setChecked(GlobalObjects::appSetting->value("MainWindow/CustomColor", false).toBool());

    QLabel *bgLightnessTip = new QLabel(tr("Background Darkness"), this);

    QGridLayout *styleGLayout = new QGridLayout(this);
    styleGLayout->addWidget(enableBg, 0, 0, 1, 3);
    styleGLayout->addWidget(bgLightnessTip, 1, 0);
    styleGLayout->addWidget(sliderBgDarkness, 1, 1, 1, 2);
    styleGLayout->addWidget(bgImgView, 2, 0, 1, 3);
    styleGLayout->addWidget(customColor, 3, 0, 1, 3);
    styleGLayout->addWidget(sliderHue, 4, 0, 1, 2);
    styleGLayout->addWidget(colorPreview, 4, 2, 2, 1);
    styleGLayout->addWidget(sliderLightness, 5, 0, 1, 2);
    styleGLayout->setRowStretch(1, 1);
    styleGLayout->setColumnStretch(1, 1);

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
    QObject::connect(sliderBgDarkness, &QSlider::valueChanged, this, &StyleEditor::setBgDarkness);
    QObject::connect(customColor, &QCheckBox::stateChanged, this, [this](int state){
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
    resize(GlobalObjects::appSetting->value("DialogSize/StyleEdit",QSize(400*logicalDpiX()/96,300*logicalDpiY()/96)).toSize());
}

void StyleEditor::setBgList(QListWidget *bgImgView)
{
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
                                                        "JPEG Images (*.jpg);;PNG Images (*.png)");
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
            emit setBackground(bgImgView->count()>1?
                                   bgImgView->item(1)->data(Qt::UserRole).toString():"");
        }
    });
    bgImgView->setContextMenuPolicy(Qt::ActionsContextMenu);
    bgImgView->addAction(actAdd);
    bgImgView->addAction(actRemove);
}

void StyleEditor::updateSetting(const QString &path, bool add)
{
    historyBgs.removeOne(path);
    if(add) historyBgs.insert(0, path);
    if(historyBgs.count()>maxBgCount) historyBgs.removeLast();
}

QPixmap StyleEditor::getThumb(const QString &path)
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

void StyleEditor::setSlide()
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
    });
    QObject::connect(sliderHue, &ColorSlider::sliderUp, this, [this](int ){
        emit setThemeColor(QColor::fromHsv(sliderHue->value(),255,sliderLightness->value()));
    });

    QObject::connect(sliderLightness, &ColorSlider::valueChanged, this, [this](){
        static_cast<ColorPreview *>(colorPreview)->setColor(QColor::fromHsv(sliderHue->value(),255,sliderLightness->value()));
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

void StyleEditor::onClose()
{
    GlobalObjects::appSetting->setValue("DialogSize/StyleEdit",size());
    GlobalObjects::appSetting->setValue("MainWindow/BackgroundDarkness", sliderBgDarkness->value());
    GlobalObjects::appSetting->setValue("MainWindow/HistoryBackgrounds", historyBgs);
    GlobalObjects::appSetting->setValue("MainWindow/CustomColorHSV", QColor::fromHsv(sliderHue->value(), 255, sliderLightness->value()));
    GlobalObjects::appSetting->setValue("MainWindow/CustomColor", customColor->isChecked());
    CFramelessDialog::onClose();
}

void ColorSlider::setEnabled(bool on)
{
    QSlider::setEnabled(on);
    update();
}

void ColorSlider::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect grooveRect(style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove,this));
    fillGradient.setStart(0, 0);
    fillGradient.setFinalStop(grooveRect.width(), 0);
    painter.fillRect(grooveRect, QBrush(fillGradient));
    if(!isEnabled()) painter.fillRect(grooveRect, QColor(255,255,255,200));
    opt.subControls = QStyle::SC_SliderHandle | QStyle::SC_SliderGroove;
    if (pressedControl)
    {
        opt.activeSubControls = pressedControl;
        opt.state |= QStyle::State_Sunken;
    }
    else
    {
        opt.activeSubControls = hoverControl;
    }
    painter.drawComplexControl(QStyle::CC_Slider, opt);
}

void ColorSlider::mousePressEvent(QMouseEvent *ev)
{
    if ((ev->button() & style()->styleHint(QStyle::SH_Slider_AbsoluteSetButtons)) == ev->button())
    {
        pressedControl = QStyle::SC_SliderHandle;
    }
    else if ((ev->button() & style()->styleHint(QStyle::SH_Slider_PageSetButtons)) == ev->button())
    {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        pressedControl = style()->hitTestComplexControl(QStyle::CC_Slider, &opt, ev->pos(), this);
    }
    QSlider::mousePressEvent(ev);
    if(!isSliderDown() && ev->button() == Qt::LeftButton)
    {
        int dur = maximum()-minimum();
        double x=qBound<double>(0., ev->x(), width());
        int pos = minimum() + dur * (x /width());
        emit sliderClick(pos);
        setValue(pos);
    }
    ev->accept();
}

void ColorSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    pressedControl = QStyle::SC_None;

    int dur = maximum()-minimum();
    double x=qBound<double>(0., ev->x(), width());
    int pos = minimum() + dur * (x /width());
    emit sliderUp(pos);

    QSlider::mouseReleaseEvent(ev);
    ev->accept();
}

bool ColorSlider::event(QEvent *event)
{
    switch(event->type())
    {
       case QEvent::HoverEnter:
       case QEvent::HoverLeave:
       case QEvent::HoverMove:
           if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
               updateHoverControl(he->pos());
           break;

    }
    return QSlider::event(event);
}

void ColorSlider::updateHoverControl(const QPoint &pos)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    if (handleRect.contains(pos))
    {
        hoverControl = QStyle::SC_SliderHandle;
    } else if (grooveRect.contains(pos))
    {
        hoverControl = QStyle::SC_SliderGroove;
    } else
    {
        hoverControl = QStyle::SC_None;
    }

}

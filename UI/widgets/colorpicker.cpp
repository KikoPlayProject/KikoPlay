#include "colorpicker.h"
#include <QPainter>
#include <QStyleOption>
#include <QPaintEvent>
#include <QButtonGroup>
#include <QGridLayout>
#include <QColorDialog>

namespace
{
    const int btnW = 32, btnH = 32;
    const int rowCount = 5;
}

ColorPicker::ColorPicker(const QList<QColor> &colorList, QWidget *parent) : QToolButton(parent)
{
    colorMenu = new ColorMenu(colorList, this);
    setPopupMode(QToolButton::InstantPopup);
    setMenu(colorMenu);
    QObject::connect(colorMenu, &ColorMenu::colorChanged, this, [=](const QColor &color){
        curColor = color;
        setIcon(ColorButton::createIcon(curColor, btnW, btnH));
        setToolTip(curColor.name());
        emit colorChanged(curColor);
    });
}

void ColorPicker::setColor(const QColor &color)
{
    colorMenu->setColor(color);
}

void ColorPicker::addColor(const QColor &color)
{
    colorMenu->addColor(color);
}

ColorButton::ColorButton(const QColor &color, int w, int h, QWidget *parent) : QToolButton(parent), btnColor(color)
{
    setIcon(createIcon(color, w, h));
    setFixedSize(iconSize() + QSize(8, 8));
    setToolTip(color.name());
    setCheckable(true);
}

void ColorButton::paintEvent(QPaintEvent *event)
{
    static QColor mHoverColor(QLatin1String("#add8e6"));

    QPainter painter(this);
    QStyleOption styleOption;
    styleOption.initFrom(this);
    auto rect = event->rect();
    auto scaleRatio = devicePixelRatioF();
    auto buttonRect = QRectF(rect.x() + (2 / scaleRatio), rect.y() + (2 / scaleRatio), rect.width() - 5, rect.height() - 5);

    if(styleOption.state & QStyle::State_MouseOver)
    {
        auto defaultPen = painter.pen();
        auto defaultBrush = painter.brush();
        painter.setPen(mHoverColor);
        painter.setBrush(mHoverColor);
        painter.drawRect(buttonRect);
        painter.setPen(defaultPen);
        painter.setBrush(defaultBrush);
    }

    painter.drawPixmap(buttonRect.topLeft() + QPointF(2, 2), icon().pixmap(iconSize()));

    if(isChecked())  painter.drawRect(buttonRect);
}

QIcon ColorButton::createIcon(const QColor &color, int w, int h)
{
    QPixmap pixmap(w, h);
    pixmap.fill(color);
    QPainter painter(&pixmap);
    auto penWidth = painter.pen().width();
    painter.setPen(QColor(Qt::gray));
    painter.drawRect(0, 0, w - penWidth, h - penWidth);
    return QIcon(pixmap);
}

ColorMenu::ColorMenu(const QList<QColor> &colorList, QWidget *parent) : QMenu(parent)
{
    btnGroup = new QButtonGroup(this);
    QGridLayout *btnGLayout = new QGridLayout(this);
    btnGLayout->setSpacing(0);
    btnGLayout->setContentsMargins(5, 5, 5, 5);
    moreColor = new QPushButton(this);
    moreColor->setText(tr("More Colors"));
    QObject::connect(moreColor, &QPushButton::clicked, this, [=](){
        QColor c = QColorDialog::getColor();
        if(!c.isValid()) return;
        setColor(c);
    });
    QObject::connect(btnGroup, &QButtonGroup::buttonToggled, this, [=](QAbstractButton *btn, bool checked) {
        if (!checked) return;
        ColorButton *cb = static_cast<ColorButton *>(btn);
        emit colorChanged(cb->color());
        hide();
    });
    for(const QColor &c : colorList)
    {
        ColorButton *cb = new ColorButton(c, btnW, btnH, this);
        buttons.append(cb);
        btnGroup->addButton(cb);
    }
    resetGrid();
}

void ColorMenu::addColor(const QColor &color)
{
    if(contains(color)) return;
    ColorButton *cb = new ColorButton(color, btnW, btnH, this);
    buttons.push_front(cb);
    btnGroup->addButton(cb);
    clean();
    resetGrid();
}

void ColorMenu::setColor(const QColor &color)
{
    ColorButton *btn = contains(color);
    if(btn)
    {
        btn->setChecked(true);
    }
    else
    {
        addColor(color);
        buttons.first()->setChecked(true);
    }
}

ColorButton *ColorMenu::contains(const QColor &color)
{
    for(ColorButton *btn : buttons)
    {
        if(btn->color() == color) return btn;
    }
    return nullptr;
}

void ColorMenu::resetGrid()
{
    int row = 0, col = 0;
    QGridLayout *gLayout = static_cast<QGridLayout *>(layout());
    for(ColorButton *btn : buttons)
    {
        gLayout->addWidget(btn, row, col++);
        if(col % rowCount == 0)
        {
            ++row;
            col = 0;
        }
    }
    if(col!=0) ++row;
    gLayout->addWidget(moreColor, row, 0, 1, rowCount, Qt::AlignCenter);
}

void ColorMenu::clean()
{
    for(auto btn : buttons)
    {
        layout()->removeWidget(btn);
    }
    layout()->removeWidget(moreColor);
}

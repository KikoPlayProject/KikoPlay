#include "ElaTheme.h"

#include <QPainter>
#include <QPainterPath>

#include "private/ElaThemePrivate.h"
Q_SINGLETON_CREATE_CPP(ElaTheme)
ElaTheme::ElaTheme(QWidget* parent)
    : QWidget{parent}, d_ptr(new ElaThemePrivate())
{
    Q_D(ElaTheme);
    d->q_ptr = this;
    d->_initThemeColor();
}

ElaTheme::~ElaTheme()
{
}

void ElaTheme::setThemeMode(ElaThemeType::ThemeMode themeMode)
{
    Q_D(ElaTheme);
    d->_themeMode = themeMode;
    Q_EMIT themeModeChanged(d->_themeMode);
}

ElaThemeType::ThemeMode ElaTheme::getThemeMode() const
{
    Q_D(const ElaTheme);
    return d->_themeMode;
}

void ElaTheme::drawEffectShadow(QPainter* painter, QRect widgetRect, int shadowBorderWidth, int borderRadius)
{
    Q_D(ElaTheme);
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    QColor color = d->_themeMode == ElaThemeType::Light ? QColor(0x70, 0x70, 0x70) : QColor(0x9C, 0x9B, 0x9E);
    for (int i = 0; i < shadowBorderWidth; i++)
    {
        path.addRoundedRect(shadowBorderWidth - i, shadowBorderWidth - i, widgetRect.width() - (shadowBorderWidth - i) * 2, widgetRect.height() - (shadowBorderWidth - i) * 2, borderRadius + i, borderRadius + i);
        int alpha = 1 * (shadowBorderWidth - i + 1);
        color.setAlpha(alpha > 255 ? 255 : alpha);
        painter->setPen(color);
        painter->drawPath(path);
    }
    painter->restore();
}

void ElaTheme::setThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor, QColor newColor)
{
    Q_D(ElaTheme);
    if (themeMode == ElaThemeType::Light)
    {
        d->_lightThemeColorList[themeColor] = newColor;
    }
    else
    {
        d->_darkThemeColorList[themeColor] = newColor;
    }
}

const QColor& ElaTheme::getThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor) const
{
    Q_D(const ElaTheme);
    if (themeMode == ElaThemeType::Light)
    {
        return d->_lightThemeColorList[themeColor];
    }
    else
    {
        return d->_darkThemeColorList[themeColor];
    }
}

ElaSelfTheme::ElaSelfTheme(QObject *parent) : QObject{parent}, d_ptr(new ElaSelfThemePrivate())
{

}

void ElaSelfTheme::setThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor, QColor newColor)
{
    Q_D(ElaSelfTheme);
    if (themeMode == ElaThemeType::Light)
    {
        d->_lightThemeColorList[themeColor] = newColor;
    }
    else
    {
        d->_darkThemeColorList[themeColor] = newColor;
    }
}

bool ElaSelfTheme::hasThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor)
{
    Q_D(ElaSelfTheme);
    const auto& colorList = themeMode == ElaThemeType::Light? d->_lightThemeColorList : d->_darkThemeColorList;
    return colorList[themeColor].isValid();
}

const QColor &ElaSelfTheme::getThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor)
{
    Q_D(ElaSelfTheme);
    const auto& colorList = themeMode == ElaThemeType::Light? d->_lightThemeColorList : d->_darkThemeColorList;
    return colorList[themeColor];
}

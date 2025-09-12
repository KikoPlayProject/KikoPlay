#ifndef ELACOMBOBOXSTYLE_H
#define ELACOMBOBOXSTYLE_H

#include <QProxyStyle>

#include "../Def.h"
class ElaComboBoxStyle : public QProxyStyle
{
    Q_OBJECT
    Q_PROPERTY_CREATE(qreal, ExpandIconRotate)
    Q_PROPERTY_CREATE(qreal, ExpandMarkWidth)
    Q_PROPERTY_CREATE(bool, WordWrap)
public:
    explicit ElaComboBoxStyle(QStyle* style = nullptr);
    ~ElaComboBoxStyle();
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = nullptr) const override;

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;

public:
    mutable int maxItemHeight;

private:
    ElaThemeType::ThemeMode _themeMode;
    int _shadowBorderWidth{4};
    int calculateTextHeight(const QString &text, int width, const QFont &font) const;
};

#endif // ELACOMBOBOXSTYLE_H

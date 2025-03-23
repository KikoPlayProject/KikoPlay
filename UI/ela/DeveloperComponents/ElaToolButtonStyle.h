#ifndef ELATOOLBUTTONSTYLE_H
#define ELATOOLBUTTONSTYLE_H

#include <QProxyStyle>

#include "../Def.h"
class QStyleOptionToolButton;
class ElaSelfTheme;
class ElaToolButtonStyle : public QProxyStyle
{
    Q_OBJECT
    Q_PRIVATE_CREATE(bool, IsTransparent)
    Q_PRIVATE_CREATE(int, BorderRadius)
    Q_PROPERTY_CREATE(qreal, ExpandIconRotate)
    Q_PRIVATE_CREATE(bool, IsSelected);

public:
    explicit ElaToolButtonStyle(QStyle* style = nullptr);
    ~ElaToolButtonStyle();
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;
    void setSelfTheme(ElaSelfTheme *theme) { _selfTheme = theme; }

private:
    ElaThemeType::ThemeMode _themeMode;
    ElaSelfTheme *_selfTheme{nullptr};
    int _contentMargin{6};
    int _hasMenuIndicatorWidth{30};
    void _drawIndicator(QPainter* painter, const QStyleOptionToolButton* bopt, const QWidget* widget) const;
    void _drawIcon(QPainter* painter, QRectF iconRect, const QStyleOptionToolButton* bopt, const QWidget* widget) const;
    void _drawText(QPainter* painter, QRect contentRect, const QStyleOptionToolButton* bopt) const;
    qreal _calculateExpandIndicatorWidth(const QStyleOptionToolButton* bopt, QPainter* painter) const;
};

#endif // ELATOOLBUTTONSTYLE_H

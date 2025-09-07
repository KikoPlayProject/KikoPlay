#ifndef ELATHEME_H
#define ELATHEME_H

#include <QWidget>
#include <QColor>

#include "Def.h"
#include "singleton.h"
#include "stdafx.h"
#ifndef Q_OS_WIN
#include "private/ElaThemePrivate.h"
#endif
#define eTheme ElaTheme::getInstance()
#define ElaThemeColor(themeMode, themeColor) eTheme->getThemeColor(themeMode, ElaThemeType::themeColor)
#define ElaSelfThemeColor(selfThemePtr, themeMode, themeColor) (selfThemePtr && selfThemePtr->hasThemeColor(themeMode, ElaThemeType::themeColor) ? selfThemePtr->getThemeColor(themeMode, ElaThemeType::themeColor) : eTheme->getThemeColor(themeMode, ElaThemeType::themeColor))
class QPainter;
class ElaThemePrivate;
class ElaSelfThemePrivate;
class ElaTheme : public QWidget
{
    Q_OBJECT
    Q_Q_CREATE(ElaTheme)
    Q_SINGLETON_CREATE_H(ElaTheme)

    Q_PROPERTY(QColor primaryNormalLight READ getPrimaryNormalLight WRITE setPrimaryNormalLight)
    Q_PROPERTY(QColor primaryNormalDark READ getPrimaryNormalDark WRITE setPrimaryNormalDark)
    Q_PROPERTY(QColor primaryHoverLight READ getPrimaryHoverLight WRITE setPrimaryHoverLight)
    Q_PROPERTY(QColor primaryHoverDark READ getPrimaryHoverDark WRITE setPrimaryHoverDark)
    Q_PROPERTY(QColor primaryPressLight READ getPrimaryPressLight WRITE setPrimaryPressLight)
    Q_PROPERTY(QColor primaryPressDark READ getPrimaryPressDark WRITE setPrimaryPressDark)

public:
    QColor getPrimaryNormalLight() const { return getThemeColor(ElaThemeType::Light, ElaThemeType::PrimaryNormal); }
    QColor getPrimaryNormalDark() const { return getThemeColor(ElaThemeType::Dark, ElaThemeType::PrimaryNormal); }
    QColor getPrimaryHoverLight() const { return getThemeColor(ElaThemeType::Light, ElaThemeType::PrimaryHover); }
    QColor getPrimaryHoverDark() const { return getThemeColor(ElaThemeType::Dark, ElaThemeType::PrimaryHover); }
    QColor getPrimaryPressLight() const { return getThemeColor(ElaThemeType::Light, ElaThemeType::PrimaryPress); }
    QColor getPrimaryPressDark() const { return getThemeColor(ElaThemeType::Dark, ElaThemeType::PrimaryPress); }

    void setPrimaryNormalLight(const QColor& color) { setThemeColor(ElaThemeType::Light, ElaThemeType::PrimaryNormal, color); }
    void setPrimaryNormalDark(const QColor& color) { setThemeColor(ElaThemeType::Dark, ElaThemeType::PrimaryNormal, color); }
    void setPrimaryHoverLight(const QColor& color) { setThemeColor(ElaThemeType::Light, ElaThemeType::PrimaryHover, color); }
    void setPrimaryHoverDark(const QColor& color) { setThemeColor(ElaThemeType::Dark, ElaThemeType::PrimaryHover, color); }
    void setPrimaryPressLight(const QColor& color) { setThemeColor(ElaThemeType::Light, ElaThemeType::PrimaryPress, color); }
    void setPrimaryPressDark(const QColor& color) { setThemeColor(ElaThemeType::Dark, ElaThemeType::PrimaryPress, color); }

private:
    explicit ElaTheme(QWidget* parent = nullptr);
    ~ElaTheme();

public:
    void setThemeMode(ElaThemeType::ThemeMode themeMode);
    ElaThemeType::ThemeMode getThemeMode() const;

    void drawEffectShadow(QPainter* painter, QRect widgetRect, int shadowBorderWidth, int borderRadius);

    void setThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor, QColor newColor);
    const QColor& getThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor) const;
Q_SIGNALS:
    Q_SIGNAL void themeModeChanged(ElaThemeType::ThemeMode themeMode);
};

class ElaSelfTheme : public QObject
{
    Q_OBJECT
    Q_Q_CREATE(ElaSelfTheme)

public:
    explicit ElaSelfTheme(QObject* parent = nullptr);

    void setThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor, QColor newColor);
    bool hasThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor);
    const QColor& getThemeColor(ElaThemeType::ThemeMode themeMode, ElaThemeType::ThemeColor themeColor);
};

#endif // ELATHEME_H

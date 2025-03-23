#ifndef ELATHEMEPRIVATE_H
#define ELATHEMEPRIVATE_H

#include <QColor>
#include <QMap>
#include <QObject>

#include "../Def.h"
#include "../stdafx.h"
class ElaTheme;
class ElaSelfTheme;
class ElaThemePrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaTheme)
public:
    explicit ElaThemePrivate(QObject* parent = nullptr);
    ~ElaThemePrivate();

private:
    ElaThemeType::ThemeMode _themeMode{ElaThemeType::Dark};
    QColor _lightThemeColorList[40];
    QColor _darkThemeColorList[40];
    void _initThemeColor();
};

class ElaSelfThemePrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaSelfTheme)
public:
    explicit ElaSelfThemePrivate(QObject* parent = nullptr);

private:
    ElaThemeType::ThemeMode _themeMode{ElaThemeType::Light};
    QColor _lightThemeColorList[40];
    QColor _darkThemeColorList[40];
};

#endif // ELATHEMEPRIVATE_H

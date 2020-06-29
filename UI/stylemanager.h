#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H
#include <QtCore>
#include <QColor>

class StyleManager
{
public:
    enum StyleMode
    {
        NO_BG,
        DEFAULT_BG,
        BG_COLOR,
        UNKNOWN
    };
    StyleManager();
    StyleManager(const StyleManager &)=delete;
    void setQSS(StyleMode mode, const QColor &color=QColor());
private:
    QString normalQSS, bgQSS, defaultBgQSS;
    QString loadQSS(const QString &fn);

    QColor themeColor;
    StyleMode mode;
    QHash<QString, QString> colorHash;

    void setColorHash();
    QColor getColorPalette(const QColor &color, int index);
    QString setQSSColor(const QString &qss);
    inline QString toString(const QColor &color);

};

#endif // STYLEMANAGER_H

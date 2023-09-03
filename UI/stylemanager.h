#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H
#include <QtCore>
#include <QColor>

class StyleManager : public QObject
{
    Q_OBJECT
public:
    enum StyleMode
    {
        NO_BG,
        DEFAULT_BG,
        BG_COLOR,
        UNKNOWN
    };
    static StyleManager *getStyleManager();
    void setQSS(StyleMode mode, const QColor &color=QColor());
    QString setQSS(const QString &qss, const QVariantMap *extraVal = nullptr);
    void setCondVariable(const QString &name, bool val, bool refresh=true);
    bool getCondVariable(const QString &name) const {return condHash.value(name, false);}
    StyleMode currentMode() const {return mode;}
signals:
    void styleModelChanged(StyleMode newMode);
    void condVariableChanged(const QString &name, bool val);
private:
    StyleManager();
    StyleManager(const StyleManager &)=delete;

    QString normalQSS, bgQSS, defaultBgQSS;
    QString loadQSS(const QString &fn);

    QColor themeColor;
    StyleMode mode;
    QHash<QString, QString> colorHash;
    QHash<QString, bool> condHash;

    void setColorHash();
    QColor getColorPalette(const QColor &color, int index);
    inline QString toString(const QColor &color);

    void pushEvent();

};

#endif // STYLEMANAGER_H

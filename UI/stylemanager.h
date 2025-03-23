#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H
#include <QtCore>
#include <QColor>

class StyleManager : public QObject
{
    Q_OBJECT
public:
    static StyleManager *getStyleManager();
    void initStyle(bool hasBackground);
    void setThemeColor(const QColor &color);


    QString setQSS(const QString &qss, const QVariantMap *extraVal = nullptr) const;
    void setCondVariable(const QString &name, bool val, bool refresh=true);
    bool getCondVariable(const QString &name) const {return condHash.value(name, false);}
    bool enableThemeColor() const { return useThemeColor; }
    QColor curThemeColor() const { return themeColor; }
signals:
    void condVariableChanged(const QString &name, bool val);
private:
    StyleManager();
    StyleManager(const StyleManager &)=delete;

    QString qss;
    QString loadQSS(const QString &fn);

    bool useThemeColor;
    QColor themeColor;
    QHash<QString, QString> colorHash;
    QMap<QString, bool> condHash;

    void setColorHash();
    QColor getColorPalette(const QColor &color, int index);
    inline QString toString(const QColor &color);

    void pushEvent();
    void updateAppFont();

};

#endif // STYLEMANAGER_H

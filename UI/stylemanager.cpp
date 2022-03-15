#include "stylemanager.h"
#include <QFile>
#include <QApplication>
#include "globalobjects.h"

StyleManager::StyleManager():QObject(),mode(UNKNOWN)
{
    normalQSS = loadQSS(":/res/style.qss");
    bgQSS = loadQSS(":/res/style_bg.qss");
    defaultBgQSS = loadQSS(":/res/style_bg_default.qss");
}

StyleManager *StyleManager::getStyleManager()
{
    static StyleManager styleManager;
    return &styleManager;
}

void StyleManager::setQSS(StyleMode mode, const QColor &color)
{
    if(mode == this->mode && color == themeColor) return;
    emit styleModelChanged(mode);
    this->mode = mode;
    if(mode == StyleMode::BG_COLOR)
    {
        if(color.isValid())
        {
            themeColor = color;
        }
        setColorHash();
        qApp->setStyleSheet(setQSS(bgQSS));
    }
    else if(mode == StyleMode::DEFAULT_BG)
    {
        qApp->setStyleSheet(setQSS(defaultBgQSS));
    }
    else if(mode == StyleMode::NO_BG)
    {
        qApp->setStyleSheet(setQSS(normalQSS));
    }
}

void StyleManager::setCondVariable(const QString &name, bool val, bool refresh)
{
    if(condHash.contains(name) && condHash[name] == val) return;
    condHash[name] = val;
    emit condVariableChanged(name, val);
    if(!refresh) return;
    if(mode == StyleMode::BG_COLOR)
    {
        setColorHash();
        qApp->setStyleSheet(setQSS(bgQSS));
    }
    else if(mode == StyleMode::DEFAULT_BG)
    {
        qApp->setStyleSheet(setQSS(defaultBgQSS));
    }
    else if(mode == StyleMode::NO_BG)
    {
        qApp->setStyleSheet(setQSS(normalQSS));
    }
}

QString StyleManager::loadQSS(const QString &fn)
{
    QFile qss(fn);
    if(qss.open(QFile::ReadOnly))
    {
        return QLatin1String(qss.readAll());
    }
    return "";
}

void StyleManager::setColorHash()
{

    colorHash["ThemeColor"] = toString(themeColor);
    colorHash["ThemeColorL1"] = toString(getColorPalette(themeColor, 5));
    colorHash["ThemeColorL2"] = toString(getColorPalette(themeColor, 4));
    colorHash["ThemeColorL3"] = toString(getColorPalette(themeColor, 3));
    colorHash["ThemeColorL4"] = toString(getColorPalette(themeColor, 2));
    colorHash["ThemeColorL5"] = toString(getColorPalette(themeColor, 1));
    colorHash["ThemeColorD1"] = toString(getColorPalette(themeColor, 7));
    colorHash["ThemeColorD2"] = toString(getColorPalette(themeColor, 8));
    colorHash["ThemeColorD3"] = toString(getColorPalette(themeColor, 9));
    colorHash["ThemeColorD4"] = toString(getColorPalette(themeColor, 10));
    colorHash["ThemeColorD5"] = toString(getColorPalette(themeColor, 11));
}

QColor StyleManager::getColorPalette(const QColor &color, int index)
{
    auto getHue = [](int h, int i, bool lighter){
        int hue;
        if(h >=60 && h <= 240) hue = lighter?h-2*i:h+2*i;
        else hue = lighter?h+2*i:h-2*i;
        if(hue<0) hue+=360;
        else if(hue>=360) hue-=360;
        return hue;
    };
    auto getSaturation = [](int s, int i, bool lighter){
        int saturation;
        if(lighter) saturation = s - 40 * i;
        else saturation = s + 12 * i;
        return qBound(15, saturation, 255);
    };
    auto getValue = [](int v, int i, bool lighter){
        int value;
        if(lighter) value = v + 12*i;
        else value = v - 38*i;
        return qBound(15, value, 255);
    };
    bool lighter = index <= 6;
    QColor c = color.toHsv();
    int i  = lighter? 6 - index : index - 6;
    int h = getHue(c.hue(), i, lighter),
        s = getSaturation(c.saturation(), i, lighter),
        v = getValue(c.value(), i, lighter);
    return QColor::fromHsv(h, s, v);
}

QString StyleManager::setQSS(const QString &qss)
{
    QVector<QPair<QString, bool>> replacedStack{{"", true}};
    int state = 0;
    QString curCond, curName;
    for(const QChar c: qss)
    {
        if(state == 0)
        {
            if(c == '@')
            {
                state = 1;
                curName = "";
            }
            else replacedStack.back().first.append(c);
        }
        else if(state == 1)
        {
            if(c == ')' || c == ',' || c == '{' || c == '\n')
            {
                if(curName.trimmed() == "if")
                {
                    state = 2;
                    curCond = "";
                }
                else if(curName.trimmed() == "else")
                {
                    bool condSatisfy = replacedStack.back().second;
                    QString block = replacedStack.back().first;
                    replacedStack.pop_back();
                    if(condSatisfy)
                    {
                        assert(!replacedStack.empty());
                        replacedStack.back().first.append(block);
                        replacedStack.append({"", false});
                    }
                    else
                    {
                        replacedStack.append({"", true});
                    }
                    state = 0;
                }
                else if(curName.trimmed() == "endif")
                {
                    bool condSatisfy = replacedStack.back().second;
                    QString block = replacedStack.back().first;
                    replacedStack.pop_back();
                    if(condSatisfy)
                    {
                        assert(!replacedStack.empty());
                        replacedStack.back().first.append(block);
                    }
                    state = 0;
                }
                else
                {
                    replacedStack.back().first.append(colorHash.value(curName, curName));
                    replacedStack.back().first.append(c);
                    state = 0;
                }
            }
            else curName.append(c);
        }
        else if(state == 2)
        {
            if(c == '}')
            {
               bool cond = condHash.value(curCond, false);
               replacedStack.append({"", cond});
               state = 0;
            }
            else curCond.append(c);
        }
    }
    if(replacedStack.size() > 1) return "";
    return replacedStack.back().first;
}

QString StyleManager::toString(const QColor &color)
{
    return QString("%1,%2,%3").arg(color.red()).arg(color.green()).arg(color.blue());
}

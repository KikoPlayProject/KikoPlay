#include "stylemanager.h"
#include <QFile>
#include <QApplication>
#include <QVariant>
#include <QFont>
#include "Common/eventbus.h"
#include "globalobjects.h"

#define SETTING_KEY_STYLE_USE_THEMECOLOR "Style/UseThemeColor"
#define SETTING_KEY_STYLE_THEMECOLOR "Style/ThemeColor"


StyleManager::StyleManager():QObject()
{
    qss = loadQSS(":/res/style.qss");

    useThemeColor = GlobalObjects::appSetting->value(SETTING_KEY_STYLE_USE_THEMECOLOR, false).toBool();
    themeColor = GlobalObjects::appSetting->value(SETTING_KEY_STYLE_THEMECOLOR, QColor(28, 160, 228)).value<QColor>();
}

StyleManager *StyleManager::getStyleManager()
{
    static StyleManager styleManager;
    return &styleManager;
}

void StyleManager::initStyle(bool hasBackground)
{
    setCondVariable("useThemeColor", useThemeColor, false);
    setCondVariable("hasBackground", hasBackground, false);
    if (useThemeColor && themeColor.isValid())
    {
        setColorHash();
    }
    qApp->setStyleSheet(setQSS(qss));
}

void StyleManager::setThemeColor(const QColor &color)
{
    pushEvent();
    if (color.isValid())
    {
        useThemeColor = true;
        themeColor = color;
        setColorHash();
        GlobalObjects::appSetting->setValue(SETTING_KEY_STYLE_THEMECOLOR, themeColor);
    }
    else
    {
        useThemeColor = false;
    }
    setCondVariable("useThemeColor", useThemeColor, false);
    qApp->setStyleSheet(setQSS(qss));
    updateAppFont();
    GlobalObjects::appSetting->setValue(SETTING_KEY_STYLE_USE_THEMECOLOR, useThemeColor);
}

void StyleManager::setCondVariable(const QString &name, bool val, bool refresh)
{
    if (condHash.contains(name) && condHash[name] == val) return;
    condHash[name] = val;
    emit condVariableChanged(name, val);
    if (!refresh) return;
    qApp->setStyleSheet(setQSS(qss));
    updateAppFont();
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

QString StyleManager::setQSS(const QString &qss, const QVariantMap *extraVal) const
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
                    QString val = colorHash.value(curName, curName);
                    if (extraVal && extraVal->contains(curName))
                    {
                        val = (*extraVal)[curName].toString();
                    }
                    replacedStack.back().first.append(val);
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
               if (extraVal && extraVal->contains(curCond))
               {
                   cond = (*extraVal)[curCond].toBool();
               }
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

void StyleManager::pushEvent()
{
    if (!EventBus::getEventBus()->hasListener(EventBus::EVENT_APP_STYLE_CHANGED)) return;
    QVariantMap condsMap;
    for (auto iter = condHash.begin(); iter != condHash.end(); ++iter)
    {
        condsMap.insert(iter.key(), iter.value());
    }
    const QVariantMap param = {
        { "conds", condsMap },
        { "themeColor", useThemeColor ? themeColor.rgb() : -1}
    };
    EventBus::getEventBus()->pushEvent(EventParam{EventBus::EVENT_APP_STYLE_CHANGED, param});
}

void StyleManager::updateAppFont()
{
    QFont font = qApp->font();
    font.setPixelSize(13);
    font.setFamily(GlobalObjects::normalFont);
    font.setHintingPreference(QFont::PreferNoHinting);
    qApp->setFont(font);
}

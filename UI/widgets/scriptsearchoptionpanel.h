#ifndef SCRIPTSEARCHOPTIONPANEL_H
#define SCRIPTSEARCHOPTIONPANEL_H

#include <QWidget>
#include <QSharedPointer>
#include "Script/scriptbase.h"

class ScriptSearchOptionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ScriptSearchOptionPanel(QWidget *parent = nullptr);
    bool hasOptions() const;
    bool changed() const {return optionChanged;}
    void setScript(QSharedPointer<ScriptBase> s, const QMap<QString, QString> *overlayOptions=nullptr);
    QMap<QString, QString> getOptionVals() const;
    int getAllWidth() const { return allWidth; }
private:
    QSharedPointer<ScriptBase> script;
    bool optionChanged;
    int allWidth;
    const QMap<QString, QString> *overlayOptions;
    struct OptionItem
    {
        QString val;
        QSharedPointer<QWidget> widget;
        const ScriptBase::SearchSettingItem *searchSettingItem = nullptr;
    };
    QVector<OptionItem> optionItems;
    void makeItem(OptionItem &item);
    void makeTextItem(OptionItem &item);
    void makeComboItem(OptionItem &item);
    void makeRadioItem(OptionItem &item);
    void makeCheckItem(OptionItem &item);
    void makeCheckListItem(OptionItem &item);
};

#endif // SCRIPTSEARCHOPTIONPANEL_H

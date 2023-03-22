#ifndef MPVPAGE_H
#define MPVPAGE_H
#include "settingpage.h"
#include <QVector>
class QPlainTextEdit;
class QTabWidget;
class MPVPage : public SettingPage
{
    Q_OBJECT
public:
    MPVPage(QWidget *parent = nullptr);
    virtual void onAccept() override;
    virtual void onClose() override;
private:
    QTabWidget *tab;
    struct OptionGroup
    {
        QString groupKey;
        QPlainTextEdit *editor;
        bool changed;
    };
    QVector<OptionGroup> optionGroups;
    bool hasRemoved, currentRemoved;
    void loadOptions();
};
#endif // MPVPAGE_H

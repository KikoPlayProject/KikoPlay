#ifndef MPVCONFEDIROR_H
#define MPVCONFEDIROR_H
#include "UI/framelessdialog.h"
class QTabWidget;
class QPlainTextEdit;
class MPVConfEdiror : public CFramelessDialog
{
    Q_OBJECT
public:
    MPVConfEdiror(QWidget *parent = nullptr);

protected:
    virtual void onAccept();
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

#endif // MPVCONFEDIROR_H

#ifndef MPVPARAMETERSETTING_H
#define MPVPARAMETERSETTING_H

#include "framelessdialog.h"
class QTextEdit;
class MPVParameterSetting : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit MPVParameterSetting(QWidget *parent = nullptr);

    // CFramelessDialog interface
protected:
    virtual void onAccept();
private:
    QTextEdit *parameterEdit;
};

#endif // MPVPARAMETERSETTING_H

#ifndef ADDURITASK_H
#define ADDURITASK_H

#include "framelessdialog.h"
class DirSelectWidget;
class QPlainTextEdit;
class AddUriTask : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AddUriTask(QWidget *parent = nullptr);
    QString dir;
    QStringList uriList;

    // CFramelessDialog interface
private:
    DirSelectWidget *dirSelect;
    QPlainTextEdit *uriEdit;
protected:
    virtual void onAccept();
};

#endif // ADDURITASK_H

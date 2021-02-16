#ifndef DANMULAUNCH_H
#define DANMULAUNCH_H
#include "framelessdialog.h"
class ColorPicker;
class QComboBox;
class QListWidget;
class QLineEdit;
class DanmuLaunch : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit DanmuLaunch(QWidget *parent = nullptr);

    void show();

protected:
    virtual void onAccept();

private:
    ColorPicker *colorPicker;
    QComboBox *typeCombo, *fontSizeCombo;
    QLineEdit *textEdit;
    QListWidget *scriptList;
    int curTime;
    QString curPoolId;

    void setTime();

    // QDialog interface
public slots:
    virtual int exec();
};

#endif // DANMULAUNCH_H

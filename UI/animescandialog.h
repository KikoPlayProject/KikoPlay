#ifndef ANIMESCANDIALOG_H
#define ANIMESCANDIALOG_H

#include "framelessdialog.h"
#include "MediaLibrary/animescantask.h"

class QListWidget;
class QSpinBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QStackedLayout;
class QTextEdit;
class AnimeScanTask;
class MPVMediaInfo;
class KPlainTextEdit;
class ElidedLabel;

class AnimeScanDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit AnimeScanDialog(const QString &initialDir, QWidget *parent = nullptr);
    ~AnimeScanDialog();

private:
    QListWidget *dirListWidget;
    QCheckBox *enableDurationCheck;
    QSpinBox *minDurationSpin;
    QSpinBox *maxDurationSpin;
    KPlainTextEdit *filenameFilterEdit;
    ElidedLabel *progressTip;
    QPushButton *startBtn;
    QPushButton *addDirBtn;
    QPushButton *removeDirBtn;
    QTextEdit *resultView;
    QStackedLayout *stackedLayout;

    AnimeScanTask *task{nullptr};
    MPVMediaInfo *mediaInfo{nullptr};

    void addDirectory();
    void removeSelectedDirectory();
    void startScan();
    void setUIEnabled(bool enabled);
    void showResults(const AnimeScanResult &result);
};

#endif // ANIMESCANDIALOG_H

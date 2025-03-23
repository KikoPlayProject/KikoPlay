#ifndef DANMUPAGE_H
#define DANMUPAGE_H

#include "settingpage.h"
struct DanmuStyle;
class QLabel;

class DanmuPage : public SettingPage
{
    Q_OBJECT
public:
    DanmuPage(QWidget *parent = nullptr);

    virtual void onAccept();
    virtual void onClose();

private:
    QLabel *previewLabel{nullptr};
    void updateDanmuPreview(DanmuStyle *danmuStyle);

    SettingItemArea *initStyleArea();
    SettingItemArea *initMergeArea();
    SettingItemArea *initOtherArea();
};

#endif // DANMUPAGE_H

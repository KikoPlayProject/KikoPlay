#ifndef SELECTEPISODE_H
#define SELECTEPISODE_H

#include "framelessdialog.h"
#include "Play/Danmu/Provider/info.h"
class QTreeWidget;
class SelectEpisode : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit SelectEpisode(DanmuAccessResult *episodeResult,QWidget *parent = nullptr);

private:
    QTreeWidget *episodeWidget;
    DanmuAccessResult *episodeResult;
    bool autoSetDelay;
signals:

public slots:
    virtual void onAccept();
};

#endif // SELECTEPISODE_H

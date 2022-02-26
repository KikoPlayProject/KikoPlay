#ifndef SELECTEPISODE_H
#define SELECTEPISODE_H

#include "framelessdialog.h"
#include "Play/Danmu/common.h"
class QTreeWidget;
class SelectEpisode : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit SelectEpisode(QList<DanmuSource> &epResults,QWidget *parent = nullptr);

private:
    QTreeWidget *episodeWidget;
    QList<DanmuSource> &episodeResult;

public slots:
    virtual void onAccept();
};

#endif // SELECTEPISODE_H

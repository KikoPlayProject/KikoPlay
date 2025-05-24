#ifndef SELECTEPISODE_H
#define SELECTEPISODE_H

#include "UI/framelessdialog.h"
#include "Play/Danmu/common.h"
#include <QTreeWidget>
class PressTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    using QTreeWidget::QTreeWidget;
protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
};

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

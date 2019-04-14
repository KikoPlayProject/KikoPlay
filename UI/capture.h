#ifndef CAPTURE_H
#define CAPTURE_H

#include "framelessdialog.h"
class PlayListItem;
class Capture : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit Capture(QImage &captureImage, QWidget *parent = nullptr, const PlayListItem *item=nullptr);

signals:

public slots:
};

#endif // CAPTURE_H

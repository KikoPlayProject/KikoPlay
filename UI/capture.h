#ifndef CAPTURE_H
#define CAPTURE_H

#include "framelessdialog.h"
class PlayListItem;
class Capture : public CFramelessDialog
{
    Q_OBJECT
public:
    Capture(QImage &captureImage, QWidget *parent = nullptr, const PlayListItem *item=nullptr);

private:
    QWidget *buttonContainer;
    QLabel *imgLabel;
    double aspectRatio;
    // QWidget interface
protected:
    virtual void resizeEvent(QResizeEvent *event);
};

#endif // CAPTURE_H

#ifndef GIFCAPTURE_H
#define GIFCAPTURE_H
#include "framelessdialog.h"
class SimplePlayer;
class QPushButton;
class GIFCapture : public CFramelessDialog
{
    Q_OBJECT
public:
    GIFCapture(const QString &fileName = "", bool showDuration = false,  QWidget *parent = nullptr);
private:
    QSharedPointer<SimplePlayer> smPlayer;
    QPushButton *saveFile;
    bool ffmpegCut(const QString &input, const QString &output, int w, int h, int r = -1, double start = 0, double duration = -1);
};

#endif // GIFCAPTURE_H

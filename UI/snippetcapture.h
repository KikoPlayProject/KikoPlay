#ifndef SNIPPETCAPTURE_H
#define SNIPPETCAPTURE_H
#include "framelessdialog.h"
class SimplePlayer;
class QPushButton;
class SnippetCapture : public CFramelessDialog
{
    Q_OBJECT
public:
    SnippetCapture(QWidget *parent = nullptr);
    ~SnippetCapture();
private:
    SimplePlayer *smPlayer;
    QPushButton *saveFile, *addToLibrary;
    bool ffmpegCut(double start, const QString &input, int duration, bool retainAudio, const QString &out);
};

#endif // SNIPPETCAPTURE_H

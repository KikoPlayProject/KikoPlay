#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include "framelessdialog.h"

class MediaInfo : public CFramelessDialog
{
    Q_OBJECT
public:
    MediaInfo(QWidget *parent = nullptr);

private:
    struct TextBlock
    {
        QString text, blockVar;
        int start=0, end=1, step=1;
    };
    QString expandMediaInfo();
    void evalCommand(QList<QString> &commandStack, QList<TextBlock> &textStack);
};

#endif // MEDIAINFO_H

#ifndef CAPTURE_H
#define CAPTURE_H

#include "framelessdialog.h"

class Capture : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit Capture(QImage &captureImage, QWidget *parent = nullptr, const QString &animeTitle="");

signals:

public slots:
};

#endif // CAPTURE_H

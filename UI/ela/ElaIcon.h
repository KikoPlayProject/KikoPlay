#ifndef ELAICON_H
#define ELAICON_H
#include <QIcon>

#include "Def.h"
#include "singleton.h"
#include "stdafx.h"
class ElaIcon
{
    Q_SINGLETON_CREATE_H(ElaIcon)
private:
    explicit ElaIcon();
    ~ElaIcon();

public:
    QIcon getElaIcon(ElaIconType::IconName awesome);
    QIcon getElaIcon(ElaIconType::IconName awesome, QColor iconColor);
    QIcon getElaIcon(ElaIconType::IconName awesome, int pixelSize);
    QIcon getElaIcon(ElaIconType::IconName awesome, int pixelSize, QColor iconColor);
    QIcon getElaIcon(ElaIconType::IconName awesome, int pixelSize, int fixedWidth, int fixedHeight);
    QIcon getElaIcon(ElaIconType::IconName awesome, int pixelSize, int fixedWidth, int fixedHeight, QColor iconColor);
};

#endif // ELAICON_H

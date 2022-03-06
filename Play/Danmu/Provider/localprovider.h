#ifndef LOCALPROVIDER_H
#define LOCALPROVIDER_H
#include "Play/Danmu/common.h"
class LocalProvider
{
public:
    static void LoadXmlDanmuFile(QString filePath, QVector<DanmuComment *> &list);
};

#endif // LOCALPROVIDER_H

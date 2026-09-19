#ifndef LOCALPROVIDER_H
#define LOCALPROVIDER_H
#include "Play/Danmu/common.h"
class LocalProvider
{
public:
    static void LoadXmlDanmuFile(QString filePath, QVector<QPair<DanmuSource, QVector<DanmuComment *>>> &srcDanmus, bool forceIgnoreSrc=false);
    static void LoadSubFile(QString filePath, QVector<DanmuComment *> &list);

    static bool loadSrcInfo();
    static void setLoadSrcInfo(bool on);
};

#endif // LOCALPROVIDER_H

#ifndef SUBTITLELOADER_H
#define SUBTITLELOADER_H

#include "subitem.h"
#include <QObject>

class SubtitleLoader : public QObject
{
    Q_OBJECT
public:
    explicit SubtitleLoader(QObject *parent = nullptr);

    SubFile loadVideoSub(const QString &path, int trackIndex, SubFileFormat format);
    SubFile loadSubFile(const QString &path);

private:
    QString extract(const QString &path, int trackIndex, SubFileFormat format);

    qint64 timeStringToMs(const QString& timeStr, int msUnit = 1);

    SubFile loadSRT(const QString &path);
    SubFile loadASS(const QString &path);

    QString parseSRTText(const QString &text);
    QString parseASSText(const QString &text);
    void mergeSubItems(QList<SubItem> &items);

};

#endif // SUBTITLELOADER_H

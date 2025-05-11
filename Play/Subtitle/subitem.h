#ifndef SUBITEM_H
#define SUBITEM_H
#include <QString>
#include <QList>

struct SubItem
{
    qint64 startTime = 0;  // ms
    qint64 endTime = 0;    // ms
    QString text;

    static QString encodeTime(qint64 time, QChar msSplit = '.');
    static qint64 timeStringToMs(const QString &timeStr, int msUnit = 1);
};

enum class SubFileFormat
{
    F_SRT,
    F_ASS,
};

struct SubFile
{
    QString rawData;
    SubFileFormat format;
    QList<SubItem> items;

    bool saveSRT(const QString &path);
};

#endif // SUBITEM_H

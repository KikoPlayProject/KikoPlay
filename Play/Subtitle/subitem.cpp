#include "subitem.h"
#include <QRegularExpression>
#include <QFile>

QString SubItem::encodeTime(qint64 time, QChar msSplit)
{
    int ms = time % 1000;
    int hh = time / 1000 / 3600;
    int mm = (time / 1000 - hh * 3600) / 60;
    int ss = time / 1000 - hh * 3600 - mm * 60;
    QString format = QString("%1:%2:%3") + msSplit + "%4";
    return format.arg(hh, 2, 10, QChar('0')).arg(mm, 2, 10, QChar('0')).arg(ss, 2, 10, QChar('0')).arg(ms);
}

qint64 SubItem::timeStringToMs(const QString &timeStr, int msUnit)
{
    static QRegularExpression splitReg = QRegularExpression("[\\.:,]");
    QStringList parts = timeStr.split(splitReg);
    qint64 ms = 0;
    if (parts.size() >= 3)
    {
        int hours = parts[0].toInt();
        int minutes = parts[1].toInt();
        int seconds = parts[2].toInt();
        ms = hours * 3600000 + minutes * 60000 + seconds * 1000;
    }
    if (parts.size() >= 4)
    {
        int milliseconds = parts[3].toInt();
        ms += milliseconds * msUnit;
    }
    return ms;
}

bool SubFile::saveSRT(const QString &path)
{
    QFile subFile(path);
    if (!subFile.open(QIODevice::WriteOnly|QIODevice::Text)) return false;

    QTextStream out(&subFile);
    int index = 1;
    for (const SubItem &item : items)
    {
        out << index << "\n";
        out << item.encodeTime(item.startTime) << " --> "<< item.encodeTime(item.endTime) << "\n";
        out << item.text << "\n\n";
        ++index;
    }

    return true;
}

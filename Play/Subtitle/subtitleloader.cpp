#include "subtitleloader.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QSettings>
#include <QProcess>
#include <QEventLoop>
#include "globalobjects.h"
#include "Common/logger.h"

SubtitleLoader::SubtitleLoader(QObject *parent) : QObject{parent}
{

}

SubFile SubtitleLoader::loadVideoSub(const QString &path, int trackIndex, SubFileFormat format)
{
    QString subPath = extract(path, trackIndex, format);

    if (subPath.isEmpty()) return SubFile();

    SubFile res;
    switch (format)
    {
    case SubFileFormat::F_ASS:
        res = loadASS(subPath);
        break;
    case SubFileFormat::F_SRT:
        res = loadSRT(subPath);
        break;
    }
    QFile::remove(subPath);
    return res;
}

SubFile SubtitleLoader::loadSubFile(const QString &path)
{
    if (path.endsWith("srt"))
    {
        return loadSRT(path);
    }
    else if (path.endsWith("ass"))
    {
        return loadASS(path);
    }
    return SubFile();
}

QString SubtitleLoader::extract(const QString &path, int trackIndex, SubFileFormat format)
{
    QString ffmpegPath = GlobalObjects::appSetting->value("Play/FFmpeg", "ffmpeg").toString();
    QStringList arguments;
    arguments << "-i" << path << "-y";
    arguments << "-map" << QString("0:%1").arg(trackIndex);

    QString outputPath = QString("%1/sub_track_tmp").arg(GlobalObjects::context()->dataPath);
    switch (format)
    {
    case SubFileFormat::F_ASS:
        outputPath += ".ass";
        break;
    case SubFileFormat::F_SRT:
        outputPath += ".srt";
        break;
    }
    arguments << outputPath;

    if (QFile::exists(outputPath)) QFile::remove(outputPath);

    QProcess ffmpegProcess;
    QEventLoop eventLoop;
    bool success = true;
    QObject::connect(&ffmpegProcess, &QProcess::errorOccurred, this, [&eventLoop, &success](QProcess::ProcessError error){
        if(error == QProcess::FailedToStart)
        {
            Logger::logger()->log(Logger::APP, tr("Start FFmpeg Failed"));
        }
        success = false;
        eventLoop.quit();
    });
    QObject::connect(&ffmpegProcess, (void (QProcess:: *)(int, QProcess::ExitStatus))&QProcess::finished, this,
                     [&eventLoop, &success](int exitCode, QProcess::ExitStatus exitStatus){
         success = (exitStatus == QProcess::NormalExit && exitCode == 0);
         if (!success)
         {
             Logger::logger()->log(Logger::APP, tr("Sub extract Failed, FFmpeg exit code: %1").arg(exitCode));
         }
         eventLoop.quit();
     });
    QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardOutput, this, [&](){
        QString content(ffmpegProcess.readAllStandardOutput());
        Logger::logger()->log(Logger::APP, content);
    });
    QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardError, this, [&]() {
        QString content(ffmpegProcess.readAllStandardError());
        Logger::logger()->log(Logger::APP, content);
    });

    QTimer::singleShot(0, [&]() {
        ffmpegProcess.start(ffmpegPath, arguments);
    });
    eventLoop.exec();

    return success ? outputPath : "";
}

qint64 SubtitleLoader::timeStringToMs(const QString &timeStr, int msUnit)
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

SubFile SubtitleLoader::loadSRT(const QString &path)
{
    SubFile res;
    res.format = SubFileFormat::F_SRT;
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return res;
    }

    QTextStream in(&file);

    int status = 0;   // -- 0: begin, 1: index 2: time parsed, 3: content
    SubItem curItem;
    static QRegularExpression timeRegex("(\\d+:\\d+:\\d+.\\d+)\\s+-->\\s+(\\d+:\\d+:\\d+.\\d+)");

    while (!in.atEnd())
    {
        QString line = in.readLine();
        res.rawData += line + '\n';
        if (line.isEmpty())
        {
            if (status == 3)
            {
                if (!curItem.text.isEmpty())
                {
                    res.items.append(curItem);
                }
                curItem = SubItem();
                status = 0;
            }
            continue;
        }
        switch (status)
        {
        case 0:
            status = 1;
            break;
        case 1:
        {
            QRegularExpressionMatch match = timeRegex.match(line);
            if (match.hasMatch())
            {
                curItem.startTime = timeStringToMs(match.captured(1));
                curItem.endTime = timeStringToMs(match.captured(2));
                status = 2;
            }
            break;
        }
        case 2:
            curItem.text = parseSRTText(line);
            status = 3;
            break;
        case 3:
            curItem.text += "\n" + parseSRTText(line);
            break;
        default:
            break;
        }
    }

    if (status == 3)
    {
        if (!curItem.text.isEmpty())
        {
            res.items.append(curItem);
        }
    }

    file.close();
    std::stable_sort(res.items.begin(), res.items.end(), [](const SubItem &item1, const SubItem &item2){
        return item1.startTime < item2.startTime;
    });
    mergeSubItems(res.items);


    return res;
}

SubFile SubtitleLoader::loadASS(const QString &path)
{
    SubFile res;
    res.format = SubFileFormat::F_ASS;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return res;
    }

    QTextStream in(&file);
    QString line;
    bool isEventsSection = false;

    int startTimeIndex = 2;
    int endTimeIndex = 3;
    int textIndex = 10;

    while (!in.atEnd())
    {
        line = in.readLine();
        res.rawData += line + '\n';
        if (line.startsWith("[Events]"))
        {
            isEventsSection = true;
            continue;
        }

        if (isEventsSection)
        {
            if (line.startsWith("Format:"))
            {
                // Format: Marked,Layer,Start,End,Style,Name,MarginL,MarginR,MarginV,Effect,Text
                QStringList fields = line.mid(7).split(",");
                for (int i = 0; i < fields.size(); ++i)
                {
                    const QString fieldName = fields[i].trimmed().toLower();
                    if (fieldName == "start")
                    {
                        startTimeIndex = i;
                    }
                    else if (fieldName == "end")
                    {
                        endTimeIndex = i;
                    }
                    else if (fieldName == "text")
                    {
                        textIndex = i;
                    }
                }
            }
            else if (line.startsWith("Dialogue:"))
            {
                QStringList parts = line.mid(9).split(',');
                if (parts.size() < startTimeIndex || parts.size() < endTimeIndex || parts.size() < textIndex) continue;
                SubItem item;
                item.startTime = timeStringToMs(parts[startTimeIndex], 10);
                item.endTime = timeStringToMs(parts[endTimeIndex], 10);
                item.text = parseASSText(parts.mid(textIndex).join(','));
                if (!item.text.isEmpty())
                {
                    res.items.append(item);

                }
            }
        }
    }

    file.close();
    std::stable_sort(res.items.begin(), res.items.end(), [](const SubItem &item1, const SubItem &item2){
        return item1.startTime < item2.startTime;
    });
    mergeSubItems(res.items);
    return res;
}

QString SubtitleLoader::parseSRTText(const QString &text)
{
    QString res;
    bool inLabel = false;
    for (int i = 0; i < text.size(); ++i)
    {
        if (text[i] == '<')
        {
            inLabel = true;
        }
        else if (text[i] == '>')
        {
            inLabel = false;
        }
        else
        {
            if (!inLabel) res.append(text[i]);
        }
    }
    return res;
}

QString SubtitleLoader::parseASSText(const QString &text)
{
    QString res;
    static QRegularExpression paintStyleRegExp{"p\\d+"};
    bool inPaintMode = false;
    for (int i = 0; i < text.size(); ++i)
    {
        if (text[i] == '{')
        {
            QStringList styleList;
            QString curStyle;
            for (int j = i + 1; j < text.size(); ++j)
            {
                if (text[j] == '\\')
                {
                    if (!curStyle.isEmpty())
                    {
                        styleList.append(curStyle);
                        curStyle.clear();
                    }
                }
                else if (text[j] == '}')
                {
                    i = j;
                    break;
                }
                else
                {
                    curStyle.append(text[j]);
                }
            }
            if (!curStyle.isEmpty())
            {
                styleList.append(curStyle);
            }
            for (const QString &style : styleList)
            {
                if (style == "p0")
                {
                    inPaintMode = false;
                }
                else
                {
                    auto match = paintStyleRegExp.match(style);
                    if (match.hasMatch() && match.capturedStart(0) == 0 && match.capturedLength(0) == style.length())
                    {
                        inPaintMode = true;
                    }
                }
            }
        }
        else if (text[i] == '\\')
        {
            if (i + 1 < text.size())
            {
                QChar next = text[i+1];
                if (next == 'n' || next == 'N')
                {
                    res.append('\n');
                    ++i;
                }
                else if (next == 'h')
                {
                    res.append(' ');
                    ++i;
                }
            }
        }
        else
        {
            if (!inPaintMode)
            {
                res.append(text[i]);
            }
        }
    }
    return res;
}

void SubtitleLoader::mergeSubItems(QList<SubItem> &items)
{
    QList<SubItem> mergedSub;
    for(const SubItem &item : items)
    {
        if (!mergedSub.empty())
        {
            SubItem &last = mergedSub.back();
            if (last.startTime == item.startTime && last.endTime == item.endTime)
            {
                last.text += "\n" + item.text;
            }
            else
            {
                mergedSub.append(item);
            }
        }
        else
        {
            mergedSub.append(item);
        }
    }
    items.swap(mergedSub);
}

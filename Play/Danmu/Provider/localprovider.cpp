#include "localprovider.h"
#include "Common/htmlparsersax.h"
#include "Common/network.h"
#include "Play/Subtitle/subtitleloader.h"
#include "globalobjects.h"
#include <QRegularExpression>
#include <limits>

#define SETTING_KEY_IMPORT_SRC_INFO "Play/XMLImportSrcInfo"


namespace
{
    bool splitXmlDanmuSources(const QByteArray &content, const QVector<DanmuComment *> &list,
                             QVector<QPair<DanmuSource, QVector<DanmuComment *>>> &sources)
    {
        // HTMLParserSax does not support underscores in element names.
        QXmlStreamReader reader(content);
        QByteArray sourceData;
        while (!reader.atEnd())
        {
            reader.readNext();
            if (reader.isStartElement() && reader.name() == QLatin1String("kikoplay_src"))
            {
                sourceData = reader.readElementText().toLatin1();
                break;
            }
        }
        if (reader.hasError() || sourceData.isEmpty()) return false;

        QByteArray data;
        if (Network::gzipDecompress(QByteArray::fromBase64(sourceData), data) != 0) return false;
        QDataStream stream(data);
        int version = 0;
        stream >> version;
        if (stream.status() != QDataStream::Ok || !GlobalObjects::isValidKikoVersion(version)) return false;

        decltype(list.size()) nextComment = 0;
        while (!stream.atEnd())
        {
            int marker = 0, start = 0;
            // Match the QVector::size() type written by Pool::exportPool.
            decltype(list.size()) count = 0;
            bool useTimeline = false;
            stream >> marker >> start >> count >> useTimeline;
            if (stream.status() != QDataStream::Ok || marker != 0x23 || start != nextComment ||
                count < 0 || count > list.size() - nextComment)
            {
                return false;
            }

            QByteArray sourceJson;
            stream >> sourceJson;
            if (stream.status() != QDataStream::Ok) return false;
            QJsonParseError jsonError;
            const QJsonDocument sourceDoc = QJsonDocument::fromJson(sourceJson, &jsonError);
            if (jsonError.error != QJsonParseError::NoError || !sourceDoc.isObject()) return false;

            QPair<DanmuSource, QVector<DanmuComment *>> sourceDanmus;
            sourceDanmus.first.fromMap(sourceDoc.object().toVariantMap());
            sourceDanmus.second.reserve(count);
            for (auto i = nextComment; i < nextComment + count; ++i)
            {
                DanmuComment *danmu = list[i];
                if (!danmu) continue;
                if (useTimeline)
                {
                    QVector<int> originTimes = sourceDanmus.first.unmapTime(danmu->time);
                    // XML export rounds to 0.01 s, possibly into a timeline gap or outside the clip.
                    for (int delta = 1; originTimes.isEmpty() && delta <= 5; ++delta)
                    {
                        const qint64 before = qint64(danmu->time) - delta;
                        const qint64 after = qint64(danmu->time) + delta;
                        if (before >= std::numeric_limits<int>::min())
                            originTimes = sourceDanmus.first.unmapTime(int(before));
                        if (originTimes.isEmpty() && after <= std::numeric_limits<int>::max())
                            originTimes = sourceDanmus.first.unmapTime(int(after));
                    }
                    if (originTimes.isEmpty()) return false;
                    // A negative timeline offset can have multiple inverses; keep one comment.
                    danmu->originTime = originTimes.first();
                }
                danmu->source = sourceDanmus.first.id;
                sourceDanmus.second.append(danmu);
            }
            sourceDanmus.first.count = sourceDanmus.second.size();
            sources.append(sourceDanmus);
            nextComment += count;
        }
        return !sources.isEmpty() && nextComment == list.size();
    }

    // ASS color "&H[AA]BBGGRR&" -> RGB int (0xBBGGRR as DanmuComment::color).
    // ASS stores color in BGR order; DanmuComment::color is 0xRRGGBB, so reorder.
    int assColorToRGB(const QString &colorStr)
    {
        QString hex = colorStr;
        hex.remove('&').remove('H').remove('h').remove(';');
        // ASS color is &H[AA]BBGGRR& -> last 6 hex digits are BBGGRR (may be shorter)
        bool ok = false;
        unsigned int val = hex.toUInt(&ok, 16);
        if (!ok) return 0xFFFFFF;
        // strip alpha byte if present (8+ digits)
        val &= 0xFFFFFF;
        int b = (val >> 16) & 0xFF;
        int g = (val >> 8) & 0xFF;
        int r = val & 0xFF;
        return (r << 16) | (g << 8) | b;
    }

    // Map an ASS alignment (numpad 1-9) to a danmu type.
    // 1/2/3 bottom-left/center/right -> Bottom
    // 7/8/9 top-left/center/right    -> Top
    // 4/5/6 middle                   -> Bottom (no middle danmu; fall back)
    DanmuComment::DanmuType assAlignToType(int align)
    {
        if (align >= 7 && align <= 9) return DanmuComment::Top;
        return DanmuComment::Bottom; // 1-6
    }

    // Map absolute font size to Small/Normal/Large relative to the default size.
    DanmuComment::FontSizeLevel mapFontSize(int size, int baseSize)
    {
        if (baseSize <= 0) baseSize = size;
        double ratio = (double)size / baseSize;
        if (ratio < 0.8) return DanmuComment::Small;
        if (ratio > 1.2) return DanmuComment::Large;
        return DanmuComment::Normal;
    }

    qint64 timeStringToMs(const QString &timeStr, int msUnit = 1)
    {
        static QRegularExpression splitReg("[\\.:,]");
        QStringList parts = timeStr.split(splitReg);
        qint64 ms = 0;
        if (parts.size() >= 3)
        {
            ms = parts[0].toLongLong() * 3600000 + parts[1].toLongLong() * 60000 + parts[2].toLongLong() * 1000;
        }
        if (parts.size() >= 4)
        {
            ms += parts[3].toLongLong() * msUnit;
        }
        return ms;
    }
}

void LocalProvider::LoadXmlDanmuFile(QString filePath, QVector<QPair<DanmuSource, QVector<DanmuComment *>>> &srcDanmus, bool forceIgnoreSrc)
{
    QFile xmlFile(filePath);
    bool ret = xmlFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if (!ret) return;

    const QByteArray content = xmlFile.readAll();

    QVector<DanmuComment *> list;
    HTMLParserSax parser(content);
    while (!parser.atEnd())
    {
        if (parser.isStartNode() && parser.currentNode() == "d")
        {
            // Source ranges count every <d>, including empty or invalid comments.
            list.append(nullptr);
            const QByteArray attr = parser.currentNodeProperty("p");
            if (!attr.isEmpty())
            {
                auto attrList = attr.split(',');
                if (attrList.length() > 4)
                {
                    const QByteArray text = parser.readContentText();
                    QByteArray danmuText;
                    for(char ch : text)
                    {
                        if ((ch>=0x0 && ch<=0x8) || (ch>=0xb && ch<=0xc) || (ch>=0xe && ch<=0x1f))
                            continue;
                        danmuText.append(ch);
                    }
                    if (danmuText.isEmpty())
                    {
                        parser.readNext();
                        continue;
                    }

                    DanmuComment *danmu=new DanmuComment();
                    danmu->text = danmuText;
                    danmu->time = qRound(attrList[0].toDouble() * 1000);
                    danmu->originTime=danmu->time;
                    int mode = attrList[1].toInt();
                    DanmuComment::DanmuType type = DanmuComment::Rolling;
                    if (mode==4) type = DanmuComment::Bottom;
                    else if (mode==5) type = DanmuComment::Top;
                    danmu->type =type;
                    danmu->color=attrList[3].toInt();
                    if(attrList.length()>4)
                        danmu->date=attrList[4].toLongLong();
                    if(attrList.length()>6)
                        danmu->sender=attrList[6];
                    switch (attrList[2].toInt())
                    {
                    case 25:
                        danmu->fontSizeLevel=DanmuComment::Normal;
                        break;
                    case 18:
                        danmu->fontSizeLevel=DanmuComment::Small;
                        break;
                    case 36:
                        danmu->fontSizeLevel=DanmuComment::Large;
                        break;
                    default:
                        danmu->fontSizeLevel=DanmuComment::Normal;
                        break;
                    }
                    list.last() = danmu;
                }
            }
        }
        parser.readNext();
    }
    xmlFile.close();

    QVector<QPair<DanmuSource, QVector<DanmuComment *>>> sources;
    bool loadSrc = LocalProvider::loadSrcInfo();
    if (forceIgnoreSrc) loadSrc = false;
    if (loadSrc && splitXmlDanmuSources(content, list, sources))
    {
        srcDanmus = std::move(sources);
        return;
    }

    QPair<DanmuSource, QVector<DanmuComment *>> fileDanmus;
    const QFileInfo fileInfo(filePath);
    fileDanmus.first.title = fileInfo.fileName();
    fileDanmus.first.scriptData = fileInfo.filePath();
    fileDanmus.second.reserve(list.size());
    for (DanmuComment *danmu : list)
    {
        if (!danmu) continue;
        // Also undo any partial restoration if the embedded source data was invalid.
        danmu->originTime = danmu->time;
        danmu->source = fileDanmus.first.id;
        fileDanmus.second.append(danmu);
    }
    fileDanmus.first.count = fileDanmus.second.size();
    srcDanmus.append(fileDanmus);
}

void LocalProvider::LoadSubFile(QString filePath, QVector<DanmuComment *> &list)
{
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    if (suffix == "srt")
    {
        // SRT has no styling; reuse the existing parser which yields plain text.
        SubtitleLoader loader;
        SubFile sub = loader.loadSubFile(filePath);
        for (const SubItem &item : sub.items)
        {
            if (item.text.isEmpty()) continue;
            DanmuComment *danmu = new DanmuComment();
            danmu->text = item.text;
            danmu->time = item.startTime;
            danmu->originTime = item.startTime;
            danmu->type = DanmuComment::Bottom;
            danmu->color = 0xFFFFFF;
            danmu->fontSizeLevel = DanmuComment::Normal;
            danmu->date = 0;
            list.append(danmu);
        }
        return;
    }

    if (suffix != "ass" && suffix != "ssa") return;

    // ASS/SSA: self-parse to retain color/fontsize/alignment from styles & overrides.
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);

    struct AssStyle
    {
        int fontsize = 0;
        int primaryColor = 0xFFFFFF;
        int alignment = 2; // default bottom-center
    };
    QHash<QString, AssStyle> styleTable;
    AssStyle defaultStyle;
    defaultStyle.fontsize = 0; // resolved from "Default" style later

    enum Section { None, Styles, Events } section = None;

    // Field indices for Style/Dialogue lines (parsed from Format: lines)
    int stNameIdx = 0, stFontIdx = 1, stColorIdx = 3, stAlignIdx = -1;
    int evStartIdx = 1, evEndIdx = 2, evStyleIdx = 3, evTextIdx = 7;

    QString line;
    while (!in.atEnd())
    {
        line = in.readLine();

        QString trimmed = line.trimmed();
        if (trimmed.startsWith('[') && trimmed.endsWith(']'))
        {
            QString name = trimmed.mid(1, trimmed.length() - 2).toLower();
            if (name.startsWith("v4+ styles") || name.startsWith("v4 styles") || name == "styles")
                section = Styles;
            else if (name == "events")
                section = Events;
            else
                section = None;
            continue;
        }

        if (section == Styles)
        {
            if (trimmed.startsWith("Format:", Qt::CaseInsensitive))
            {
                QStringList fields = trimmed.mid(7).split(',');
                for (int i = 0; i < fields.size(); ++i)
                {
                    QString f = fields[i].trimmed().toLower();
                    if (f == "name") stNameIdx = i;
                    else if (f == "fontname" || f == "font") {}
                    else if (f == "fontsize") stFontIdx = i;
                    else if (f == "primarycolour") stColorIdx = i;
                    else if (f == "alignment") stAlignIdx = i;
                }
            }
            else if (trimmed.startsWith("Style:", Qt::CaseInsensitive))
            {
                QStringList parts = trimmed.mid(6).split(',');
                if (parts.size() <= qMax(qMax(stNameIdx, stFontIdx), qMax(stColorIdx, stAlignIdx))) continue;
                AssStyle st;
                st.fontsize = parts[stFontIdx].trimmed().toInt();
                st.primaryColor = assColorToRGB(parts[stColorIdx].trimmed());
                if (stAlignIdx >= 0) st.alignment = parts[stAlignIdx].trimmed().toInt();
                QString name = parts[stNameIdx].trimmed();
                styleTable.insert(name, st);
                if (name.compare("Default", Qt::CaseInsensitive) == 0 && defaultStyle.fontsize == 0)
                    defaultStyle = st;
            }
        }
        else if (section == Events)
        {
            if (trimmed.startsWith("Format:", Qt::CaseInsensitive))
            {
                QStringList fields = trimmed.mid(7).split(',');
                for (int i = 0; i < fields.size(); ++i)
                {
                    QString f = fields[i].trimmed().toLower();
                    if (f == "start") evStartIdx = i;
                    else if (f == "end") evEndIdx = i;
                    else if (f == "style") evStyleIdx = i;
                    else if (f == "text") evTextIdx = i;
                }
            }
            else if (trimmed.startsWith("Dialogue:", Qt::CaseInsensitive))
            {
                QStringList parts = trimmed.mid(9).split(',');
                if (parts.size() <= qMax(qMax(evStartIdx, evEndIdx), qMax(evStyleIdx, evTextIdx))) continue;
                qint64 startMs = timeStringToMs(parts[evStartIdx].trimmed(), 10); // ASS uses centiseconds

                // Base style lookup
                AssStyle base = defaultStyle;
                QString styleName = parts[evStyleIdx].trimmed();
                auto it = styleTable.find(styleName);
                if (it != styleTable.end()) base = it.value();
                int baseFontSize = (defaultStyle.fontsize > 0) ? defaultStyle.fontsize
                                                                : (base.fontsize > 0 ? base.fontsize : 0);

                // Text is everything after the text column (may contain commas).
                QString text = parts.mid(evTextIdx).join(',');

                // Parse override blocks: extract \an \c/\1c \fs, strip tags, handle \n \N \h.
                DanmuComment::DanmuType type = assAlignToType(base.alignment);
                int color = base.primaryColor;
                int fontSize = base.fontsize;

                QString outText;
                static QRegularExpression paintRe("^p\\d+$");
                bool inPaint = false;
                int i = 0;
                while (i < text.size())
                {
                    QChar c = text.at(i);
                    if (c == '{')
                    {
                        // collect override tags until '}'
                        QStringList tags;
                        QString cur;
                        int j = i + 1;
                        for (; j < text.size(); ++j)
                        {
                            QChar cj = text.at(j);
                            if (cj == '\\')
                            {
                                if (!cur.isEmpty()) { tags.append(cur); cur.clear(); }
                            }
                            else if (cj == '}')
                            {
                                break;
                            }
                            else
                            {
                                cur.append(cj);
                            }
                        }
                        if (!cur.isEmpty()) tags.append(cur);
                        i = (j < text.size()) ? j + 1 : j; // skip past '}'

                        for (const QString &tag : tags)
                        {
                            if (tag == "p0") { inPaint = false; continue; }
                            if (paintRe.match(tag).hasMatch()) { inPaint = true; continue; }
                            if (tag.startsWith("an"))
                            {
                                bool ok = false;
                                int n = tag.mid(2).toInt(&ok);
                                if (ok) type = assAlignToType(n);
                            }
                            else if (tag.startsWith("1c&") || tag.startsWith("c&"))
                            {
                                int p = tag.startsWith("1c") ? 2 : 1;
                                color = assColorToRGB(tag.mid(p));
                            }
                            else if (tag.startsWith("fs"))
                            {
                                bool ok = false;
                                int s = tag.mid(2).toInt(&ok);
                                if (ok) fontSize = s;
                            }
                            // \n \N handled as hard/soft newline outside the block;
                            // but they can also appear as tags here.
                            else if (tag == "n" || tag == "N")
                            {
                                outText.append('\n');
                            }
                            else if (tag == "h")
                            {
                                outText.append(' ');
                            }
                            // other tags (b, i, fn, bord, pos, move, fad, k, ...) -> strip
                        }
                    }
                    else if (c == '\\')
                    {
                        // bare \n \N \h outside override blocks
                        if (i + 1 < text.size())
                        {
                            QChar next = text.at(i + 1);
                            if (next == 'n' || next == 'N') { outText.append('\n'); i += 2; continue; }
                            if (next == 'h') { outText.append(' '); i += 2; continue; }
                        }
                        ++i;
                    }
                    else
                    {
                        if (!inPaint) outText.append(c);
                        ++i;
                    }
                }

                if (outText.trimmed().isEmpty()) continue;

                DanmuComment *danmu = new DanmuComment();
                danmu->text = outText;
                danmu->time = startMs;
                danmu->originTime = startMs;
                danmu->type = type;
                danmu->color = (color == 0) ? 0xFFFFFF : color;
                danmu->fontSizeLevel = mapFontSize(fontSize, baseFontSize);
                danmu->date = 0;
                list.append(danmu);
            }
        }
    }
    file.close();
}

bool LocalProvider::loadSrcInfo()
{
    return GlobalObjects::appSetting->value(SETTING_KEY_IMPORT_SRC_INFO, true).toBool();
}

void LocalProvider::setLoadSrcInfo(bool on)
{
    GlobalObjects::appSetting->setValue(SETTING_KEY_IMPORT_SRC_INFO, on);
}

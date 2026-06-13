#include "subtitlerecognizer.h"
#include "Common/notifier.h"
#include "globalobjects.h"
#include <QSettings>
#include <QFileInfo>
#include <QFile>
#include <QProcess>
#include <QEventLoop>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QDir>
#include <algorithm>
#include "Common/logger.h"
#include "vad.h"
#include "wav.h"
#include "subtitleloader.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace
{
    constexpr qint64 MIN_SUB_DURATION = 300;
    constexpr qint64 DUPLICATE_MERGE_GAP = 1500;

    qint64 samplesToMs(qint64 samples, int sampleRate)
    {
        return sampleRate > 0 ? samples * 1000 / sampleRate : 0;
    }

    int msToSamples(qint64 ms, int sampleRate)
    {
        return sampleRate > 0 ? static_cast<int>(ms * sampleRate / 1000) : 0;
    }

    qint64 maxSubDurationForText(const QString &text)
    {
        const int textLength = qMax(1, text.simplified().size());
        return qBound(4000, textLength * 350, 10000);
    }
}

SubtitleRecognizer::SubtitleRecognizer(const SubRecognizeOptions &options) : KTask{"sub_recognizer"}, _options(options)
{
    QString baseName = QFileInfo(options.videoPath).fileName();
    if (baseName.isEmpty()) baseName = "sub_recognizer";
    _tmpBaseName = QString("%1_%2").arg(baseName, QString::number(reinterpret_cast<quintptr>(this), 16));
    _tmpBaseName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
}

SubtitleRecognizer::~SubtitleRecognizer()
{
    for (const QString &tmpFile : tmpFiles)
    {
        if (QFile::exists(tmpFile)) QFile::remove(tmpFile);
    }
}

QString SubtitleRecognizer::recognize()
{
    setInfo(tr("Processing...Extract Audio"), 0);
    QString wavExtracted = extractAudio(_options.videoPath);
    if (_cancelFlag)
    {
        setInfo(tr("Task Canceled"), NM_ERROR | NM_HIDE);
        return "";
    }
    if (wavExtracted.isEmpty())
    {
        setInfo(tr("Audio Extract Failed"), NM_ERROR | NM_HIDE);
        return "";
    }
    tmpFiles.append(wavExtracted);

    QString modelInput = wavExtracted;
    WAVInfo wavInfo;
    bool loadWAVSuccess = loadWAV(modelInput, wavInfo, _options.vadMode == 0);
    QList<QPair<qint64, qint64>> voiceSegments;
    bool useVAD = false;

    if (!loadWAVSuccess || modelInput.isEmpty())
    {
        setInfo(tr("Load WAV Failed"), NM_ERROR | NM_HIDE);
        Logger::logger()->log(Logger::APP, QString("Load WAV Failed: %1").arg(wavExtracted));
        return "";
    }

    const qint64 audioDuration = samplesToMs(wavInfo.samples, wavInfo.sampleRate);
    setInfo(tr("Processing...Duration: %1").arg(SubItem::encodeTime(audioDuration)), 0);

    if (_options.vadMode != 0)
    {
        if (!runVAD(wavInfo, voiceSegments))
        {
            return "";
        }

        qint64 vadDuration = 0;
        for (const auto &segment : voiceSegments)
        {
            vadDuration += segment.second - segment.first;
        }
        useVAD = _options.vadMode == 1 || (audioDuration > 0 && vadDuration / static_cast<double>(audioDuration) < _options.autoVADThres);
        if (useVAD)
        {
            setInfo(tr("Processing...VAD Duration: %1").arg(SubItem::encodeTime(vadDuration)), 0);
        }
    }
    if (_cancelFlag)
    {
        setInfo(tr("Task Canceled"), NM_ERROR | NM_HIDE);
        return "";
    }

    QString subFile;
    if (useVAD)
    {
        QList<AudioChunk> chunks;
        if (!buildAudioChunks(wavInfo, voiceSegments, chunks))
        {
            setInfo(tr("Build VAD Chunks Failed"), NM_ERROR | NM_HIDE);
            return "";
        }
        subFile = runChunkedRecognize(chunks, audioDuration);
    }
    else
    {
        subFile = runRecognize(wavInfo);
    }
    if (_cancelFlag)
    {
        setInfo(tr("Task Canceled"), NM_ERROR | NM_HIDE);
        if (!subFile.isEmpty()) tmpFiles.append(subFile);
        return "";
    }
    if (_options.cleanupRepeat && !subFile.isEmpty())
    {
        QString cleanedSubFile = cleanupSub(subFile);
        if (!cleanedSubFile.isEmpty() && cleanedSubFile != subFile)
        {
            tmpFiles.append(subFile);
            subFile = cleanedSubFile;
        }
    }
    if (_options.language == "zh" || _options.langDetected == "zh")
    {
#ifdef Q_OS_WIN
        tmpFiles.append(subFile);
        subFile = simplifySub(subFile);
#endif
    }
    return subFile;
}

QString SubtitleRecognizer::extractAudio(const QString &video)
{
    QString ffmpegPath = GlobalObjects::appSetting->value("Play/FFmpeg", "ffmpeg").toString();
    QStringList arguments;
    arguments << "-i" << video << "-y";
    arguments << "-vn" << "-ar" << "16000";
    arguments <<"-ac" << "1" << "-c:a" << "pcm_s16le";
    arguments << "-f" << "wav";

    const QString outputPath = makeTempPath("wav_tmp");
    arguments << outputPath;

    if (QFile::exists(outputPath)) QFile::remove(outputPath);

    QProcess ffmpegProcess;
    QEventLoop eventLoop;
    bool success = true;
    QObject::connect(&ffmpegProcess, &QProcess::errorOccurred, &eventLoop, [&eventLoop, &success](QProcess::ProcessError error){
        if (error == QProcess::FailedToStart)
        {
            Logger::logger()->log(Logger::APP, tr("Start FFmpeg Failed"));
        }
        success = false;
        eventLoop.quit();
    }, Qt::QueuedConnection);
    QObject::connect(&ffmpegProcess, (void (QProcess:: *)(int, QProcess::ExitStatus))&QProcess::finished, &eventLoop,
                     [&eventLoop, &success](int exitCode, QProcess::ExitStatus exitStatus){
        success = (exitStatus == QProcess::NormalExit && exitCode == 0);
        if (!success)
        {
            Logger::logger()->log(Logger::APP, tr("Audio extract Failed, FFmpeg exit code: %1").arg(exitCode));
        }
        eventLoop.quit();
    }, Qt::QueuedConnection);
    QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardOutput, &eventLoop, [&](){
        QString content(ffmpegProcess.readAllStandardOutput());
        Logger::logger()->log(Logger::APP, content);
    }, Qt::QueuedConnection);
    QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardError, &eventLoop, [&]() {
        QString content(ffmpegProcess.readAllStandardError());
        Logger::logger()->log(Logger::APP, content);
    }, Qt::QueuedConnection);

    QObject::connect(this, &SubtitleRecognizer::taskCancel, &eventLoop, [&](){
        ffmpegProcess.kill();
    }, Qt::QueuedConnection);

    QTimer::singleShot(0, [&]() {
        ffmpegProcess.start(ffmpegPath, arguments);
    });
    eventLoop.exec();

    return success ? outputPath : "";
}

bool SubtitleRecognizer::runVAD(const WAVInfo &audio, QList<QPair<qint64, qint64>> &segments)
{
    if (audio.data.empty()) return false;

    bool cancelFlag = false;
    auto conn = QObject::connect(this, &SubtitleRecognizer::taskCancel, this, [&](){
        cancelFlag = true;
    });
    VadIterator vad(_options.vadModelPath, audio.sampleRate, 32, _options.vadThres, _options.vadMinSilence, _options.vadSpeechPad, _options.vadMinSpeech);
    vad.process(audio.data, &cancelFlag);
    QObject::disconnect(conn);
    if (cancelFlag) return false;

    const QList<timestamp_t> &stamps = vad.get_speech_timestamps();
    if (stamps.empty())
    {
        setInfo(tr("No speech detected"), NM_ERROR | NM_HIDE);
        return false;
    }

    const qint64 audioDuration = samplesToMs(audio.samples, audio.sampleRate);
    const int mergeGap = qMax(0, _options.vadChunkMergeGap);
    QList<QPair<qint64, qint64>> paddedSegments;
    paddedSegments.reserve(stamps.size());
    for (const timestamp_t &stamp : stamps)
    {
        qint64 start = qMax<qint64>(0, samplesToMs(stamp.start, audio.sampleRate));
        qint64 end = qMin<qint64>(audioDuration, samplesToMs(stamp.end, audio.sampleRate));
        if (end - start < _options.vadMinSpeech) continue;

        if (!paddedSegments.empty() && start - paddedSegments.back().second <= mergeGap)
        {
            paddedSegments.back().second = qMax(paddedSegments.back().second, end);
        }
        else
        {
            paddedSegments.append({start, end});
        }
    }

    if (paddedSegments.empty())
    {
        setInfo(tr("No speech detected"), NM_ERROR | NM_HIDE);
        return false;
    }

    segments.swap(paddedSegments);
    return true;
}

bool SubtitleRecognizer::buildAudioChunks(const WAVInfo &audio, const QList<QPair<qint64, qint64>> &voiceSegments, QList<AudioChunk> &chunks)
{
    chunks.clear();
    if (audio.data.empty() || voiceSegments.empty()) return false;

    const qint64 maxDuration = qMax<qint64>(10000, _options.vadChunkMaxDuration);
    const int silenceSamples = msToSamples(qMax(0, _options.vadChunkSilence), audio.sampleRate);
    QList<float> chunkData;
    AudioChunk chunk;

    auto flushChunk = [&]() -> bool {
        if (chunkData.empty() || chunk.segments.empty()) return true;

        chunk.duration = samplesToMs(chunkData.size(), audio.sampleRate);
        const QString outputPath = makeTempPath(QString("vad_chunk_%1_wav_tmp").arg(chunks.size()));
        wav::WavWriter writer(chunkData.data(), chunkData.size(), 1, audio.sampleRate, 16);
        writer.Write(outputPath);
        if (!QFile::exists(outputPath)) return false;

        chunk.srcFile = outputPath;
        chunks.append(chunk);
        tmpFiles.append(outputPath);
        chunkData.clear();
        chunk = AudioChunk();
        return true;
    };

    for (const auto &segment : voiceSegments)
    {
        int startSample = qBound(0, msToSamples(segment.first, audio.sampleRate), audio.data.size());
        int endSample = qBound(0, msToSamples(segment.second, audio.sampleRate), audio.data.size());
        if (startSample >= endSample) continue;

        const int segmentSamples = endSample - startSample;
        const qint64 nextDuration = samplesToMs(chunkData.size() + (chunkData.empty() ? 0 : silenceSamples) + segmentSamples, audio.sampleRate);
        if (!chunkData.empty() && nextDuration > maxDuration)
        {
            if (!flushChunk()) return false;
        }

        if (!chunkData.empty() && silenceSamples > 0)
        {
            const int oldSize = chunkData.size();
            chunkData.resize(oldSize + silenceSamples);
        }

        ChunkSegment chunkSegment;
        chunkSegment.chunkStartTime = samplesToMs(chunkData.size(), audio.sampleRate);
        chunkSegment.sourceStartTime = segment.first;
        chunkSegment.sourceEndTime = segment.second;

        chunkData += audio.data.mid(startSample, segmentSamples);

        chunkSegment.chunkEndTime = samplesToMs(chunkData.size(), audio.sampleRate);
        chunk.segments.append(chunkSegment);
    }

    if (!flushChunk()) return false;
    return !chunks.empty();
}

QString SubtitleRecognizer::runRecognize(const WAVInfo &audio, double progressStart, double progressSpan, bool emitPreview)
{
    QStringList arguments;
    arguments << "-m" << _options.modelPath;
    const QString language = _options.language == "auto" && !_options.langDetected.isEmpty() ? _options.langDetected : _options.language;
    arguments << "-l" << language;
    arguments <<"-f" << audio.srcFile;
    arguments << "-osrt";
    if (_options.whisperMaxContext >= 0)
    {
        arguments << "-mc" << QString::number(_options.whisperMaxContext);
    }
    if (_options.whisperMaxLen > 0)
    {
        arguments << "-ml" << QString::number(_options.whisperMaxLen);
    }

    const QString outputPath = makeTempPath(QString("%1_srt_tmp").arg(QFileInfo(audio.srcFile).baseName()));
    arguments << "-of" << outputPath;

    QProcess whisperProcess;
    QEventLoop eventLoop;
    bool success = true;
    QObject::connect(&whisperProcess, &QProcess::errorOccurred, &eventLoop, [&eventLoop, &success](QProcess::ProcessError error){
        if (error == QProcess::FailedToStart)
        {
            Logger::logger()->log(Logger::APP, tr("Start Whisper Failed"));
        }
        success = false;
        eventLoop.quit();
    }, Qt::QueuedConnection);
    QObject::connect(&whisperProcess, (void (QProcess:: *)(int, QProcess::ExitStatus))&QProcess::finished, &eventLoop,
                     [&eventLoop, &success](int exitCode, QProcess::ExitStatus exitStatus){
         success = (exitStatus == QProcess::NormalExit && exitCode == 0);
         if (!success)
         {
             Logger::logger()->log(Logger::APP, tr("Whisper run failed, exit code: %1").arg(exitCode));
         }
         eventLoop.quit();
     }, Qt::QueuedConnection);
    QObject::connect(&whisperProcess, &QProcess::readyReadStandardOutput, &eventLoop, [&](){
        QString content(whisperProcess.readAllStandardOutput().trimmed());
        QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        static const QRegularExpression subRe("^\\[(\\d+:\\d+:\\d+.\\d+)\\s*\\-\\->\\s*(\\d+:\\d+:\\d+.\\d+)\\](.*)");
        QList<SubItem> subItems;
        for (auto &line : lines)
        {
            auto match = subRe.match(line);
            if (!match.hasMatch()) continue;

            SubItem item;
            item.text = match.captured(3);
            item.startTime = SubItem::timeStringToMs(match.captured(1));
            item.endTime = SubItem::timeStringToMs(match.captured(2));
            subItems.append(item);
        }
        if (!subItems.empty())
        {
            double duration = static_cast<double>(audio.samples) / audio.sampleRate * 1000;
            double localProgress = duration > 0 ? subItems.back().endTime / duration : 0;
            emit progreeUpdated(qMin(progressStart + localProgress * progressSpan, 100.0));
            if (emitPreview)
            {
                emit newSubRecognized(subItems);
            }
        }
    }, Qt::QueuedConnection);
    QObject::connect(&whisperProcess, &QProcess::readyReadStandardError, &eventLoop, [&]() {
        QString content(whisperProcess.readAllStandardError().trimmed());
        if (_options.langDetected.isEmpty())
        {
            QStringList lines = content.split('\n', Qt::SkipEmptyParts);
            static const QRegularExpression langRe("language:\\s*(.+)\\s*\\(p\\s*=(.+)\\)");
            for (auto &line : lines)
            {
                auto match = langRe.match(line);
                if (!match.hasMatch()) continue;

                QString langDetected = match.captured(1).trimmed();
                setInfo(tr("Processing...Auto-detected Language: %1").arg(langDetected), 0);
                _options.langDetected = langDetected;
                break;
            }
        }
        Logger::logger()->log(Logger::APP, content);
    }, Qt::QueuedConnection);

    QObject::connect(this, &SubtitleRecognizer::taskCancel, &eventLoop, [&](){
        whisperProcess.kill();
    }, Qt::QueuedConnection);


    QTimer::singleShot(0, [&]() {
        whisperProcess.start(_options.whisperUseCuda ? _options.whisperCudaPath : _options.whisperPath, arguments);
    });
    eventLoop.exec();
    const QString srtPath = outputPath + ".srt";
    if (!success || !QFile::exists(srtPath))
    {
        if (success)
        {
            Logger::logger()->log(Logger::APP, tr("Whisper output file missing: %1").arg(srtPath));
        }
        return "";
    }
    return srtPath;
}

QString SubtitleRecognizer::runChunkedRecognize(const QList<AudioChunk> &chunks, qint64 audioDuration)
{
    if (chunks.empty()) return "";

    SubtitleLoader loader;
    QList<SubItem> allItems;
    for (int i = 0; i < chunks.size(); ++i)
    {
        if (_cancelFlag) return "";

        WAVInfo chunkInfo;
        if (!loadWAV(chunks[i].srcFile, chunkInfo, true))
        {
            Logger::logger()->log(Logger::APP, QString("Load chunk WAV Failed: %1").arg(chunks[i].srcFile));
            return "";
        }

        const double progressStart = i * 100.0 / chunks.size();
        const double progressSpan = 100.0 / chunks.size();
        QString chunkSubFile = runRecognize(chunkInfo, progressStart, progressSpan, false);
        if (chunkSubFile.isEmpty()) return "";
        tmpFiles.append(chunkSubFile);

        SubFile chunkSub(loader.loadSubFile(chunkSubFile));
        QList<SubItem> mappedItems;
        mappedItems.reserve(chunkSub.items.size());
        for (SubItem sub : chunkSub.items)
        {
            const MappedChunkTime mappedStart = mapChunkTime(sub.startTime, chunks[i].segments);
            const MappedChunkTime mappedEnd = mapChunkTime(sub.endTime, chunks[i].segments);
            sub.startTime = qBound<qint64>(0, mappedStart.time, audioDuration);
            sub.endTime = qBound<qint64>(0, mappedEnd.time, audioDuration);
            if (mappedStart.segmentIndex >= 0 && mappedEnd.segmentIndex >= 0 && mappedStart.segmentIndex != mappedEnd.segmentIndex &&
                sub.endTime - sub.startTime > maxSubDurationForText(sub.text))
            {
                sub.endTime = qMin(sub.endTime, chunks[i].segments[mappedStart.segmentIndex].sourceEndTime);
            }
            if (sub.endTime <= sub.startTime)
            {
                sub.endTime = qMin(audioDuration, sub.startTime + MIN_SUB_DURATION);
            }
            if (sub.endTime > sub.startTime && !sub.text.trimmed().isEmpty())
            {
                mappedItems.append(sub);
            }
        }
        if (!mappedItems.empty())
        {
            allItems += mappedItems;
            emit newSubRecognized(mappedItems);
        }
    }

    if (allItems.empty())
    {
        setInfo(tr("No subtitle recognized"), NM_ERROR | NM_HIDE);
        return "";
    }

    cleanupSubItems(allItems);
    SubFile subFile;
    subFile.format = SubFileFormat::F_SRT;
    subFile.items = allItems;

    const QString outputPath = makeTempPath("srt_chunked_tmp.srt");
    return subFile.saveSRT(outputPath) ? outputPath : "";
}

SubtitleRecognizer::MappedChunkTime SubtitleRecognizer::mapChunkTime(qint64 chunkTime, const QList<ChunkSegment> &segments) const
{
    if (segments.empty()) return {chunkTime, -1};

    for (int i = 0; i < segments.size(); ++i)
    {
        const ChunkSegment &segment = segments[i];
        if (chunkTime < segment.chunkStartTime)
        {
            return {segment.sourceStartTime, i};
        }
        if (chunkTime <= segment.chunkEndTime)
        {
            return {segment.sourceStartTime + chunkTime - segment.chunkStartTime, i};
        }
        if (i + 1 < segments.size() && chunkTime < segments[i + 1].chunkStartTime)
        {
            const qint64 prevDistance = chunkTime - segment.chunkEndTime;
            const qint64 nextDistance = segments[i + 1].chunkStartTime - chunkTime;
            return prevDistance <= nextDistance ? MappedChunkTime{segment.sourceEndTime, i} :
                                                  MappedChunkTime{segments[i + 1].sourceStartTime, i + 1};
        }
    }

    return MappedChunkTime(segments.back().sourceEndTime, segments.size() - 1);
}

QString SubtitleRecognizer::simplifySub(const QString &subFilePath)
{
    SubtitleLoader loader;
    SubFile subFile(loader.loadSubFile(subFilePath));

    for (SubItem &sub : subFile.items)
    {
        sub.text = toSimplified(sub.text);
    }

    const QString outputPath = makeTempPath(QString("%1_srt_simp_tmp.srt").arg(QFileInfo(subFilePath).baseName()));
    return subFile.saveSRT(outputPath) ? outputPath : "";
}

QString SubtitleRecognizer::cleanupSub(const QString &subFilePath)
{
    SubtitleLoader loader;
    SubFile subFile(loader.loadSubFile(subFilePath));
    if (subFile.items.empty()) return subFilePath;

    cleanupSubItems(subFile.items);

    const QString outputPath = makeTempPath(QString("%1_srt_cleanup_tmp.srt").arg(QFileInfo(subFilePath).baseName()));
    return subFile.saveSRT(outputPath) ? outputPath : subFilePath;
}

QString SubtitleRecognizer::makeTempPath(const QString &suffix) const
{
    QString safeSuffix = suffix;
    static const QRegularExpression re("[\\\\/:*?\"<>|]");
    safeSuffix.replace(re, "_");
    return QString("%1/%2_%3").arg(GlobalObjects::context()->tmpPath, _tmpBaseName, safeSuffix);
}

void SubtitleRecognizer::cleanupSubItems(QList<SubItem> &items) const
{
    QList<SubItem> normalizedItems;
    normalizedItems.reserve(items.size());
    for (SubItem item : items)
    {
        item.text = cleanupSubText(item.text);
        if (item.text.isEmpty()) continue;
        if (item.endTime <= item.startTime)
        {
            item.endTime = item.startTime + MIN_SUB_DURATION;
        }
        normalizedItems.append(item);
    }

    std::stable_sort(normalizedItems.begin(), normalizedItems.end(), [](const SubItem &item1, const SubItem &item2){
        return item1.startTime < item2.startTime;
    });

    QList<SubItem> cleanedItems;
    for (const SubItem &item : normalizedItems)
    {
        if (!cleanedItems.empty())
        {
            SubItem &last = cleanedItems.back();
            if (normalizedSubText(last.text) == normalizedSubText(item.text) && item.startTime - last.endTime <= DUPLICATE_MERGE_GAP)
            {
                last.endTime = qMax(last.endTime, item.endTime);
                continue;
            }
        }
        cleanedItems.append(item);
    }
    items.swap(cleanedItems);
}

QString SubtitleRecognizer::cleanupSubText(const QString &text) const
{
    QStringList cleanedLines;
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
    {
        QString cleanedLine = line.simplified();
        if (cleanedLine.isEmpty()) continue;
        if (!cleanedLines.empty() && normalizedSubText(cleanedLines.back()) == normalizedSubText(cleanedLine)) continue;
        cleanedLines.append(cleanedLine);
    }
    QString cleaned = cleanedLines.join('\n');
    if (cleaned.size() < 6) return cleaned;

    const int maxUnit = qMin(20, cleaned.size() / 3);
    for (int unitLength = 2; unitLength <= maxUnit; ++unitLength)
    {
        if (cleaned.size() % unitLength != 0) continue;

        const QString unit = cleaned.left(unitLength);
        if (unit.trimmed().isEmpty()) continue;

        bool repeated = true;
        for (int pos = unitLength; pos < cleaned.size(); pos += unitLength)
        {
            if (cleaned.mid(pos, unitLength) != unit)
            {
                repeated = false;
                break;
            }
        }
        if (repeated)
        {
            return unit.trimmed();
        }
    }

    return cleaned;
}

QString SubtitleRecognizer::normalizedSubText(const QString &text) const
{
    QString normalized = text.simplified().toLower();
    static const QRegularExpression reg("\\s+");
    normalized.remove(reg);
    return normalized;
}

QString SubtitleRecognizer::toSimplified(const QString &text)
{
#ifdef Q_OS_WIN
    WORD wLanguageID = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
    LCID Locale = MAKELCID(wLanguageID, SORT_CHINESE_PRCP);
    QScopedPointer<QChar> buf(new QChar[text.length()]);
    LCMapString(Locale,LCMAP_SIMPLIFIED_CHINESE, reinterpret_cast<LPCWSTR>(text.constData()), text.length(),reinterpret_cast<LPWSTR>(buf.data()), text.length());
    return QString(buf.data(), text.length());
#else
    return text;
#endif
}

bool SubtitleRecognizer::loadWAV(const QString &filename, WAVInfo &info, bool headerOnly)
{
    info.srcFile = filename;
    wav::WavReader reader;
    bool ret = reader.Open(filename, headerOnly);  //16000,1,32float
    if (!ret) return false;
    info.channel = reader.num_channel();
    info.sampleRate = reader.sample_rate();
    info.samples = reader.num_samples();
    if (headerOnly) return true;

    info.data.resize(reader.num_samples());
    for (int i = 0; i < reader.num_samples(); i += info.channel)
    {
        info.data[i] = static_cast<float>(*(reader.data() + i));
    }

    return true;
}

TaskStatus SubtitleRecognizer::runTask()
{
    QString subFile = recognize();
    if (!subFile.isEmpty())
    {
        emit recognizedDown(subFile);
        return TaskStatus::Finished;
    }
    return _cancelFlag ? TaskStatus::Cancelled : TaskStatus::Failed;
}

SubRecognizeOptions::SubRecognizeOptions()
{
    QString rPathBase;

    const QString strApp(QCoreApplication::applicationDirPath() + "/extension/sub_recognizer");
#ifdef CONFIG_UNIX_DATA
    const QString strHome(QDir::homePath()+"/.config/kikoplay/extension/sub_recognizer");
    const QString strSys("/usr/share/kikoplay/extension/sub_recognizer");

    const QFileInfo fileinfoHome(strHome);
    const QFileInfo fileinfoSys(strSys);
    const QFileInfo fileinfoApp(strApp);

    if (fileinfoHome.exists() || fileinfoHome.isDir()) {
        rPathBase = strHome;
    } else if (fileinfoSys.exists() || fileinfoSys.isDir()) {
        rPathBase = strSys;
    } else {
        rPathBase = strApp;
    }

#else
    rPathBase = strApp;
#endif

    whisperPath = rPathBase + "/whisper/main.exe";
    whisperCudaPath = rPathBase + "/whisper/cuda/main.exe";
    modelPathBase = rPathBase + "/model";
    vadModelPath = rPathBase + "/silero_vad.onnx";

}

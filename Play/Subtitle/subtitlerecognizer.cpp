#include "subtitlerecognizer.h"
#include "Common/notifier.h"
#include "globalobjects.h"
#include <QSettings>
#include <QFileInfo>
#include <QProcess>
#include <QEventLoop>
#include <QRegularExpression>
#include <QCoreApplication>
#include "Common/logger.h"
#include "vad.h"
#include "wav.h"
#include "subtitleloader.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

SubtitleRecognizer::SubtitleRecognizer(const SubRecognizeOptions &options) : KTask{"sub_recognizer"}, _options(options)
{

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
    bool loadWAVSuccess = false;
    QList<QPair<qint64, qint64>> hasVoiceSegments;
    bool useVAD = false;

    if (_options.vadMode == 0)
    {
        loadWAVSuccess = loadWAV(modelInput, wavInfo, true);
        if (loadWAVSuccess)
        {
            setInfo(tr("Processing...Duration: %1").arg(SubItem::encodeTime(static_cast<double>(wavInfo.samples) / wavInfo.sampleRate * 1000)), 0);
        }
    }
    else
    {
        do
        {
            loadWAVSuccess = loadWAV(modelInput, wavInfo);
            if (!loadWAVSuccess) break;
            setInfo(tr("Processing...Duration: %1").arg(SubItem::encodeTime(static_cast<double>(wavInfo.samples) / wavInfo.sampleRate * 1000)), 0);

            QString vadSplitFile = runVAD(wavInfo, hasVoiceSegments);
            if (vadSplitFile.isEmpty()) break;
            tmpFiles.append(vadSplitFile);

            WAVInfo splitWAVInfo;
            loadWAVSuccess = loadWAV(vadSplitFile, splitWAVInfo);
            if (!loadWAVSuccess) break;

            if (_options.vadMode == 1 || static_cast<float>(splitWAVInfo.samples) / wavInfo.samples < _options.autoVADThres)
            {
                useVAD = true;
                modelInput = vadSplitFile;
                wavInfo = splitWAVInfo;
                setInfo(tr("Processing...VAD Duration: %1").arg(SubItem::encodeTime(static_cast<double>(wavInfo.samples) / wavInfo.sampleRate * 1000)), 0);
            }
        } while(false);
    }
    if (_cancelFlag)
    {
        setInfo(tr("Task Canceled"), NM_ERROR | NM_HIDE);
        return "";
    }
    if (!loadWAVSuccess || modelInput.isEmpty())
    {
        setInfo(tr("Load WAV Failed"), NM_ERROR | NM_HIDE);
        Logger::logger()->log(Logger::APP, QString("Load WAV Failed: %1").arg(wavExtracted));
        return "";
    }
    QString subFile = runRecognize(wavInfo);
    if (_cancelFlag)
    {
        setInfo(tr("Task Canceled"), NM_ERROR | NM_HIDE);
        if (!subFile.isEmpty()) tmpFiles.append(subFile);
        return "";
    }
    if (useVAD)
    {
        tmpFiles.append(subFile);
        subFile = adjustSub(subFile, hasVoiceSegments);
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

    const QString outputPath = QString("%1/%2_wav_tmp").arg(GlobalObjects::context()->tmpPath, QFileInfo(video).fileName());
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

QString SubtitleRecognizer::runVAD(const WAVInfo &audio, QList<QPair<qint64, qint64>> &segments)
{
    if (audio.data.empty()) return "";

    bool cancelFlag = false;
    auto conn = QObject::connect(this, &SubtitleRecognizer::taskCancel, this, [&](){
        cancelFlag = true;
    });
    VadIterator vad(_options.vadModelPath, audio.sampleRate, 32, _options.vadThres, _options.vadMinSilence, _options.vadSpeechPad, _options.vadMinSpeech);
    vad.process(audio.data, &cancelFlag);
    if (cancelFlag) return "";
    QObject::disconnect(conn);

    const QList<timestamp_t> &stamps = vad.get_speech_timestamps();
    segments.resize(stamps.size());
    for (int i = 0; i < stamps.size(); i++)
    {
        segments[i].first = stamps[i].start / (double)audio.sampleRate * 1000;
        segments[i].second = stamps[i].end / (double)audio.sampleRate * 1000;
    }

    QList<float> output_wav;
    vad.collect_chunks(audio.data, output_wav);
    wav::WavWriter writer(output_wav.data(), output_wav.size(), 1, audio.sampleRate, 16);
    const QString outputPath = QString("%1/%2_vad_wav_tmp").arg(GlobalObjects::context()->tmpPath, QFileInfo(audio.srcFile).fileName());
    writer.Write(outputPath);

    return outputPath;
}

QString SubtitleRecognizer::runRecognize(const WAVInfo &audio)
{
    QStringList arguments;
    arguments << "-m" << _options.modelPath;
    arguments << "-l" << _options.language;
    arguments <<"-f" << audio.srcFile;
    arguments << "-osrt";

    const QString outputPath = QString("%1/%2_srt_tmp").arg(GlobalObjects::context()->tmpPath, QFileInfo(audio.srcFile).fileName());
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
            emit progreeUpdated(qMin(subItems.back().endTime / duration * 100, 100.0));
            emit newSubRecognized(subItems);
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
    return success ? outputPath + ".srt" : "";
}

QString SubtitleRecognizer::adjustSub(const QString &subFilePath, const QList<QPair<qint64, qint64>> &voiceSegments)
{
    QList<QPair<qint64, qint64>> targetSegments;
    targetSegments.reserve(voiceSegments.size());
    qint64 offset = 0;
    for (auto &p : voiceSegments)
    {
        qint64 nextStart = offset + p.second - p.first;
        targetSegments.append({offset, nextStart});
        offset = nextStart;
    }

    SubtitleLoader loader;
    SubFile subFile(loader.loadSubFile(subFilePath));

    for (SubItem &sub : subFile.items)
    {
        for (int i = targetSegments.size() - 1; i >= 0; --i)
        {
            if (sub.startTime >= targetSegments[i].first)
            {
                sub.startTime = voiceSegments[i].first + sub.startTime - targetSegments[i].first;
                sub.endTime = voiceSegments[i].first + sub.endTime - targetSegments[i].first;
                break;
            }
        }
    }

    const QString outputPath = QString("%1/%2_srt_adjust_tmp.srt").arg(GlobalObjects::context()->tmpPath, QFileInfo(subFilePath).baseName());
    return subFile.saveSRT(outputPath) ? outputPath : "";
}

QString SubtitleRecognizer::simplifySub(const QString &subFilePath)
{
    SubtitleLoader loader;
    SubFile subFile(loader.loadSubFile(subFilePath));

    for (SubItem &sub : subFile.items)
    {
        sub.text = toSimplified(sub.text);
    }

    const QString outputPath = QString("%1/%2_srt_simp_tmp.srt").arg(GlobalObjects::context()->tmpPath, QFileInfo(subFilePath).baseName());
    return subFile.saveSRT(outputPath) ? outputPath : "";
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

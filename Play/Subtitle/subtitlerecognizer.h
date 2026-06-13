#ifndef SUBTITLERECOGNIZER_H
#define SUBTITLERECOGNIZER_H

#include "Common/taskpool.h"
#include "subitem.h"

struct SubRecognizeOptions
{
    SubRecognizeOptions();

    QString videoPath;

    QString rPathBase;
    QString whisperPath;
    QString whisperCudaPath;
    QString modelPathBase;
    QString vadModelPath;

    bool whisperUseCuda{false};
    QString modelPath;
    QString language{"auto"};
    QString langDetected;

    int vadMode{0};  // 0: disable  1: enable  2: auto
    float autoVADThres{0.8};
    float vadThres{0.5};
    int vadMinSilence{1000};  // In the end of each speech chunk wait for min_silence_duration_ms before separating it(ms)
    int vadMinSpeech{200};  // Final speech chunks shorter min_speech_duration_ms are thrown out(ms)
    int vadSpeechPad{200};  // Final speech chunks are padded by speech_pad_ms each side(ms)
    int vadChunkMaxDuration{45000};
    int vadChunkMergeGap{300};
    int vadChunkSilence{300};

    int whisperMaxContext{32};
    int whisperMaxLen{80};
    bool cleanupRepeat{true};
};

struct WAVInfo
{
    QString srcFile;
    QList<float> data;
    int channel;
    int sampleRate;
    int samples;
};

class SubtitleRecognizer : public KTask
{
    Q_OBJECT
public:
    explicit SubtitleRecognizer(const SubRecognizeOptions &options);
    ~SubtitleRecognizer();

private:
    struct ChunkSegment
    {
        qint64 chunkStartTime = 0;
        qint64 chunkEndTime = 0;
        qint64 sourceStartTime = 0;
        qint64 sourceEndTime = 0;
    };

    struct AudioChunk
    {
        QString srcFile;
        qint64 duration = 0;
        QList<ChunkSegment> segments;
    };

    struct MappedChunkTime
    {
        MappedChunkTime(qint64 mappedTime = 0, int mappedSegmentIndex = -1) :
            time(mappedTime), segmentIndex(mappedSegmentIndex) {}

        qint64 time = 0;
        int segmentIndex = -1;
    };

    SubRecognizeOptions _options;
    QStringList tmpFiles;
    QString _tmpBaseName;

    QString recognize();
    QString extractAudio(const QString &video);
    bool runVAD(const WAVInfo &audio, QList<QPair<qint64, qint64>> &segments);
    bool buildAudioChunks(const WAVInfo &audio, const QList<QPair<qint64, qint64>> &voiceSegments, QList<AudioChunk> &chunks);
    QString runRecognize(const WAVInfo &audio, double progressStart = 0, double progressSpan = 100, bool emitPreview = true);
    QString runChunkedRecognize(const QList<AudioChunk> &chunks, qint64 audioDuration);
    MappedChunkTime mapChunkTime(qint64 chunkTime, const QList<ChunkSegment> &segments) const;
    QString cleanupSub(const QString &subFilePath);
    QString simplifySub(const QString &subFilePath);

    QString makeTempPath(const QString &suffix) const;
    QString toSimplified(const QString &text);
    bool loadWAV(const QString &filename, WAVInfo &info, bool headerOnly = false);
    void cleanupSubItems(QList<SubItem> &items) const;
    QString cleanupSubText(const QString &text) const;
    QString normalizedSubText(const QString &text) const;

protected:
    virtual TaskStatus runTask() override;

signals:
    void newSubRecognized(QList<SubItem> subItems);
    void recognizedDown(const QString &srtFile);

};

#endif // SUBTITLERECOGNIZER_H

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
    int vadSpeechPad{30};  // Final speech chunks are padded by speech_pad_ms each side(ms)
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
    SubRecognizeOptions _options;
    QStringList tmpFiles;

    QString recognize();
    QString extractAudio(const QString &video);
    QString runVAD(const WAVInfo &audio, QList<QPair<qint64, qint64>> &segments);
    QString runRecognize(const WAVInfo &audio);
    QString adjustSub(const QString &subFilePath, const QList<QPair<qint64, qint64>> &voiceSegments);
    QString simplifySub(const QString &subFilePath);

    QString toSimplified(const QString &text);
    bool loadWAV(const QString &filename, WAVInfo &info, bool headerOnly = false);

protected:
    virtual TaskStatus runTask() override;

signals:
    void newSubRecognized(QList<SubItem> subItems);
    void recognizedDown(const QString &srtFile);

};

#endif // SUBTITLERECOGNIZER_H

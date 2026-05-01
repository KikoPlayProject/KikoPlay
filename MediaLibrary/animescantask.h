#ifndef ANIMESCANTASK_H
#define ANIMESCANTASK_H

#include "Common/taskpool.h"
#include <QStringList>
#include <QSet>
#include <QRegularExpression>

class MPVMediaInfo;

struct AnimeScanOptions
{
    QStringList directories;
    int minDurationSec{0};
    int maxDurationSec{0};
    bool enableDurationFilter{false};
    QStringList filenameFilters;
};

struct ScanFileRecord
{
    enum Status { NewAnime, ExistingAnime, Filtered, Failed };
    enum FilterReason { NoFilter, FilenameExcluded, AlreadyInLibrary, DurationOutOfRange };
    Status status;
    FilterReason filterReason{NoFilter};
    QString filePath;
    QString animeName;
    QString epString;
};

struct AnimeScanResult
{
    int totalFiles{0};
    int matchedFiles{0};
    int skippedFiles{0};
    int failedFiles{0};
    QVector<ScanFileRecord> records;
};
Q_DECLARE_METATYPE(AnimeScanResult)

class AnimeScanTask : public KTask
{
    Q_OBJECT
public:
    AnimeScanTask(const AnimeScanOptions &options, MPVMediaInfo *mediaInfo = nullptr);
    const AnimeScanResult &result() const { return _result; }

signals:
    void scanFinished(AnimeScanResult result);

protected:
    TaskStatus runTask() override;

private:
    void collectFiles(const QString &dir, QStringList &files);
    bool passesFilenameFilter(const QString &fileName) const;
    bool passesDurationFilter(const QString &filePath);
    bool passLibrarayExistFilter(const QString &filePath);

    AnimeScanOptions _options;
    AnimeScanResult _result;
    QVector<QRegularExpression> _filenameRules;
    QSet<QString> _newAnimeNames;
    MPVMediaInfo *_mediaInfo;
};

#endif // ANIMESCANTASK_H

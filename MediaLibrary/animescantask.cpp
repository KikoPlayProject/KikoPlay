#include "animescantask.h"
#include <QDir>
#include <QFileInfo>
#include "Common/logger.h"
#include "Common/notifier.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/animeprovider.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Video/mpvmediainfo.h"
#include "globalobjects.h"

AnimeScanTask::AnimeScanTask(const AnimeScanOptions &options, MPVMediaInfo *mediaInfo)
    : KTask(tr("Anime Scan")), _options(options), _mediaInfo(mediaInfo)
{
    for (const QString &pattern : _options.filenameFilters)
    {
        QString trimmed = pattern.trimmed();
        if (trimmed.isEmpty()) continue;
        _filenameRules.append(QRegularExpression(trimmed, QRegularExpression::CaseInsensitiveOption));
    }
}

TaskStatus AnimeScanTask::runTask()
{
    setInfo(tr("Collecting video files..."), NM_HIDE);

    QStringList files;
    for (const QString &dir : _options.directories)
    {
        if (_cancelFlag) break;
        collectFiles(dir, files);
    }
    if (_cancelFlag) return TaskStatus::Cancelled;

    _result.totalFiles = files.size();
    if (files.isEmpty())
    {
        setInfo(tr("No video files found"), NM_HIDE);
        return TaskStatus::Failed;
    }
    setInfo(tr("Found %1 video files, filtering...").arg(files.size()), NM_HIDE);

    QStringList filteredFiles;
    for (int i = 0; i < files.size(); ++i)
    {
        if (_cancelFlag) return TaskStatus::Cancelled;
        const QString &filePath = files[i];
        const QString fileName = QFileInfo(filePath).fileName();

        if (!passesFilenameFilter(fileName))
        {
            _result.skippedFiles++;
            _result.records.append({ScanFileRecord::Filtered, ScanFileRecord::FilenameExcluded, filePath, "", ""});
            continue;
        }
        if (!passLibrarayExistFilter(filePath))
        {
            _result.skippedFiles++;
            _result.records.append({ScanFileRecord::Filtered, ScanFileRecord::AlreadyInLibrary, filePath, "", ""});
            continue;
        }
        if (_options.enableDurationFilter && !passesDurationFilter(filePath))
        {
            _result.skippedFiles++;
            _result.records.append({ScanFileRecord::Filtered, ScanFileRecord::DurationOutOfRange, filePath, "", ""});
            continue;
        }
        filteredFiles.append(filePath);
        updateProgress(i * 30 / files.size());
    }

    if (_cancelFlag) return TaskStatus::Cancelled;
    if (filteredFiles.isEmpty())
    {
        setInfo(tr("All files filtered out"), NM_HIDE);
        return TaskStatus::Failed;
    }

    setInfo(tr("Matching %1 files...").arg(filteredFiles.size()), NM_HIDE);
    for (int i = 0; i < filteredFiles.size(); ++i)
    {
        if (_cancelFlag)
        {
            if (i > 0)
            {
                emit scanFinished(_result);
                return TaskStatus::PartiallyFinished;
            }
            return TaskStatus::Cancelled;
        }
        const QString &filePath = filteredFiles[i];
        const QString fileName = QFileInfo(filePath).fileName();
        setInfo(tr("Matching: %1").arg(fileName), NM_HIDE);

        MatchResult match;
        GlobalObjects::danmuManager->localMatch(filePath, match);
        if (match.success)
        {
            if (!AnimeWorker::instance()->hasAnime(match.name))
            {
                match.success = false;
            }
        }
        if (!match.success)
        {
            GlobalObjects::animeProvider->match(GlobalObjects::animeProvider->defaultMatchScript(), filePath, match);
        }

        if (match.success)
        {
            bool isNew = !AnimeWorker::instance()->hasAnime(match.name) && !_newAnimeNames.contains(match.name);
            match.ep.localFile = filePath;
            GlobalObjects::danmuManager->createPool(filePath, match);
            AnimeWorker::instance()->addAnime(match);
            _result.matchedFiles++;
            if (isNew) _newAnimeNames.insert(match.name);
            _result.records.append({
                _newAnimeNames.contains(match.name) ? ScanFileRecord::NewAnime : ScanFileRecord::ExistingAnime,
                ScanFileRecord::NoFilter, filePath, match.name, match.ep.toString()
            });
        }
        else
        {
            _result.failedFiles++;
            _result.records.append({ScanFileRecord::Failed, ScanFileRecord::NoFilter, filePath, "", ""});
            Logger::logger()->log(Logger::APP, tr("Scan match failed: %1").arg(filePath));
        }
        updateProgress(30 + (i + 1) * 70 / filteredFiles.size());
    }

    setInfo(tr("Scan complete: %1 matched, %2 failed, %3 skipped")
                .arg(_result.matchedFiles).arg(_result.failedFiles).arg(_result.skippedFiles), NM_HIDE);
    emit scanFinished(_result);
    return TaskStatus::Finished;
}

void AnimeScanTask::collectFiles(const QString &dir, QStringList &files)
{
    QDir folder(dir);
    for (const QFileInfo &fileInfo : folder.entryInfoList())
    {
        if (_cancelFlag) return;
        const QString fileName = fileInfo.fileName();
        if (fileInfo.isFile())
        {
            if (GlobalObjects::mpvplayer->videoFileFormats.contains("*." + fileInfo.suffix().toLower()))
            {
                files.append(fileInfo.absoluteFilePath());
            }
        }
        else
        {
            if (fileName == "." || fileName == "..") continue;
            collectFiles(fileInfo.absoluteFilePath(), files);
        }
    }
}

bool AnimeScanTask::passesFilenameFilter(const QString &fileName) const
{
    for (const auto &rule : _filenameRules)
    {
        if (fileName.contains(rule)) return false;
    }
    return true;
}

bool AnimeScanTask::passesDurationFilter(const QString &filePath)
{
    if (!_mediaInfo) return true;
    if (!_mediaInfo->loadFile(filePath)) return true;
    qint64 durationMs = _mediaInfo->getDuration();
    _mediaInfo->reset();
    int durationSec = durationMs / 1000;
    if (_options.minDurationSec > 0 && durationSec < _options.minDurationSec) return false;
    if (_options.maxDurationSec > 0 && durationSec > _options.maxDurationSec) return false;
    return true;
}

bool AnimeScanTask::passLibrarayExistFilter(const QString &filePath)
{
    return !AnimeWorker::instance()->hasFile(filePath);
}

#include "webdavsubtitleloader.h"
#include "Play/Playlist/webdav/qwebdav.h"
#include "Play/Playlist/webdav/qwebdavdirparser.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>
#include <algorithm>

namespace
{
    const int requestTimeout = 30000;
    const qint64 maxSubtitleSize = 20 * 1024 * 1024;

    bool matchesSubtitle(const QString &name, const QString &videoName, const QString &mode)
    {
        const QString baseName = QFileInfo(name).completeBaseName();
        if (mode == "all") return true;
        if (mode == "fuzzy") return baseName.contains(videoName, Qt::CaseInsensitive);
        if (baseName.compare(videoName, Qt::CaseInsensitive) == 0) return true;
        if (!baseName.startsWith(videoName, Qt::CaseInsensitive)) return false;

        // Only known language/encoding tags extend an exact match; episode numbers do not.
        static const QRegularExpression languageSuffix(
            QStringLiteral("^(?:[. _\\-\\[\\]()&]+(?:uni_(?:gb|big5)|zh(?:-(?:cn|tw|hk|hans|hant))?|zho|chi|chs|cht|sc|tc|cn|gb|big5|"
                           "en|eng|ja|jpn|jp|ko|kor|fr|fra|fre|de|deu|ger|es|spa|ru|rus|pt|por|it|ita|"
                           "ar|ara|th|tha|vi|vie|id|ind|简体中文|繁体中文|繁體中文|简体|繁体|简中|繁中|中文|英文|日文|双语|雙語))+[\\] )]*$"),
            QRegularExpression::CaseInsensitiveOption);
        return languageSuffix.match(baseName.mid(videoName.size())).hasMatch();
    }
}

WebDAVSubtitleLoader::WebDAVSubtitleLoader(const QString &cacheDir, QObject *parent)
    : QObject(parent), cacheDir(cacheDir)
{
}

WebDAVSubtitleLoader::~WebDAVSubtitleLoader()
{
    cancel();
}

void WebDAVSubtitleLoader::start(const QString &videoPath, const QString &directoryUrl,
                                const QString &user, const QString &password, const QString &matchMode)
{
    cancel();
    this->videoPath = videoPath;
    this->videoName = QFileInfo(QUrl(videoPath).fileName()).completeBaseName();
    this->directoryUrl = QUrl(directoryUrl);
    this->directoryUrl.setUserInfo(QString());
    this->matchMode = matchMode;
    if (matchMode == "no" || videoName.isEmpty())
    {
        finish();
        return;
    }
    if (!QDir().mkpath(cacheDir))
    {
        finish(tr("Unable to create the WebDAV subtitle cache directory"));
        return;
    }

    task = new QObject(this);
    webdav = new QWebdav(task);
    webdav->setConnectionSettings(user, password);
    parser = new QWebdavDirParser(task);
    timer = new QTimer(task);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (reply)
        {
            requestError = tr("Subtitle download timed out");
            reply->abort();
        }
        else
        {
            finish(tr("WebDAV directory request timed out"));
        }
    });
    connect(parser, &QWebdavDirParser::finished, this, &WebDAVSubtitleLoader::directoryFinished);
    connect(parser, &QWebdavDirParser::errorChanged, this, [this](const QString &) {
        finish(tr("Unable to list the WebDAV subtitle directory"));
    });
    if (!parser->listDirectory(webdav, this->directoryUrl.toString(QUrl::FullyEncoded)))
    {
        finish(tr("Unable to list the WebDAV subtitle directory"));
        return;
    }
    timer->start(requestTimeout);
    emit progress(this->videoPath, tr("Searching for WebDAV subtitles..."));
}

void WebDAVSubtitleLoader::cancel()
{
    if (timer)
    {
        timer->stop();
        disconnect(timer, nullptr, this, nullptr);
    }
    if (parser)
    {
        disconnect(parser, nullptr, this, nullptr);
        if (parser->isBusy()) parser->abort();
    }
    if (reply)
    {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
    }
    if (file) file->cancelWriting();
    if (task) task->deleteLater();
    task = nullptr;
    webdav = nullptr;
    parser = nullptr;
    reply = nullptr;
    file = nullptr;
    timer = nullptr;
    candidates.clear();
    subtitles.clear();
    nextCandidate = 0;
    failedCount = 0;
    requestError.clear();
}

void WebDAVSubtitleLoader::directoryFinished()
{
    timer->stop();
    QList<QWebdavItem> items = parser->getList();
    std::sort(items.begin(), items.end(), [](const QWebdavItem &left, const QWebdavItem &right) {
        return left.name() < right.name();
    });
    for (const QWebdavItem &item : items)
    {
        const QString extension = item.ext().toLower();
        if (item.isDir() || (extension != "ass" && extension != "ssa" && extension != "srt" && extension != "vtt")) continue;
        if (!matchesSubtitle(item.name(), videoName, matchMode)) continue;
        // Depth: 1 responses should contain only children of this directory.
        if (item.path().left(item.path().lastIndexOf('/') + 1) != directoryUrl.path()) continue;

        Candidate candidate;
        candidate.url = directoryUrl;
        candidate.url.setPath(item.path(), QUrl::DecodedMode);
        candidate.title = item.name();
        candidate.size = item.size();
        QByteArray key = candidate.url.toEncoded() + '\0' + webdav->username().toUtf8() + '\0';
        key += QByteArray::number(item.size()) + '\0' + QByteArray::number(item.lastModified().toMSecsSinceEpoch());
        candidate.cachePath = QDir(cacheDir).filePath(
            QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex()) + "." + extension);
        candidates.append(candidate);
    }
    downloadNext();
}

void WebDAVSubtitleLoader::downloadNext()
{
    while (nextCandidate < candidates.size())
    {
        const Candidate &candidate = candidates.at(nextCandidate++);
        if (candidate.size > quint64(maxSubtitleSize))
        {
            ++failedCount;
            continue;
        }
        const QFileInfo cached(candidate.cachePath);
        if (cached.isFile() && cached.size() > 0 && (!candidate.size || quint64(cached.size()) == candidate.size))
        {
            subtitles.append({candidate.cachePath, candidate.title});
            continue;
        }
        file = new QSaveFile(candidate.cachePath, task);
        if (!file->open(QIODevice::WriteOnly))
        {
            delete file;
            file = nullptr;
            ++failedCount;
            continue;
        }
        requestError.clear();
        reply = webdav->get(candidate.url.toString(QUrl::FullyEncoded));
        reply->setReadBufferSize(64 * 1024);
        connect(reply, &QNetworkReply::readyRead, this, &WebDAVSubtitleLoader::readDownload);
        connect(reply, &QNetworkReply::finished, this, &WebDAVSubtitleLoader::downloadFinished);
        timer->start(requestTimeout);
        emit progress(videoPath, tr("Downloading WebDAV subtitles (%1/%2): %3")
                      .arg(nextCandidate).arg(candidates.size()).arg(candidate.title));
        return;
    }
    finish();
}

void WebDAVSubtitleLoader::readDownload()
{
    if (!requestError.isEmpty() || !reply->bytesAvailable()) return;
    const QByteArray data = reply->readAll();
    if (file->size() + data.size() > maxSubtitleSize)
    {
        requestError = tr("Subtitle exceeds the 20 MB download limit");
    }
    else if (file->write(data) != data.size())
    {
        requestError = tr("Unable to save the downloaded subtitle");
    }
    if (!requestError.isEmpty() && !reply->isFinished()) reply->abort();
}

void WebDAVSubtitleLoader::downloadFinished()
{
    timer->stop();
    readDownload();
    const Candidate &candidate = candidates.at(nextCandidate - 1);
    bool success = reply->error() == QNetworkReply::NoError && requestError.isEmpty();
    success = success && file->size() > 0 && (!candidate.size || quint64(file->size()) == candidate.size);
    if (success) success = file->commit();
    if (success)
    {
        subtitles.append({candidate.cachePath, candidate.title});
    }
    else
    {
        file->cancelWriting();
        ++failedCount;
    }
    delete file;
    file = nullptr;
    reply->deleteLater();
    reply = nullptr;
    downloadNext();
}

void WebDAVSubtitleLoader::finish(const QString &error)
{
    const QString completedVideo = videoPath;
    const QList<Subtitle> completedSubtitles = subtitles;
    const int failures = failedCount;
    cancel();
    emit finished(completedVideo, completedSubtitles, failures, error);
}

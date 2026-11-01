#ifndef WEBDAVSUBTITLELOADER_H
#define WEBDAVSUBTITLELOADER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QUrl>

class QWebdav;
class QWebdavDirParser;
class QNetworkReply;
class QSaveFile;
class QTimer;

class WebDAVSubtitleLoader : public QObject
{
    Q_OBJECT
public:
    struct Subtitle
    {
        QString path;
        QString title;
    };

    explicit WebDAVSubtitleLoader(const QString &cacheDir, QObject *parent = nullptr);
    ~WebDAVSubtitleLoader();

    void start(const QString &videoPath, const QString &directoryUrl,
               const QString &user, const QString &password, const QString &matchMode);
    void cancel();

signals:
    void progress(const QString &videoPath, const QString &message);
    void finished(const QString &videoPath, const QList<WebDAVSubtitleLoader::Subtitle> &subtitles,
                  int failedCount, const QString &error);

private:
    struct Candidate
    {
        QUrl url;
        QString title;
        QString cachePath;
        quint64 size;
    };

    void directoryFinished();
    void downloadNext();
    void readDownload();
    void downloadFinished();
    void finish(const QString &error = QString());

    QString cacheDir;
    QString videoPath;
    QString videoName;
    QString matchMode;
    QUrl directoryUrl;
    QObject *task = nullptr;
    QWebdav *webdav = nullptr;
    QWebdavDirParser *parser = nullptr;
    QNetworkReply *reply = nullptr;
    QSaveFile *file = nullptr;
    QTimer *timer = nullptr;
    QList<Candidate> candidates;
    QList<Subtitle> subtitles;
    int nextCandidate = 0;
    int failedCount = 0;
    QString requestError;
};

#endif // WEBDAVSUBTITLELOADER_H

#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QXmlStreamWriter>
#include "Play/Subtitle/webdavsubtitleloader.h"

// A local DAV endpoint exercises the real parser, authentication and download paths.
class DavServer : public QObject
{
public:
    struct File
    {
        QString name;
        QByteArray data = "subtitle contents\n";
        QString modified = "Wed, 28 Oct 2026 12:00:00 GMT";
        int status = 200;
        bool truncated = false;
        qint64 declaredSize = -1;
    };

    DavServer()
    {
        server.listen(QHostAddress::LocalHost);
        connect(&server, &QTcpServer::newConnection, this, [this]() {
            while (server.hasPendingConnections())
            {
                QTcpSocket *socket = server.nextPendingConnection();
                connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                    QByteArray &buffer = buffers[socket];
                    buffer += socket->readAll();
                    const int headerEnd = buffer.indexOf("\r\n\r\n");
                    if (headerEnd < 0) return;
                    const QList<QByteArray> lines = buffer.left(headerEnd).split('\n');
                    QMap<QByteArray, QByteArray> headers;
                    for (int i = 1; i < lines.size(); ++i)
                    {
                        const int colon = lines[i].indexOf(':');
                        headers.insert(lines[i].left(colon).trimmed().toLower(), lines[i].mid(colon + 1).trimmed());
                    }
                    if (buffer.size() < headerEnd + 4 + headers.value("content-length").toInt()) return;
                    const QList<QByteArray> request = lines.first().trimmed().split(' ');
                    buffers.remove(socket);
                    handle(socket, request[0], QUrl::fromEncoded(request[1]).path(), headers);
                });
                connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
                    buffers.remove(socket);
                    socket->deleteLater();
                });
            }
        });
    }

    QString url(const QString &name = QString()) const
    {
        QUrl result;
        result.setScheme("http");
        result.setHost("127.0.0.1");
        result.setPort(server.serverPort());
        result.setPath("/dav/" + name);
        return result.toString(QUrl::FullyEncoded);
    }

    bool isListening() const { return server.isListening(); }
    QList<File> files;
    QMap<QString, int> downloads;
    int listings = 0;
    int challenges = 0;
    int directoryStatus = 207;
    bool requireAuth = false;
    bool holdDownloads = false;
    bool absoluteHref = false;

private:
    void respond(QTcpSocket *socket, int status, const QByteArray &body, const QByteArray &contentType,
                 bool truncated = false)
    {
        const QByteArray phrase = status == 207 ? "Multi-Status" : status == 200 ? "OK" : "Error";
        QByteArray header = "HTTP/1.1 " + QByteArray::number(status) + " " + phrase + "\r\n";
        header += "Connection: close\r\nContent-Type: " + contentType + "\r\n";
        header += "Content-Length: " + QByteArray::number(body.size() + (truncated ? 10 : 0)) + "\r\n";
        if (status == 401) header += "WWW-Authenticate: Basic realm=\"test\"\r\n";
        socket->write(header + "\r\n" + body);
        socket->disconnectFromHost();
    }

    QByteArray listing() const
    {
        QByteArray body;
        QXmlStreamWriter xml(&body);
        xml.writeStartDocument();
        xml.writeStartElement("multistatus");
        xml.writeDefaultNamespace("DAV:");
        for (const File &file : files)
        {
            xml.writeStartElement("response");
            xml.writeTextElement("href", absoluteHref ? url(file.name) : QUrl(url(file.name)).path(QUrl::FullyEncoded));
            xml.writeStartElement("propstat");
            xml.writeTextElement("status", "HTTP/1.1 200 OK");
            xml.writeStartElement("prop");
            xml.writeTextElement("getcontentlength", QString::number(file.declaredSize < 0 ? file.data.size() : file.declaredSize));
            xml.writeTextElement("getlastmodified", file.modified);
            xml.writeStartElement("resourcetype");
            if (file.name.endsWith('/')) xml.writeEmptyElement("collection");
            xml.writeEndElement();
            xml.writeEndElement();
            xml.writeEndElement();
            xml.writeEndElement();
        }
        xml.writeEndDocument();
        return body;
    }

    void handle(QTcpSocket *socket, const QByteArray &method, const QString &path,
                const QMap<QByteArray, QByteArray> &headers)
    {
        if (requireAuth && headers.value("authorization") != "Basic " + QByteArray("viewer:secret").toBase64())
        {
            ++challenges;
            respond(socket, 401, {}, "text/plain");
            return;
        }
        if (method == "PROPFIND")
        {
            ++listings;
            respond(socket, directoryStatus, listing(), "application/xml; charset=utf-8");
            return;
        }
        ++downloads[path];
        if (holdDownloads) return;
        for (const File &file : files)
        {
            if (path == "/dav/" + file.name)
            {
                respond(socket, file.status, file.data, "text/plain", file.truncated);
                return;
            }
        }
        respond(socket, 404, {}, "text/plain");
    }

    QTcpServer server;
    QMap<QTcpSocket *, QByteArray> buffers;
};

struct LoadResult
{
    int count = 0;
    QString video;
    QList<WebDAVSubtitleLoader::Subtitle> subtitles;
    int failures = 0;
    QString error;

    void watch(WebDAVSubtitleLoader &loader)
    {
        QObject::connect(&loader, &WebDAVSubtitleLoader::finished, &loader,
                         [this](const QString &video, const QList<WebDAVSubtitleLoader::Subtitle> &subtitles,
                                int failures, const QString &error) {
            ++count;
            this->video = video;
            this->subtitles = subtitles;
            this->failures = failures;
            this->error = error;
        });
    }
};

class WebDAVSubtitleLoaderTest : public QObject
{
    Q_OBJECT
private slots:
    void matching_data()
    {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<QStringList>("expected");
        QTest::newRow("disabled") << QString("no") << QStringList{};
        QTest::newRow("exact") << QString("exact") << QStringList{
            "Episode.01.chs.ASS", "Episode.01.en.srt", "Episode.01.ssa"};
        QTest::newRow("fuzzy") << QString("fuzzy") << QStringList{
            "Episode.01.chs.ASS", "Episode.01.en.srt", "Episode.01.notes.ass", "Episode.01.ssa",
            "Episode.010.ass", "prefix Episode.01.vtt"};
        QTest::newRow("all") << QString("all") << QStringList{
            "Episode.01.chs.ASS", "Episode.01.en.srt", "Episode.01.notes.ass", "Episode.01.ssa",
            "Episode.010.ass", "Other.vtt", "prefix Episode.01.vtt"};
    }

    void matching()
    {
        QFETCH(QString, mode);
        QFETCH(QStringList, expected);
        DavServer server;
        QVERIFY(server.isListening());
        server.files = {{"Other.vtt"}, {"Episode.01.ssa"}, {"Episode.010.ass"},
                        {"Episode.01.chs.ASS"}, {"Episode.01.notes.ass"}, {"Episode.01.en.srt"},
                        {"Episode.01.txt"}, {"prefix Episode.01.vtt"}, {"Episode.01.ass/"}};
        QTemporaryDir cache(QDir::current().filePath("cache-XXXXXX"));
        QVERIFY(cache.isValid());
        LoadResult result;
        WebDAVSubtitleLoader loader(cache.path());
        result.watch(loader);
        const QString video = server.url("Episode.01.mkv");
        loader.start(video, server.url(), "", "", mode);
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 1, 5000);
        QCOMPARE(result.video, video);
        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QCOMPARE(result.failures, 0);
        QStringList titles;
        for (const auto &sub : result.subtitles)
        {
            titles.append(sub.title);
            QFile file(sub.path);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), QByteArray("subtitle contents\n"));
            QCOMPARE(QFileInfo(sub.path).suffix(), QFileInfo(sub.title).suffix().toLower());
        }
        QCOMPARE(titles, expected);
        QCOMPARE(server.listings, mode == "no" ? 0 : 1);
        QCOMPARE(server.downloads.size(), expected.size());
    }

    void legacyChineseSuffixes()
    {
        const QString baseName = "[HKG][GendaiMahou][BDrip][01][x264-Hi10P_FLAC][1920x1080][CRC32_2D1967FF]";
        DavServer server;
        QVERIFY(server.isListening());
        server.files = {{baseName + ".uni_gb.ass"}, {baseName + ".uni_big5.ass"},
                        {baseName + ".uni_notes.ass"}, {baseName + ".notes.ass"}};
        QTemporaryDir cache(QDir::current().filePath("cache-XXXXXX"));
        QVERIFY(cache.isValid());
        LoadResult result;
        WebDAVSubtitleLoader loader(cache.path());
        result.watch(loader);
        loader.start(server.url(baseName + ".mkv"), server.url(), "", "", "exact");
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 1, 5000);
        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QCOMPARE(result.failures, 0);
        QStringList titles;
        for (const auto &sub : result.subtitles) titles.append(sub.title);
        QCOMPARE(titles, QStringList({baseName + ".uni_big5.ass", baseName + ".uni_gb.ass"}));
        QCOMPARE(server.downloads.size(), 2);
    }

    void cacheAndPartialFailure()
    {
        DavServer server;
        QVERIFY(server.isListening());
        server.files = {{"Episode.01.ass"}, {"Episode.01.en.srt"}};
        server.files[1].status = 404;
        QTemporaryDir cache(QDir::current().filePath("cache-XXXXXX"));
        LoadResult result;
        WebDAVSubtitleLoader loader(cache.path());
        result.watch(loader);
        const auto start = [&]() { loader.start(server.url("Episode.01.mkv"), server.url(), "", "", "exact"); };
        start();
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 1, 5000);
        QCOMPARE(result.subtitles.size(), 1);
        QCOMPARE(result.failures, 1);
        const QString firstCache = result.subtitles.first().path;
        start();
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 2, 5000);
        QCOMPARE(result.subtitles.first().path, firstCache);
        QCOMPARE(server.downloads.value("/dav/Episode.01.ass"), 1);
        QCOMPARE(server.downloads.value("/dav/Episode.01.en.srt"), 2);
        server.files[0].modified = "Thu, 29 Oct 2026 12:00:00 GMT";
        start();
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 3, 5000);
        QCOMPARE(server.downloads.value("/dav/Episode.01.ass"), 2);
        QVERIFY(result.subtitles.first().path != firstCache);
        QVERIFY(QFile::remove(result.subtitles.first().path));
        start();
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 4, 5000);
        QCOMPARE(server.downloads.value("/dav/Episode.01.ass"), 3);
        QCOMPARE(QDir(cache.path()).entryList(QDir::Files).size(), 2);
    }

    void authenticationAndEncodedNames()
    {
        DavServer server;
        QVERIFY(server.isListening());
        server.requireAuth = true;
        server.absoluteHref = true;
        const QString baseName = QStringLiteral("第 01 集 #100%");
        server.files = {{baseName + ".chs.ass"}};
        QTemporaryDir cache(QDir::current().filePath("cache-XXXXXX"));
        LoadResult result;
        WebDAVSubtitleLoader loader(cache.path());
        result.watch(loader);
        loader.start(server.url(baseName + ".mkv"), server.url(), "viewer", "secret", "exact");
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 1, 5000);
        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QCOMPARE(result.failures, 0);
        QCOMPARE(result.subtitles.size(), 1);
        QCOMPARE(result.subtitles.first().title, server.files.first().name);
        QCOMPARE(server.downloads.value("/dav/" + server.files.first().name), 1);
        QVERIFY(server.challenges > 0);
    }

    void cancelAndRestart()
    {
        DavServer server;
        QVERIFY(server.isListening());
        server.files = {{"Episode.01.ass"}, {"Episode.02.ass"}};
        server.holdDownloads = true;
        QTemporaryDir cache(QDir::current().filePath("cache-XXXXXX"));
        LoadResult result;
        WebDAVSubtitleLoader loader(cache.path());
        result.watch(loader);
        loader.start(server.url("Episode.01.mkv"), server.url(), "", "", "exact");
        QTRY_COMPARE_WITH_TIMEOUT(server.downloads.value("/dav/Episode.01.ass"), 1, 5000);
        loader.cancel();
        loader.start(server.url("Episode.01.mkv"), server.url(), "", "", "exact");
        QTRY_COMPARE_WITH_TIMEOUT(server.downloads.value("/dav/Episode.01.ass"), 2, 5000);
        server.holdDownloads = false;
        const QString secondVideo = server.url("Episode.02.mkv");
        loader.start(secondVideo, server.url(), "", "", "exact");
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 1, 5000);
        QCOMPARE(result.video, secondVideo);
        QCOMPARE(result.subtitles.size(), 1);
        QCOMPARE(result.subtitles.first().title, QString("Episode.02.ass"));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(QDir(cache.path()).entryList(QDir::Files).size(), 1);
    }

    void requestFailures()
    {
        DavServer server;
        QVERIFY(server.isListening());
        server.files = {{"Episode.01.ass"}};
        server.directoryStatus = 500;
        QTemporaryDir cache(QDir::current().filePath("cache-XXXXXX"));
        LoadResult result;
        WebDAVSubtitleLoader loader(cache.path());
        result.watch(loader);
        loader.start(server.url("Episode.01.mkv"), server.url(), "", "", "exact");
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 1, 5000);
        QVERIFY(!result.error.isEmpty());
        QVERIFY(result.subtitles.isEmpty());
        QVERIFY(server.downloads.isEmpty());
        server.directoryStatus = 207;
        server.files[0].truncated = true;
        loader.start(server.url("Episode.01.mkv"), server.url(), "", "", "exact");
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 2, 5000);
        QCOMPARE(result.failures, 1);
        QVERIFY(result.subtitles.isEmpty());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(QDir(cache.path()).entryList(QDir::Files).isEmpty());
        server.downloads.clear();
        server.files[0].declaredSize = 21 * 1024 * 1024;
        loader.start(server.url("Episode.01.mkv"), server.url(), "", "", "all");
        QTRY_COMPARE_WITH_TIMEOUT(result.count, 3, 5000);
        QCOMPARE(result.failures, 1);
        QVERIFY(result.subtitles.isEmpty());
        QVERIFY(server.downloads.isEmpty());
    }
};

QTEST_GUILESS_MAIN(WebDAVSubtitleLoaderTest)
#include "tst_webdavsubtitleloader.moc"

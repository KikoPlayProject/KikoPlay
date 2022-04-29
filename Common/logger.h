#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QTimer>
#include <QSharedPointer>
#include <QThread>
#include <QFile>

class Logger : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Logger)
public:
    enum LogType
    {
        APP, MPV, LANServer, Script, Aria2, UNKNOWN
    };
    const QStringList LogTypeNames{"App", "MPV", "LAN Server", "Script", "Aria2"};
    const int bufferSize = 1024;
    const qint64 maxLogFileSize = 10*1024*1024;
    virtual ~Logger();
public:
    static Logger *logger();
    void log(LogType logType, const QString &message, const QtMsgType type, const QMessageLogContext& context);
    void log(LogType logType, const QString &message);
    void log(LogType logType, const char *format, ...);
    const QStringList &getLogs(LogType type) const {assert(type!=UNKNOWN);return logBuffer[(int)type];}

signals:
    void logAppend(Logger::LogType type, const QString &log);

protected:
    void timerEvent(QTimerEvent* event);

private:
    static QSharedPointer<Logger> defaultLogger;
    explicit Logger(QObject *parent = nullptr);
    static void msgHandler(const QtMsgType type, const QMessageLogContext& context, const QString &message);

    QStringList logBuffer[(int)LogType::UNKNOWN];
    QString typeIdentity[(int)LogType::UNKNOWN] = {"app", "mpv", "server", "script", "aria2"};
    QString timestampFormat{"yyyy-MM-dd hh:mm:ss"};
    QSharedPointer<QThread> logThread;
    QBasicTimer flushTimer;
    QString logFolder;
    int flushPos[(int)LogType::UNKNOWN] = {0};
    QSharedPointer<QFile> logFiles[(int)LogType::UNKNOWN];

    void refreshLogFile();
    void writeLogFile(LogType type);
};

#endif // LOGGER_H

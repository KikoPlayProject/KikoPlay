#include "logger.h"
#include <QDateTime>
#include <QTimerEvent>
#include <QDir>
#include <cstdarg>
#include <QRegularExpression>
#include "globalobjects.h"

QSharedPointer<Logger> Logger::defaultLogger = nullptr;

Logger *Logger::logger()
{
    if(!defaultLogger)
    {
        qRegisterMetaType<Logger::LogType>("Logger::LogType");
        defaultLogger = QSharedPointer<Logger>(new Logger());
    }
    return defaultLogger.data();
}

void Logger::log(Logger::LogType logType, const QString &message, const QtMsgType type, const QMessageLogContext &context)
{
    QString msgType;
    switch (type)
    {
    case QtDebugMsg:
        msgType = "DEBUG";
        break;
    case QtWarningMsg:
        msgType = "WARNING";
        break;
    case QtCriticalMsg:
        msgType = "CRITICAL";
        break;
    case QtFatalMsg:
        msgType = "FATAL";
        break;
    case QtInfoMsg:
        msgType = "INFO";
        break;
    }
    QString logContent("[%1][%2:%3]%4");
    logContent = logContent.arg(msgType, context.file, QString::number(context.line), message);
    log(logType, logContent);
}

void Logger::log(Logger::LogType logType, const QString &message)
{
    QString logContent(QString("[%1]%2").arg(QDateTime::currentDateTime().toString(timestampFormat), message));
    assert(logType != LogType::UNKNOWN);
    QMetaObject::invokeMethod(this, [=](){
        logBuffer[(int)logType].push_back(logContent);
        emit logAppend(logType,  logContent);
    }, Qt::ConnectionType::QueuedConnection);
}

void Logger::log(Logger::LogType logType, const char *format, ...)
{
    va_list argp;
    va_start(argp, format);
    QString msg = QString::vasprintf(format, argp);
    va_end(argp);
    log(logType, msg);
}

void Logger::timerEvent(QTimerEvent *event)
{
    if (event->timerId()==flushTimer.timerId())
    {
        for(int i = 0; i < LogType::UNKNOWN; ++i)
        {
            if(flushPos[i] < logBuffer[i].size())
            {
                writeLogFile((LogType)i);
            }
        }
    }
}

Logger::Logger(QObject *parent) : QObject(parent)
{
    logFolder = QString("%1/log/").arg(GlobalObjects::context()->dataPath);

    qInstallMessageHandler(msgHandler);
    logThread=QSharedPointer<QThread>::create();
    logThread->setObjectName(QStringLiteral("logThread"));
    logThread->start(QThread::NormalPriority);
    moveToThread(logThread.data());
    QMetaObject::invokeMethod(this, [=](){
        refreshLogFile();
        flushTimer.start(10000, this);
    });
}

Logger::~Logger()
{
    logThread->quit();
    logThread->wait();
}

void Logger::msgHandler(const QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    defaultLogger->log(LogType::APP, message, type, context);
}

void Logger::refreshLogFile()
{
    QDir folder;
    if(!folder.exists(logFolder))
        folder.mkpath(logFolder);
    folder.setPath(logFolder);

    QRegularExpression re("\\d{4}-\\d{2}-\\d{2}");
    qint64 curTime = QDateTime::currentDateTime().toSecsSinceEpoch();
    for (QFileInfo fileInfo : folder.entryInfoList())
    {
        if (fileInfo.isFile() && fileInfo.suffix().toLower()=="log")
        {
            auto match = re.match(fileInfo.fileName());
            if (!match.hasMatch() || match.capturedStart(0) != 0) continue;
            //if(re.indexIn(fileInfo.fileName())!=0) continue;
            int logTime = QDateTime::fromString(match.captured(0), "yyyy-MM-dd").toSecsSinceEpoch();
            if(curTime - logTime > 3*24*3600)
            {
                folder.remove(fileInfo.fileName());
                log(LogType::APP, QString("Remove Log File: %1").arg(fileInfo.fileName()));
            }
        }
    }
}

void Logger::writeLogFile(Logger::LogType type)
{
    QString datePrefix = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString logFileName = QString("%1/%2_%3.log").arg(logFolder, datePrefix, typeIdentity[(int)type]);
    if(!logFiles[(int)type] || logFiles[(int)type]->fileName()!=logFileName)
    {
        logFiles[(int)type] = QSharedPointer<QFile>::create(logFileName);
        if(!logFiles[(int)type]->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            logFiles[(int)type] = nullptr;
            return;
        }
    }
    if(logFiles[(int)type]->size() > maxLogFileSize)
    {
         logFiles[(int)type]->remove();
         logFiles[(int)type] = QSharedPointer<QFile>::create(logFileName);
         if(!logFiles[(int)type]->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
         {
             logFiles[(int)type] = nullptr;
             return;
         }
         log(LogType::APP, QString("Reset Big Log File: %1").arg(logFileName));
    }
    auto &curLogBuffer = logBuffer[(int)type];
    for(int i = flushPos[(int)type]; i < curLogBuffer.size(); ++i)
    {
        logFiles[(int)type]->write(curLogBuffer[i].toUtf8().constData());
        logFiles[(int)type]->write("\n");
    }
    logFiles[(int)type]->flush();
    while(curLogBuffer.size() > bufferSize)
    {
        curLogBuffer.pop_front();
    }
    flushPos[(int)type] = curLogBuffer.size();
}


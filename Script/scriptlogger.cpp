#include "scriptlogger.h"
#include <QTime>

void ScriptLogger::appendLog(const QString &log)
{
    QMetaObject::invokeMethod(this, [=](){
        logs.append(log);
        if(logs.size()>maxCount) logs.removeFirst();
        emit logging(log);
    }, Qt::ConnectionType::QueuedConnection);
}

void ScriptLogger::appendError(const QString &info, const QString &extra)
{
    appendLog(QString("[Error][%1]: %2").arg(extra, info));
}

void ScriptLogger::appendInfo(const QString &info, const QString &scriptId)
{
    appendLog(QString("[%1][%2]: %3").arg(QTime::currentTime().toString("hh:mm:ss"), scriptId, info));
}

void ScriptLogger::appendText(const QString &text)
{
    appendLog(text);
}

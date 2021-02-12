#ifndef SCRIPTLOGGER_H
#define SCRIPTLOGGER_H
#include <QObject>

class ScriptLogger : public QObject
{
    Q_OBJECT
private:
    explicit ScriptLogger(QObject *parent=nullptr) : QObject(parent) {}
    ScriptLogger(const ScriptLogger &);
    QStringList logs;
    const int maxCount = 1024;
    inline void appendLog(const QString &log);
public:
    static ScriptLogger *instance()
    {
        static ScriptLogger logger;
        return &logger;
    }

    void appendError(const QString &info, const QString &extra);
    void appendInfo(const QString &info, const QString &scriptId);
    void appendText(const QString &text);

    const QStringList &getLogs() const {return logs;}
signals:
    void logging(const QString &log);
};

#endif // SCRIPTLOGGER_H

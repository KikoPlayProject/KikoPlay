#ifndef DBMANAGER_H
#define DBMANAGER_H
#include <QThreadStorage>
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QString>
#include <QList>

struct DBConn
{
    QSqlDatabase db;

    DBConn(const QSqlDatabase &_db) : db(_db) {}
    ~DBConn() { db.close(); };
};

class DBManager
{
    Q_DISABLE_COPY(DBManager)
    DBManager();

public:
    static DBManager *instance();

    enum DBType
    {
        Comment,
        Bangumi,
        Download,
        UNKNOWN,
    };

    QSqlDatabase getDB(DBType db);

private:
    const QStringList dbFiles{ "comment","bangumi","download" };
    QThreadStorage<QSharedPointer<DBConn>> dbManagers[DBType::UNKNOWN];
    QSqlDatabase openDBConn(DBType db);
};

#endif // DBMANAGER_H

#include "dbmanager.h"
#include "globalobjects.h"
#include <QSqlQuery>
#include <QSqlError>
#include "logger.h"

DBManager::DBManager()
{
    for (int i = 0; i < DBType::UNKNOWN; ++i)
    {
        QString fn(GlobalObjects::context()->dataPath + dbFiles[i] + ".db");
        if (!QFile::exists(fn))
        {
            QSqlDatabase db = getDB(DBType(i));
            QFile sqlFile(QString(":/res/db/%1.sql").arg(dbFiles[i]));
            sqlFile.open(QFile::ReadOnly);
            if (sqlFile.isOpen())
            {
                QSqlQuery query(db);
                QStringList sqls = QString(sqlFile.readAll()).split(';', Qt::SkipEmptyParts);
                for (const QString &sql : sqls)
                {
                    query.exec(sql);
                }
            }
        }
    }
}

DBManager *DBManager::instance()
{
    static DBManager manager;
    return &manager;
}

QSqlDatabase DBManager::getDB(DBType db)
{
    if (!dbManagers[db].hasLocalData())
    {
        dbManagers[db].setLocalData(QSharedPointer<DBConn>::create(openDBConn(db)));
    }
    return dbManagers[db].localData().get()->db;
}

QSqlDatabase DBManager::openDBConn(DBType db)
{
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", QString("%1-%2").arg(dbFiles[db]).arg(reinterpret_cast<int64_t>(QThread::currentThreadId())));
    QString dbFile(GlobalObjects::context()->dataPath + dbFiles[db] + ".db");
    database.setDatabaseName(dbFile);
    if (!database.open())
    {
        Logger::logger()->log(Logger::APP, database.lastError().text());
        return database;
    }
    QSqlQuery query(database);
    query.exec("PRAGMA foreign_keys = ON;");
    return database;
}

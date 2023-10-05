#include "appstorage.h"
#include "Common/logger.h"
#include <QFile>
#include <QDataStream>
#include <QCoreApplication>
#include <QEvent>

namespace Extension
{

AppStorage::AppStorage(const QString &path, QObject *parent) : QObject(parent), counter(0), storagePath(path)
{
    QFile storageFile(storagePath);
    if (storageFile.exists())
    {
        bool ret = storageFile.open(QIODevice::ReadOnly);
        if(!ret)
        {
            Logger::logger()->log(Logger::Extension, QString("[storage]open failed: %1").arg(storagePath));
        }
        else
        {
            QDataStream fs(&storageFile);
            fs >> storageHash;
        }
    }
}

AppStorage::~AppStorage()
{
    flush();
}

void AppStorage::set(const QString &key, const QVariant &val)
{
    if (!val.isValid())
    {
        storageHash.remove(key);
    }
    else
    {
        storageHash[key] = val;
    }
    if (++counter > 0)
    {
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }

}

QVariant AppStorage::get(const QString &key)
{
    return storageHash.value(key, QVariant());
}

void AppStorage::flush()
{
    if (counter > 0)
    {
        QFile storageFile(storagePath);
        bool ret = storageFile.open(QIODevice::WriteOnly);
        if (!ret)
        {
            Logger::logger()->log(Logger::Extension, QString("[storage]flush failed: %1").arg(storagePath));
            return;
        }
        QDataStream fs(&storageFile);
        fs << storageHash;
        storageFile.flush();
        counter = 0;
    }
}

bool AppStorage::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest)
    {
        flush();
        return true;
    }
    return QObject::event(event);
}

}

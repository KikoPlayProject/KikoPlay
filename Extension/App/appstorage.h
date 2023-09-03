#ifndef APPSTORAGE_H
#define APPSTORAGE_H
#include <QHash>
#include <QVariant>
#include "Extension/Common/ext_common.h"

namespace Extension
{

class AppStorage : public QObject
{
    Q_OBJECT
public:
    AppStorage(const QString &path, QObject *parent = nullptr);
    ~AppStorage();

    void set(const QString &key, const QVariant &val);
    QVariant get(const QString &key);

private:
    QHash<QString, QVariant> storageHash;
    int counter;
    QString storagePath;

    void flush();

    // QObject interface
public:
    bool event(QEvent *event);
};

struct StorageRes : public AppRes
{
    StorageRes(AppStorage *s) : storage(s) {}
    ~StorageRes() {  if (storage)  storage->deleteLater(); }
    AppStorage *storage;
};

}

#endif // APPSTORAGE_H

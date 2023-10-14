#ifndef KUPDATER_H
#define KUPDATER_H

#include <QObject>

class KUpdater : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KUpdater)
    explicit KUpdater(QObject *parent = nullptr);
public:
    static KUpdater *instance();

    void check();
    bool needCheck();
    bool hasNewVersion() const { return hasNVer; }
    const QString &version() const { return nVer; }
    const QString &versionURL() const { return nVerURL; }
    const QString &vesrionDescription() const { return nVerDesc; }
    const QString &errInfo() const { return errorInfo; }
signals:
    void checkDone();
private:
    bool hasNVer;
    QString nVer;
    QString nVerURL;
    QString nVerDesc;
    QString errorInfo;
    bool hasNewVersion(const QString &nv);
};

#endif // KUPDATER_H

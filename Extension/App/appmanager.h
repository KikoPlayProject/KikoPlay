#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QAbstractItemModel>
#include <QSharedPointer>
#include "kapp.h"

class AppManager : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AppManager(QObject *parent = nullptr);
    ~AppManager();
    void refresh(bool inWorkerThread = false);
    void start(const QModelIndex &index);
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const{return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const {return parent.isValid() ? 0 : appList.count();}
    virtual int columnCount(const QModelIndex &parent) const {return parent.isValid() ? 0 : 1;}
    virtual QVariant data(const QModelIndex &index, int role) const;
signals:
    void appLaunched(Extension::KApp *app);
    void appTerminated(Extension::KApp *app);
    void appFlash(Extension::KApp *app);
private:
    QVector<QSharedPointer<Extension::KApp>> appList;
    QString getAppPath() const;
    QVector<QSharedPointer<Extension::KApp> > refresApp();
};

#endif // APPMANAGER_H

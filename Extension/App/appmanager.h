#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QSet>
#include "kapp.h"

class AppManager : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AppManager(QObject *parent = nullptr);
    ~AppManager();
    void refresh(bool inWorkerThread = false);
    void start(const QModelIndex &index);
    void autoStart();
    void closeAll();
public:
    enum struct Columns
    {
        NAME,
        VERSION,
        DESC,
        AUTO_START,
        NONE
    };
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:(int)Columns::NONE;}
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const{return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const {return parent.isValid() ? 0 : appList.count();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
signals:
    void appLaunched(Extension::KApp *app);
    void appTerminated(Extension::KApp *app);
    void appFlash(Extension::KApp *app);
private:
    QVector<QSharedPointer<Extension::KApp>> appList;
    QString getAppPath() const;
    QVector<QSharedPointer<Extension::KApp> > refresApp();
    QSet<QString> autoStartAppIds;
};

#endif // APPMANAGER_H

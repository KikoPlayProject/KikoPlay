#ifndef EPISODESMODEL_H
#define EPISODESMODEL_H
#include <QAbstractItemModel>
#include "animeinfo.h"
class EpisodesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit EpisodesModel(Anime *anime, QObject *parent = nullptr);
    void setAnime(Anime *anime);
    enum class Columns
    {
        TITLE=0, LASTPLAY, FINISHTIME, LOCALFILE
    };
    static const int EpRole = Qt::UserRole+1;
private:
    Anime *currentAnime;
    QList<EpInfo> currentEps;
    QMap<QString, int> epMap;
    const QStringList headers={tr("Title"),tr("Last Play"), tr("Finish Time"), tr("LocalFile")};
public slots:
    void addEp(const QString &path);
    void removeEp(const QModelIndex &index);
    void updateEpInfo(const QModelIndex &index, const EpInfo &nEpInfo);
    void updateFinishTime(const QModelIndex &index, qint64 nTime);
    // QAbstractItemModel interface
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:(currentAnime?currentEps.count():0);}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:headers.size();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // EPISODESMODEL_H

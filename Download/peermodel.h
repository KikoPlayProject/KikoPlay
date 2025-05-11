#ifndef PEERMODEL_H
#define PEERMODEL_H
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include <QAbstractItemModel>

class PeerModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    PeerModel(QObject *parent = nullptr);
    void setPeers(const QJsonArray &statusObj, int numPieces);
    void clear();
    static constexpr const int ProgressCluster = 1024;
    struct PeerInfo
    {
        QString client, ip;
        QByteArray peerId;
        float progressPercent;
        unsigned char progress[PeerModel::ProgressCluster/8];
        unsigned downspeed, upspeed;
    };
    enum class Columns
    {
        CLIENT, IP, PROGRESS, DOWNSPEED, UPSPEED
    };
private:
    const QStringList headers={tr("Client"),tr("IP"),tr("Progress"),tr("DownSpeed"), tr("UpSpeed")};


    QList<QSharedPointer<PeerInfo>> peers;
    int currentPiecesNum = 0;
    void setProgress(PeerInfo &peer, const QString &progressStr);
public:
    inline virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    inline virtual QModelIndex parent(const QModelIndex &) const {return QModelIndex();}
    inline virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:peers.count();}
    inline virtual int columnCount(const QModelIndex &parent) const{return parent.isValid()?0:headers.size();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};
class PeerDelegate : public KTreeviewItemDelegate
{
    Q_OBJECT
public:
    PeerDelegate(QObject *parent = nullptr) : KTreeviewItemDelegate(parent) {}
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QColor barColor = QColor(51,168,255,200), borderColor = QColor(200, 200, 200), backgroundcolor = QColor(255,255,255,120);
};

#endif // PEERMODEL_H

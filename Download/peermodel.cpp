#include "peermodel.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include "peerid.h"
#include "util.h"
#define PiecesNumRole Qt::UserRole+1
PeerModel::PeerModel(QObject *parent) : QAbstractItemModel(parent)
{

}

void PeerModel::setPeers(const QJsonArray &peerArray, int numPieces)
{
    static QSet<QByteArray> curPeerIds;
    static QHash<QByteArray, int> peerRow;
    curPeerIds.clear();
    peerRow.clear();
    int curPeerCount = peers.size();
    for(int i=0;i<curPeerCount;++i)
    {
        peerRow[peers.at(i)->peerId] = i;
    }
    currentPiecesNum = numPieces;
    for(auto iter=peerArray.begin();iter!=peerArray.end();++iter)
    {
        QJsonObject peerObj=(*iter).toObject();
        QByteArray peerId(QByteArray::fromPercentEncoding(peerObj.value("peerId").toString().toLatin1()));
        curPeerIds.insert(peerId);
        if(peerRow.contains(peerId))
        {
            int row = peerRow[peerId];
            auto peer = peers[row];
            peer->downspeed = peerObj.value("downloadSpeed").toString().toInt();
            peer->upspeed = peerObj.value("uploadSpeed").toString().toInt();
            setProgress(*peer, peerObj.value("bitfield").toString());
            emit dataChanged(index(row, static_cast<int>(Columns::CLIENT),QModelIndex()),
                             index(row, static_cast<int>(Columns::UPSPEED),QModelIndex()));
        }
        else
        {
            PeerInfo *peer = new PeerInfo;
            peer->ip = peerObj.value("ip").toString();
            peer->peerId = peerId;
            peer->client = PeerId::convertPeerId(peerId);
            peer->downspeed = peerObj.value("downloadSpeed").toString().toInt();
            peer->upspeed = peerObj.value("uploadSpeed").toString().toInt();
            setProgress(*peer, peerObj.value("bitfield").toString());

            int insertPosition = peers.count();
            beginInsertRows(QModelIndex(), insertPosition, insertPosition);
            peers.append(QSharedPointer<PeerInfo>(peer));
            endInsertRows();
        }
    }
    for(int i=curPeerCount-1;i>=0;--i)
    {
        auto peer = peers[i];
        if(!curPeerIds.contains(peer->peerId))
        {
            beginRemoveRows(QModelIndex(), i, i);
            peers.removeAt(i);
            endRemoveRows();
        }
    }

}

void PeerModel::clear()
{
    if(peers.size()>0)
    {
        beginResetModel();
        peers.clear();
        endResetModel();
    }
}

void PeerModel::setProgress(PeerModel::PeerInfo &peer, const QString &progressStr)
{
    int pos = 0, count = 0;
    const int clusters = qMin(currentPiecesNum, PeerModel::ProgressCluster);
    memset(peer.progress, 0, sizeof peer.progress);
    if(clusters==0)
    {
        peer.progressPercent = 0;
        return;
    }
    float percent = 1.0 / currentPiecesNum;
    auto bitSet = [&](int p){
        peer.progress[p/8] |= (1<<(7-p%8));
    };
    for(QChar c: progressStr){
        int n = c.toLatin1();
        if(c.isDigit()) n = n-'0';
        else if('a'<=n && 'f'>=n) n = n-'a'+10;
        if(n >=0 && n < 16){
            if(n & 0x8) { ++count; bitSet(pos * percent * clusters);}
            if(n & 0x4) { ++count; bitSet((pos+1) * percent * clusters);}
            if(n & 0x2) { ++count; bitSet((pos+2) * percent * clusters);}
            if(n & 0x1) { ++count; bitSet((pos+3) * percent * clusters);}
        }
        pos += 4;
    }
    peer.progressPercent = count * percent * 100;
}

QVariant PeerModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    const auto &peer=peers.at(index.row());
    Columns col=static_cast<Columns>(index.column());
    if(role==Qt::DisplayRole)
    {
    switch (col)
    {
    case Columns::CLIENT:
        return peer->client;
    case Columns::IP:
        return peer->ip;
    case Columns::PROGRESS:
        return QVariant::fromValue((void *)peer.data());
    case Columns::DOWNSPEED:
        return formatSize(true, peer->downspeed);
    case Columns::UPSPEED:
        return formatSize(true, peer->upspeed);
    default:
        break;
    }
    }
    if(role==PiecesNumRole)
    {
        return currentPiecesNum;
    }
    return QVariant();
}

QVariant PeerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.count())return headers.at(section);
    }
    return QVariant();
}

void PeerDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption, index);
    if (option.state.testFlag(QStyle::State_HasFocus))
        viewOption.state = viewOption.state ^ QStyle::State_HasFocus;

    QStyledItemDelegate::paint(painter, viewOption, index);
    if(index.column()==static_cast<int>(PeerModel::Columns::PROGRESS))
    {
        const int clusters = qMin(index.model()->data(index, PiecesNumRole).toInt(), PeerModel::ProgressCluster);
        const PeerModel::PeerInfo *peer = (const PeerModel::PeerInfo *)index.data().value<void *>();
        QRect bRect = viewOption.rect;
        bRect.adjust(1, 1, -1, -1);
        painter->setPen(borderColor);
        painter->setBrush(QBrush(QColor(0, 0, 0, 0)));
        painter->drawRect(bRect);
        float wRatio = (bRect.width() - painter->pen().widthF()) / clusters;
        float h = bRect.height() - painter->pen().widthF();
        float x = bRect.x()+painter->pen().widthF();
        float y = bRect.y()+painter->pen().widthF();
        for(int i=0;i<clusters;++i)
        {
            if(peer->progress[i/8] & (1<<(7-i%8)))
            {
                painter->fillRect(QRectF(x, y, wRatio,h), barColor);
            }
            x += wRatio;
        }
        painter->setPen(Qt::black);
        painter->drawText(bRect, Qt::AlignCenter, QString("%1%").arg(peer->progressPercent,0,'g',3));
    }
    painter->restore();
}

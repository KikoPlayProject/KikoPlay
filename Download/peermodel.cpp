#include "peermodel.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include "peerid.h"
#include "util.h"
#include "UI/stylemanager.h"
#define PiecesNumRole Qt::UserRole+1

constexpr const int PeerModel::ProgressCluster;

PeerModel::PeerModel(QObject *parent) : QAbstractItemModel(parent)
{

}

void PeerModel::setPeers(const QJsonArray &peerArray, int numPieces)
{
    static QSet<QString> curPeerIds;
    static QHash<QString, int> peerRow;
    curPeerIds.clear();
    peerRow.clear();
    int curPeerCount = peers.size();
    for(int i=0;i<curPeerCount;++i)
    {
        peerRow[peers.at(i)->ip] = i;
    }
    currentPiecesNum = numPieces;
    for(auto iter=peerArray.begin();iter!=peerArray.end();++iter)
    {
        QJsonObject peerObj=(*iter).toObject();
        QString ip(QString("%1:%2").arg(peerObj.value("ip").toString(), peerObj.value("port").toString()));
        curPeerIds.insert(ip);
        if(peerRow.contains(ip))
        {
            int row = peerRow[ip];
            auto peer = peers[row];
            peer->downspeed = peerObj.value("downloadSpeed").toString().toInt();
            peer->upspeed = peerObj.value("uploadSpeed").toString().toInt();
            if(peer->client.isEmpty())
            {
                peer->peerId = QByteArray::fromPercentEncoding(peerObj.value("peerId").toString().toLatin1());
                peer->client = PeerId::convertPeerId(peer->peerId);
            }
            setProgress(*peer, peerObj.value("bitfield").toString());
            emit dataChanged(index(row, static_cast<int>(Columns::CLIENT),QModelIndex()),
                             index(row, static_cast<int>(Columns::UPSPEED),QModelIndex()));
        }
        else
        {
            PeerInfo *peer = new PeerInfo;
            peer->ip = ip;
            peer->peerId = QByteArray::fromPercentEncoding(peerObj.value("peerId").toString().toLatin1());
            peer->client = PeerId::convertPeerId(peer->peerId);
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
        if(!curPeerIds.contains(peer->ip))
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
    const int clusters = qMin(currentPiecesNum, PeerModel::ProgressCluster);
    memset(peer.progress, 0, sizeof peer.progress);
    if(clusters==0)
    {
        peer.progressPercent = 0;
        return;
    }
    int pos = 0, count = 0;
    float percent = 1.0 / currentPiecesNum;
    static int bucket[ProgressCluster*2] = {};
    memset(bucket, 0, sizeof bucket);
    for(int i=0;i<currentPiecesNum;++i)  ++bucket[int(percent*clusters*i)];

    for(QChar c: progressStr){
        int n = c.toLatin1();
        if(c.isDigit()) n = n-'0';
        else if('a'<=n && 'f'>=n) n = n-'a'+10;
        if(n > 0 && n < 16){
            if(pos>=currentPiecesNum) break;
            if (n & 0x8) { ++count; ++bucket[int(percent * clusters * pos) + ProgressCluster]; }
            if(pos+1>=currentPiecesNum) break;
            if (n & 0x4) { ++count; ++bucket[int(percent * clusters * (pos + 1)) + ProgressCluster]; }
            if(pos+2>=currentPiecesNum) break;
            if (n & 0x2) { ++count; ++bucket[int(percent * clusters  * (pos + 2)) + ProgressCluster]; }
            if(pos+3>=currentPiecesNum) break;
            if (n & 0x1) { ++count; ++bucket[int(percent * clusters * (pos + 3)) + ProgressCluster]; }
        }
        pos += 4;
    }
    for(int i = 0;i<clusters;++i)
    {
        if(bucket[i+ProgressCluster]*2>bucket[i])
        {
            peer.progress[i/8] |= (1<<(7-i%8));
        }
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

void PeerDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    KTreeviewItemDelegate::paint(painter, option, index);
    painter->save();
    if (index.column() == static_cast<int>(PeerModel::Columns::PROGRESS))
    {
        const int clusters = qMin(index.model()->data(index, PiecesNumRole).toInt(), PeerModel::ProgressCluster);
        const PeerModel::PeerInfo* peer = (const PeerModel::PeerInfo*)index.data().value<void*>();
        QRect bRect = option.rect;
        bRect.adjust(1, 1, -1, -1);
        painter->setPen(borderColor);
        painter->setBrush(QBrush(backgroundcolor));
        painter->drawRect(bRect);
        float wRatio = (bRect.width() - painter->pen().widthF()) / clusters;
        float h = bRect.height() - painter->pen().widthF();
        float x = bRect.x() + painter->pen().widthF();
        float y = bRect.y() + painter->pen().widthF();
        float cx = x, cw = 0;
        QColor curBarColor = StyleManager::getStyleManager()->enableThemeColor() ? StyleManager::getStyleManager()->curThemeColor() : barColor;
        for (int i = 0; i < clusters; ++i)
        {
            if (peer->progress[i / 8] & (1 << (7 - i % 8)))
            {
                cw += wRatio;
            }
            else
            {
                if (cw > 0) painter->fillRect(QRectF(cx, y, cw, h), curBarColor);
                cx = qMax(x + (i + 1) * wRatio, cx + cw);
                cw = 0;
            }
        }
        if (cw > 0) painter->fillRect(QRectF(cx, y, cw, h), curBarColor);
        painter->setPen(Qt::black);
        painter->drawText(bRect, Qt::AlignCenter, QString("%1%").arg(peer->progressPercent, 0, 'g', 3));
    }
    painter->restore();
}

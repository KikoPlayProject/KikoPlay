#include "danmupool.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>
#include "eventanalyzer.h"
#include "Render/danmurender.h"
#include "globalobjects.h"
#include "blocker.h"
#include "Manager/danmumanager.h"
#include "Manager/pool.h"
#include "Play/Playlist/playlist.h"
namespace
{
    struct
    {
        inline bool operator ()(const QSharedPointer<DanmuComment> &danmu,int time) const
        {
            return danmu->time<time;
        }
    } DanmuComparer;
    struct
    {
        inline bool operator()(const QSharedPointer<DanmuComment> &dm1,const QSharedPointer<DanmuComment> &dm2) const
        {
            return dm1->time<dm2->time;
        }
    } DanmuSPCompare;
}
DanmuPool::DanmuPool(QObject *parent) : QAbstractItemModel(parent),curPool(nullptr), emptyPool(new Pool("","","",this)),
    currentPosition(0),currentTime(0),enableMerged(true),mergeInterval(15*1000),maxContentUnsimCount(4),minMergeCount(3)
{
    analyzer=new EventAnalyzer(this);
	setConnect(emptyPool);
}

DanmuPool::~DanmuPool()
{
    qDeleteAll(prepareListPool);
    delete emptyPool;
}

QSharedPointer<DanmuComment> DanmuPool::getDanmu(const QModelIndex &index)
{
    if(index.parent().isValid())
    {
        DanmuComment *p = static_cast<DanmuComment *>(index.parent().internalPointer());
        Q_ASSERT(p->mergedList && index.row()<p->mergedList->length());
        return (*p->mergedList).at(index.row());
    }
    else
    {
        return finalPool.at(index.row());
    }
}

void DanmuPool::cleanUp()
{
    if(curPool!=emptyPool)
    {
        setConnect(emptyPool);
    }
}

void DanmuPool::testBlockRule(BlockRule *rule)
{
    statisInfo.blockCount=0;
    for(QSharedPointer<DanmuComment> &danmu:danmuPool)
    {
        if(danmu->blockBy==-1)
        {
            if(rule->blockTest(danmu.data()))
                danmu->blockBy=rule->id;
        }
        else if(danmu->blockBy==rule->id)
        {
            if(!rule->blockTest(danmu.data()))
                danmu->blockBy=-1;
        }
        statisInfo.blockCount+=(danmu->blockBy==-1?0:1);
    }
    GlobalObjects::danmuRender->removeBlocked();
}

void DanmuPool::deleteDanmu(QSharedPointer<DanmuComment> danmu)
{
    if(!danmu->m_parent)
    {
        int row = finalPool.indexOf(danmu);
        beginRemoveRows(QModelIndex(), row, row);
        finalPool.removeAt(row);
        endRemoveRows();
        if(danmu->mergedList)
        {
           for(auto &c:*danmu->mergedList)
           {
               c->m_parent=nullptr;
           }
        }
    }
    else
    {
        int f_pos = std::lower_bound(finalPool.begin(), finalPool.end(), danmu->m_parent->time, DanmuComparer) - finalPool.begin();
        while(finalPool.at(f_pos)!=danmu->m_parent) f_pos++;
        Q_ASSERT(f_pos<finalPool.length());
        int c_pos=danmu->m_parent->mergedList->indexOf(danmu);
        beginRemoveRows(createIndex(f_pos,0,danmu->m_parent), c_pos, c_pos);
        danmu->m_parent->mergedList->removeAt(c_pos);
        endRemoveRows();
    }
	int row = danmuPool.indexOf(danmu);
    curPool->deleteDanmu(row);
	beginRemoveRows(QModelIndex(), row, row);
    danmuPool.removeAt(row);
    endRemoveRows();
    setStatisInfo();
}
void DanmuPool::setMerged()
{
#ifdef QT_DEBUG
    qDebug()<<"merge start";
    QElapsedTimer timer;
    timer.start();
#endif
    for(auto iter=danmuPool.cbegin();iter!=danmuPool.cend();++iter)
    {
        if((*iter)->mergedList)
        {
            delete (*iter)->mergedList;
            (*iter)->mergedList=nullptr;
        }
        if((*iter)->m_parent) (*iter)->m_parent=nullptr;
    }
    statisInfo.mergeCount=0;
    if(enableMerged)
    {
        finalPool.clear();
        QList<QSharedPointer<DanmuComment> > slideWindow;
        for(auto iter=danmuPool.cbegin();iter!=danmuPool.cend();++iter)
        {
            DanmuComment *cc((*iter).data());
            while(!slideWindow.isEmpty() && cc->time-slideWindow.first()->time>mergeInterval)
            {
                auto c(slideWindow.takeFirst());
                if(c->mergedList)
                {
                    if(c->mergedList->count()<minMergeCount)
                    {
                        finalPool.append(*c->mergedList);
                        for(auto &cc:*c->mergedList)
                            cc->m_parent=nullptr;
                        delete c->mergedList;
                        c->mergedList=nullptr;
                    }
                    else
                    {
                        statisInfo.mergeCount+=c->mergedList->count();
                    }
                }
                finalPool.append(c);
            }
            bool merged=false;
            for(int i=0;i<slideWindow.length();++i)
            {
                DanmuComment *sw(slideWindow.at(i).data());
                if(sw->type!=cc->type || qAbs(cc->text.length()-sw->text.length())>maxContentUnsimCount) continue;
                if((cc->text==sw->text) || contentSimilar(cc,sw))
                {
                    Q_ASSERT(!(*iter)->mergedList);
                    cc->m_parent=sw;
                    if(!sw->mergedList) sw->mergedList=new QList<QSharedPointer<DanmuComment> >();
                    sw->mergedList->append((*iter));
                    merged=true;
                    break;
                }
            }
            if(!merged)
            {
                slideWindow.append((*iter));
            }
        }
        finalPool.append(slideWindow);
        std::sort(finalPool.begin(),finalPool.end(),DanmuSPCompare);
    }
    else
    {
        finalPool=danmuPool;
    }
    currentPosition = std::lower_bound(finalPool.begin(), finalPool.end(), currentTime, DanmuComparer) - finalPool.begin();
#ifdef QT_DEBUG
    qDebug()<<"merge done:"<<timer.elapsed()<<"ms";
#endif
}

bool DanmuPool::contentSimilar(const DanmuComment *dm1, const DanmuComment *dm2)
{
    static QVector<int> charSpace(1<<16);
    int sz1=dm1->text.length(),sz2=dm2->text.length();
    for(int i=0;i<sz1;++i) charSpace[dm1->text.at(i).unicode()]++;
    for(int i=0;i<sz2;++i) charSpace[dm2->text.at(i).unicode()]--;
    int diff=0;
    for(int i=0;i<sz1;++i)
    {
        diff+=qAbs(charSpace[dm1->text.at(i).unicode()]);
        charSpace[dm1->text.at(i).unicode()]=0;
    }
    for(int i=0;i<sz2;++i)
    {
        diff+=qAbs(charSpace[dm2->text.at(i).unicode()]);
        charSpace[dm2->text.at(i).unicode()]=0;
    }
    return  diff<=maxContentUnsimCount;

}

void DanmuPool::setAnalyzation()
{
#ifdef QT_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    emit eventAnalyzeFinished(enableAnalyze?analyzer->analyze(curPool):QList<DanmuEvent>());
#ifdef QT_DEBUG
    qDebug()<<"Analyze time:"<<timer.elapsed();
#endif
}

void DanmuPool::setConnect(Pool *pool)
{
	if (curPool)
	{
		disconnect(curPool);
		curPool->setUsed(false);
	}
    curPool=pool;
    //poolID=curPool->id();
    reset();
    curPool->setUsed(true);
    beginResetModel();
    danmuPool=curPool->comments();
    setMerged();
    setStatisInfo();
    setAnalyzation();
    endResetModel();
    QObject::connect(curPool,&Pool::poolChanged,this,[this](bool ){
        beginResetModel();
        danmuPool=curPool->comments();
        setMerged();
        setAnalyzation();
        setStatisInfo();
        endResetModel();
        currentPosition = std::lower_bound(finalPool.begin(), finalPool.end(), currentTime, DanmuComparer) - finalPool.begin();
    });
}

void DanmuPool::setStatisInfo()
{
    statisInfo.countOfMinute.clear();
    statisInfo.maxCountOfMinute=0;
    statisInfo.blockCount=0;
    statisInfo.mergeCount=0;
    statisInfo.totalCount=danmuPool.count();
    int curMinuteCount=0;
    int startTime=0;
    for(auto iter=danmuPool.cbegin();iter!=danmuPool.cend();++iter)
    {
        if(iter==danmuPool.cbegin())
        {
            startTime=(*iter)->time;
        }
        if((*iter)->time-startTime<1000)
            curMinuteCount++;
        else
        {
            statisInfo.countOfMinute.append(QPair<int,int>(startTime/1000,curMinuteCount));
            if(curMinuteCount>statisInfo.maxCountOfMinute)
                statisInfo.maxCountOfMinute=curMinuteCount;
            curMinuteCount=1;
            startTime=(*iter)->time;
        }
        if((*iter)->blockBy!=-1)
            statisInfo.blockCount++;
        if((*iter)->mergedList)
            statisInfo.mergeCount+=(*iter)->mergedList->count();
    }
	statisInfo.countOfMinute.append(QPair<int, int>(startTime / 1000, curMinuteCount));
	if (curMinuteCount>statisInfo.maxCountOfMinute)
		statisInfo.maxCountOfMinute = curMinuteCount;
    emit statisInfoChange();

}

void DanmuPool::setAnalyzeEnable(bool enable)
{
    enableAnalyze = enable;
    setAnalyzation();
}

void DanmuPool::setMergeEnable(bool enable)
{
    if(enable!=enableMerged)
    {
        enableMerged=enable;
        beginResetModel();
        setMerged();
        endResetModel();
    }
}

void DanmuPool::setMergeInterval(int val)
{
    if(val!=mergeInterval)
    {
        mergeInterval=val;
        beginResetModel();
        setMerged();
        endResetModel();
    }
}

void DanmuPool::setMaxUnSimCount(int val)
{
    if(val!=maxContentUnsimCount)
    {
        maxContentUnsimCount=val;
        beginResetModel();
        setMerged();
        endResetModel();
    }
}

void DanmuPool::setMinMergeCount(int val)
{
    if(val!=minMergeCount)
    {
        minMergeCount=val;
        beginResetModel();
        setMerged();
        endResetModel();
    }
}

void DanmuPool::setPoolID(const QString &pid)
{
    if(pid==curPool->id()) return;
    Pool *pool = GlobalObjects::danmuManager->getPool(pid);
    if(pool) setConnect(pool);
    else if(curPool!=emptyPool) setConnect(emptyPool);
}

void DanmuPool::mediaTimeElapsed(int newTime)
{
    if(currentTime>newTime || newTime-currentTime>5000)
    {
        QCoreApplication::instance()->processEvents();
        currentTime=newTime;
        currentPosition=std::lower_bound(finalPool.begin(),finalPool.end(),currentTime,DanmuComparer)-finalPool.begin();
        return;
    }
    currentTime=newTime;
    PrepareList *prepareList(nullptr);
    prepareList=prepareListPool.isEmpty()?new PrepareList:prepareListPool.takeFirst();
    const int bundleSize=32;
    for(;currentPosition<finalPool.length();++currentPosition)
    {
        int curTime=finalPool.at(currentPosition)->time;
        if(curTime<0)continue;
        if(curTime<newTime)
        {
            auto dm=finalPool.at(currentPosition);
            if (dm->blockBy == -1 && curPool->sources()[dm->source].show)
			{
                prepareList->append(QPair<QSharedPointer<DanmuComment>,DanmuDrawInfo*>(dm,nullptr));
                if(prepareList->size()>=bundleSize)
                {
                    GlobalObjects::danmuRender->prepareDanmu(prepareList);
                    prepareList=prepareListPool.isEmpty()?new PrepareList :prepareListPool.takeFirst();
                }
			}
        }
        else
            break;
    }
    if(prepareList->size()>0)
    {
        GlobalObjects::danmuRender->prepareDanmu(prepareList);
    }
	else
	{
		recyclePrepareList(prepareList);
	}
}

void DanmuPool::mediaTimeJumped(int newTime)
{
#ifdef QT_DEBUG
    qDebug()<<"pool:media time jumped,newTime:"<<newTime<<",currentTime:"<<currentTime<<",currentPos"<<currentPosition;
#endif
    currentTime=newTime;
    currentPosition=std::lower_bound(finalPool.begin(),finalPool.end(),newTime,DanmuComparer)-finalPool.begin();
    GlobalObjects::danmuRender->cleanup();
#ifdef QT_DEBUG
    qDebug()<<"pool:media time jumped,currentPos"<<currentPosition;
#endif
}

QModelIndex DanmuPool::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    DanmuComment *childItem=nullptr;
    if(parent.isValid())
    {
        const DanmuComment *parentItem = static_cast<DanmuComment *>(parent.internalPointer());
        if(!parentItem->mergedList) return QModelIndex();
        childItem = parentItem->mergedList->value(row).data();
    }
    else
    {
        childItem = finalPool.value(row).data();
    }
    return childItem?createIndex(row,column,childItem):QModelIndex();
}

QModelIndex DanmuPool::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();
    DanmuComment *cc = static_cast<DanmuComment*>(child.internalPointer());
    if(!cc->m_parent) return QModelIndex();
    int p_pos = std::lower_bound(finalPool.begin(), finalPool.end(), cc->m_parent->time, DanmuComparer) - finalPool.begin();
    while(finalPool.at(p_pos)!=cc->m_parent)p_pos++;
    Q_ASSERT(p_pos<finalPool.count());
    return createIndex(p_pos, 0,cc->m_parent);
}

int DanmuPool::rowCount(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return finalPool.count();
    int c=0;
    if(finalPool.at(parent.row())->mergedList)
        c=finalPool.at(parent.row())->mergedList->count();
    return c;
}

QVariant DanmuPool::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    auto comment=static_cast<DanmuComment*>(index.internalPointer());
    int col=index.column();
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col==0)
        {
            int sec_total=comment->time/1000;
            int min=sec_total/60;
            int sec=sec_total-min*60;
            return QString("%1:%2").arg(min,2,10,QChar('0')).arg(sec,2,10,QChar('0'));
        }
        else if(col==1)
        {
            if(comment->mergedList)
                return QString("[%1]%2").arg(comment->mergedList->count()).arg(comment->text);
            return comment->text;
        }
        break;
    }
    case Qt::ForegroundRole:
    {
        static QBrush blockBrush(QColor(150,150,150));
        static QBrush normalBrush;
        if(comment->blockBy!=-1)
            return blockBrush;
        normalBrush.setColor(QColor((comment->color>>16)&0xff,(comment->color>>8)&0xff,comment->color&0xff,200));
        return normalBrush;
    }
    case Qt::ToolTipRole:
    {
        static QString types[3]={tr("Roll"),tr("Top"),tr("Bottom")};
        return tr("User: %1\nTime: %2\nText: %3\nType: %4%5%6")
                .arg(comment->sender,
                     QDateTime::fromSecsSinceEpoch(comment->date).toString("yyyy-MM-dd hh:mm:ss"),
                     comment->text,
                     types[comment->type],
                     comment->blockBy==-1?"":tr("\nBlock By Rule:%1").arg(comment->blockBy),
                     comment->mergedList?tr("\nMerged Count: %1").arg(comment->mergedList->count()):"");
    }
	case Qt::FontRole:
    {
        static QFont blockFont("Microsoft YaHei UI", 11, -1, true);
        static QFont mergeFont("Microsoft YaHei UI", 11, QFont::DemiBold);
		if (comment->blockBy != -1 && col==1)    
            return blockFont;
        if(comment->mergedList && comment->mergedList->count()>0)
            return mergeFont;
        break;
    }
    default:
        return QVariant();
    }
	return QVariant();
}

QVariant DanmuPool::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QString headers[]={tr("Time"),tr("Content")};
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<2)return headers[section];
    }
    return QVariant();
}

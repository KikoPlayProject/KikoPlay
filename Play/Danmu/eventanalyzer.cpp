#include "eventanalyzer.h"
#include "Manager/pool.h"
#include <algorithm>
namespace
{
    template<class InputIt>
    void compute(InputIt first, InputIt last, float &avg, float &std_dev)
    {
        float sum = std::accumulate(first, last, 0.f);
        int slice_size = static_cast<int>(std::distance(first, last));
        avg = sum / slice_size;
        static QVector<float> diff;
        diff.resize(slice_size);
        std::transform(first, last, diff.begin(), [avg](float x) { return x - avg; });
        float sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.f);
        std_dev = std::sqrt(sq_sum / slice_size);
    }
}
EventAnalyzer::EventAnalyzer(QObject *parent):QObject(parent),curPool(nullptr),lag(30),threshold(3.f),influence(0.1f),
    iterations(20),c(1e-4f),d(0.85f)
{
    qRegisterMetaType<DanmuEvent>("DanmuEvent");
    qRegisterMetaType<QList<DanmuEvent> >("QList<DanmuEvent>");
}

QVector<DanmuEvent> EventAnalyzer::analyze(Pool *pool)
{
    if(!pool) return QVector<DanmuEvent>();
    // Assert that the pool has been sorted
    curPool = pool;
    int count = curPool->comments().count();
    do
    {
        if(count == 0) break;
        int duration = curPool->comments().last()->time / 1000;
        // if there is not enough danmu, we do not perform analyzing
        if(count < duration) break;
        moveAverage();
        return postProcess(zScoreThresholding());
    }while(false);
    curPool = nullptr;
    return QVector<DanmuEvent>();
}

void EventAnalyzer::moveAverage()
{
    auto &comments = curPool->comments();
    int count = comments.size();
    const int mergeInterval = 1000;
    QVector<int> countSerise;
    int i = 0, curCount=0, curTime=0;
    while(i<count)
    {
        if(comments.at(i)->time - curTime < mergeInterval)
        {
            ++curCount;
            ++i;
        }
        else
        {
            countSerise.append(curCount);
            curCount = 0;
            curTime += mergeInterval;
        }
    }
    countSerise.append(curCount);

    const int windowSize = 1;
    int counts = countSerise.size();
    timeSeries.resize(counts);
    for (int i = 0; i<counts;++i) {
        int l = std::max(i-windowSize,0);
        int r = std::min(i+windowSize+1,counts);
        float w_sum = std::max(std::accumulate(countSerise.begin()+l, countSerise.begin()+r, 0.f),1.f);
        float t_sum = 0.f;
        for (int j = l; j<r; ++j)
            t_sum += countSerise[j] / w_sum * countSerise[j];
        timeSeries[i] = t_sum;
    }
}

QVector<int> EventAnalyzer::zScoreThresholding()
{
    QVector<int> eventPoints;
    int count = timeSeries.size();
    if(count<lag) return eventPoints;
    QVector<float> filteredWindow(lag);
    for(int i=0; i<lag; ++i) filteredWindow[i] = timeSeries[i];
    float avg, std;
    compute(filteredWindow.begin(),filteredWindow.end(), avg, std);
    float gAvg,gStd,gThreshold;
    compute(timeSeries.begin(), timeSeries.end(), gAvg, gStd);
    gThreshold = gAvg + 3*gStd;

    for(int i=lag;i<count;++i)
    {
        float nval(timeSeries[i]);
        if(fabsf(nval-avg)>std*threshold && nval>gThreshold)
        {
            eventPoints.append(i);
            nval=influence*nval + (1-influence)*filteredWindow[lag-1];
        }
        filteredWindow.removeFirst();
        filteredWindow.append(nval);
        compute(filteredWindow.begin(),filteredWindow.end(), avg, std);
    }
    return eventPoints;
}

QVector<DanmuEvent> EventAnalyzer::postProcess(const QVector<int> &eventPoints)
{
    QVector<DanmuEvent> events;
	if (eventPoints.isEmpty()) return events;
    DanmuEvent curEvent({-1,0,""});
    const int mergeInterval = 8;
    for(int p:eventPoints)
    {
        if(curEvent.start==-1)
        {
            curEvent.start=p;
            curEvent.duration=1;
        }
        else
        {
            if(p-curEvent.start-curEvent.duration<=mergeInterval)
            {
                curEvent.duration=p-curEvent.start+1;
            }
            else
            {
                if(curEvent.duration<5)
                {
                    curEvent.start-=2;
                    curEvent.duration+=4;
                }
                curEvent.start *= 1000;
                curEvent.duration *= 1000;
                events.append(curEvent);
                curEvent.start=p;
                curEvent.duration=1;
            }
        }
    }
    if(curEvent.duration<5)
    {
        curEvent.start-=2;
        curEvent.duration+=4;
    }
    curEvent.start *= 1000;
    curEvent.duration *= 1000;
    events.append(curEvent);
    setEventDescription(events);
    return events;
}

void EventAnalyzer::setEventDescription(QVector<DanmuEvent> &eventList)
{
    for(auto &event:eventList)
    {
        QStringList dmList(getDanmuRange(event));
        if(dmList.count()<10) continue;
        event.description=textRank(dmList);
    }
}

QStringList EventAnalyzer::getDanmuRange(const DanmuEvent &dmEvent)
{
    Q_ASSERT(curPool);
    QStringList dmList;
    auto &comments = curPool->comments();
    int position=std::lower_bound(comments.begin(),comments.end(),dmEvent.start,
                 [](const QSharedPointer<DanmuComment> &danmu,int time){return danmu->time<time;})-comments.begin();
    int m_t = dmEvent.start+dmEvent.duration;
    for (;position<comments.count();++position)
    {
        auto &dm = comments.at(position);
        if(dm->time<m_t && !dm->text.isEmpty() && dm->blockBy==-1)
        {
            dmList<<dm->text;
        }
    }
    return dmList;
}

QString EventAnalyzer::textRank(const QStringList &dmList)
{
    static QVector<QVector<float> > weightMatrix;
    int length = dmList.size();
    Q_ASSERT(length>=10);
    weightMatrix.fill(QVector<float>(length),length);
    for(int i=0;i<length;++i)
    {
        weightMatrix[i][i] = 0.f;
        for(int j=i+1;j<length;++j)
        {
            weightMatrix[i][j] = getSimilarity(dmList[i],dmList[j]);
            weightMatrix[j][i] = weightMatrix[i][j];
        }
    }
    for(int i=0;i<length;++i)
    {
        float weight_sum = std::accumulate(weightMatrix[i].begin(), weightMatrix[i].end(), 0.f);
        if(weight_sum==0.f)weight_sum=1.f;
        for(int j=0;j<length;++j)
        {
            weightMatrix[j][i] /= weight_sum;
        }
    }
    static QVector<float> score;
    static QVector<int> index;
    score.fill(1.f, length);
    index.resize(length);
    for(int i = 0; i<length;++i) index[i]=i;
    float t_c = 0.f;
    for(int i=0;i<iterations;++i)
    {
        for(int k=0;k<length;++k)
        {
            float sum = std::inner_product(weightMatrix[k].begin(), weightMatrix[k].end(), score.begin(), 0.f);
            float ns = 1-d + d*sum;
            t_c += fabsf(score[k]-ns);
            score[k] = ns;
        }
        if(t_c<c) break;
        t_c = 0.f;
    }

    int m_pos=0,s_pos=1;
    if(score[0]<score[1])
    {
        m_pos=1;
        s_pos=0;
    }
    for(int i=2;i<length;++i)
    {
        if(score[i]>score[m_pos])
        {
            s_pos = m_pos;
            m_pos = i;
        }
        else if(score[i]>score[s_pos])
        {
            s_pos = i;
        }
    }
    return dmList.at(m_pos).length()<dmList.at(s_pos).length()?dmList.at(m_pos):dmList.at(s_pos);

}

float EventAnalyzer::getSimilarity(const QString &t1, const QString &t2)
{
    static QVector<int> charSpace(1<<16);
    int l1=t1.length(),l2=t2.length();
    int numIntersection=0,numUnion=0;
    for(int i=0;i<l1;++i) charSpace[t1.at(i).unicode()]++;
    for(int i=0;i<l2;++i) if(charSpace[t2.at(i).unicode()])numIntersection++;
    for(int i=0;i<l2;++i) charSpace[t2.at(i).unicode()]++;
    for(int i=0;i<l1;++i)
    {
        if(charSpace[t1.at(i).unicode()])numUnion++;
        charSpace[t1.at(i).unicode()]=0;
    }
    for(int i=0;i<l2;++i)
    {
        if(charSpace[t2.at(i).unicode()])numUnion++;
        charSpace[t2.at(i).unicode()]=0;
    }
    return numIntersection / static_cast<float>(numUnion);
}

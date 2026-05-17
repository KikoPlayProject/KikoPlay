#include "cliprangeedit.h"
#include "Play/Danmu/common.h"
#include "UI/widgets/danmurangeselector.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include <QVBoxLayout>
#include <QTreeView>
#include <QHeaderView>
#include <QLabel>
#include <QTimeEdit>
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"


ClipRangeEdit::ClipRangeEdit(const DanmuSource *src, QVector<SimpleDanmuInfo> *dmList, QWidget *parent, int duration) :
    CFramelessDialog(tr("Clip Edit"), parent, true)
{
    std::sort(dmList->begin(),dmList->end(), [](const SimpleDanmuInfo &d1, const SimpleDanmuInfo &d2){ return d1.originTime < d2.originTime; });
    const int durationMs = duration > 0 ? duration * 1000 : (src->duration > 0 ? src->duration * 1000 : ( dmList->isEmpty() ? 1000 : dmList->last().originTime + 1000));

    QWidget *spinContainer = new QWidget(this);
    QTimeEdit *startSpin = new QTimeEdit(spinContainer);
    startSpin->setDisplayFormat("mm:ss");
    startSpin->setTimeRange(QTime(0, 0), QTime(0, 0).addMSecs(durationMs));
    QTimeEdit *endSpin = new QTimeEdit(spinContainer);
    endSpin->setDisplayFormat("mm:ss");
    endSpin->setTimeRange(QTime(0, 0), QTime(0, 0).addMSecs(durationMs));
    endSpin->setTime(QTime(0, 0).addMSecs(durationMs));
    KPushButton *clearBtn = new KPushButton(tr("Reset"), this);
    QHBoxLayout *spinHLayout = new QHBoxLayout(spinContainer);
    spinHLayout->setContentsMargins(0, 0, 0, 0);
    spinHLayout->addWidget(new QLabel(tr("Start Time(s)"), spinContainer));
    spinHLayout->addWidget(startSpin);
    spinHLayout->addWidget(new QLabel(tr("End Time(s)"), spinContainer));
    spinHLayout->addWidget(endSpin);
    spinHLayout->addWidget(clearBtn);
    spinHLayout->addStretch();

    DanmuRangeSelector *rangeSelector = new DanmuRangeSelector(this);
    rangeSelector->setFixedHeight(120);

    ClipDanmuPoolModel *model = new ClipDanmuPoolModel(dmList, this);

    QTreeView *dmView = new QTreeView(this);
    dmView->setRootIsDecorated(false);
    dmView->setFont(QFont(GlobalObjects::normalFont, 11));
    dmView->header()->setFont(QFont(GlobalObjects::normalFont, 12));
    dmView->setAlternatingRowColors(true);
    dmView->setItemDelegate(new KTreeviewItemDelegate(dmView));
    dmView->header()->setStretchLastSection(true);
    dmView->setModel(model);
    dmView->setObjectName(QStringLiteral("DanmuView"));

    QVBoxLayout *dialogVLayout=new QVBoxLayout(this);
    dialogVLayout->addWidget(spinContainer);
    dialogVLayout->addWidget(rangeSelector);
    dialogVLayout->addWidget(dmView);


    QObject::connect(rangeSelector, &DanmuRangeSelector::rangeChanged, this, [=](int s, int e){
        startSpin->blockSignals(true);
        endSpin->blockSignals(true);
        startSpin->setTime(QTime(0, 0).addSecs(s / 1000));
        endSpin->setTime(QTime(0, 0).addSecs(e / 1000));
        startSpin->blockSignals(false);
        endSpin->blockSignals(false);
        model->setClipRange(s, e);
        dmView->scrollTo(model->getIndex(s),QAbstractItemView::PositionAtCenter);
        this->clipStart = s;
        this->clipDuration = e - s;
        if (this->clipDuration / 1000 >= durationMs / 1000)
        {
            this->clipStart = -1;
            this->clipDuration = 0;
        }
    });
    rangeSelector->setData(dmList, durationMs);

    QObject::connect(startSpin, &QTimeEdit::userTimeChanged, this, [=](QTime time){
        if (time > endSpin->time())
        {
            startSpin->setTime(endSpin->time());
            return;
        }
        rangeSelector->setStartTime(time.msecsSinceStartOfDay());
    });
    QObject::connect(endSpin, &QTimeEdit::userTimeChanged, this, [=](QTime time){
        if (time < startSpin->time())
        {
            endSpin->setTime(startSpin->time());
            return;
        }
        rangeSelector->setEndTime(time.msecsSinceStartOfDay());
    });
    QObject::connect(clearBtn, &KPushButton::clicked, this, [=](){
        rangeSelector->setStartTime(0);
        rangeSelector->setEndTime(durationMs);
    });

    if (src->hasClip())
    {
        rangeSelector->setStartTime(src->clipStart);
        rangeSelector->setEndTime(src->clipStart + src->clipDuration);
    }

    resize(800, 420);

}

ClipDanmuPoolModel::ClipDanmuPoolModel(const QVector<SimpleDanmuInfo> *dmList, QObject *parent) :
    QAbstractItemModel(parent), m_dmList(dmList), clipStart(0), clipEnd(0)
{

}

QModelIndex ClipDanmuPoolModel::getIndex(int time)
{
    if (m_dmList->isEmpty()) return QModelIndex();
    int pos = std::lower_bound(m_dmList->begin(),m_dmList->end(), time,
                               [](const SimpleDanmuInfo &danmu, int time){ return danmu.originTime < time; }) - m_dmList->begin();
    pos = qMin<int>(pos, m_dmList->size() - 1);
    return createIndex(pos, 0);
}

void ClipDanmuPoolModel::setClipRange(int start, int end)
{
    clipStart = start;
    clipEnd = end;
}

QVariant ClipDanmuPoolModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    auto &danmu = m_dmList->at(index.row());
    int col=index.column();
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if (col==0)
        {
            return formatTime(danmu.originTime);
        }
        else if (col==1)
        {
            return danmu.text;
        }
        break;
    }
    case Qt::ForegroundRole:
    {
        if (danmu.originTime < clipStart || danmu.originTime > clipEnd)
        {
            return QColor(180, 180, 180);
        }
        else
        {
            return QColor(255, 255, 255);
        }
        break;
    }
    default:
        return QVariant();
    }
    return QVariant();
}

QVariant ClipDanmuPoolModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QString headers[]={ tr("Time"), tr("Content") };
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if (section < sizeof(headers) / sizeof(QString)) return headers[section];
    }
    return QVariant();
}

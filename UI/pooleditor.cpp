#include "pooleditor.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QAction>
#include <QApplication>
#include <QStackedLayout>
#include "widgets/danmusourcetip.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaMenu.h"
#include "UI/ela/ElaSpinBox.h"
#include "UI/widgets/elidedlabel.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Video/mpvplayer.h"
#include "dialogs/timelineedit.h"
#include "Common/notifier.h"
#include "dialogs/danmuview.h"
#include "UI/widgets/fonticonbutton.h"

namespace
{
    QVector<QPair<int,int> > timelineClipBoard;
    QVector<PoolItem *> *items;
    PoolEditor *editor;
    class PoolSignalBlock
    {
    public:
        PoolSignalBlock()
        {
            Pool *curPool = GlobalObjects::danmuPool->getPool();
            if (curPool && editor)
            {
                curPool->disconnect(editor);
            }
        }
        ~PoolSignalBlock()
        {
            if (editor)
            {
                QObject::connect(GlobalObjects::danmuPool->getPool(), &Pool::poolChanged, editor, &PoolEditor::refreshItems);
            }
        }
    };
}
PoolEditor::PoolEditor(QWidget *parent) : CFramelessDialog(tr("Edit Pool"), parent, false, true, false), curPool(nullptr)
{
    items = &poolItems;
    editor = this;
    QWidget *contentWidget = new QWidget(this);

    contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QScrollArea *contentScrollArea = new QScrollArea(this);
    contentScrollArea->setContentsMargins(0, 0, 0, 0);
    contentScrollArea->setWidget(contentWidget);
    contentScrollArea->setWidgetResizable(true);
    contentScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentScrollArea->setAlignment(Qt::AlignCenter);

    poolItemVLayout = new QVBoxLayout(contentWidget);
    poolItemVLayout->setContentsMargins(0, 0, 4, 0);
    poolItemVLayout->addStretch(1);

    QObject::connect(GlobalObjects::danmuPool, &DanmuPool::poolIdChanged, this, &PoolEditor::refreshItems);

    QPushButton *shareCodeButton = new KPushButton(tr("Share Pool Code"), this);
    QPushButton *addCodeButton = new KPushButton(tr("Add Pool Code"), this);
    QPushButton *exportButton = new KPushButton(tr("Export"), this);

    QWidget *btnContainer = new QWidget(this);
    QHBoxLayout *btnHLayout = new QHBoxLayout(btnContainer);
    btnHLayout->setContentsMargins(0, 0, 0, 0);
    btnHLayout->addWidget(shareCodeButton);
    btnHLayout->addWidget(addCodeButton);
    btnHLayout->addWidget(exportButton);
    btnHLayout->addStretch(1);

    QLabel *exportTip = new QLabel(tr("Select items to be exported"), this);
    QPushButton *exportPoolButton = new KPushButton(tr("Export"), this);
    QPushButton *cancelButton = new KPushButton(tr("Cancel"), this);
    QWidget *exportPage = new QWidget(this);
    QHBoxLayout *eHLayout = new QHBoxLayout(exportPage);
    eHLayout->setContentsMargins(0, 0, 0, 0);
    eHLayout->addWidget(exportTip);
    eHLayout->addStretch(1);
    eHLayout->addWidget(cancelButton);
    eHLayout->addWidget(exportPoolButton);

    pageBtnSLayout = new QStackedLayout;
    pageBtnSLayout->setContentsMargins(0, 0, 0, 0);
    pageBtnSLayout->addWidget(btnContainer);
    pageBtnSLayout->addWidget(exportPage);

    QGridLayout *gLayout=new QGridLayout(this);
    gLayout->addLayout(pageBtnSLayout, 0, 0, 1, 3);
    gLayout->addWidget(contentScrollArea, 1, 0, 1, 3);
    gLayout->setRowStretch(1, 1);
    gLayout->setColumnStretch(2, 1);

    setMinimumSize(320, 160);
    setSizeSettingKey("DialogSize/PoolEdit",QSize(600, 360));


    QObject::connect(shareCodeButton, &QPushButton::clicked, this, [this](){
        QString code(GlobalObjects::danmuPool->getPool()->getPoolCode());
        if(code.isEmpty())
        {
            this->showMessage(tr("No Danmu Source to Share"), NM_ERROR | NM_HIDE);
        }
        else
        {
            QClipboard *cb = QApplication::clipboard();
            cb->setText("kikoplay:pool="+code);
            this->showMessage(tr("Pool Code has been Copied to Clipboard"));
        }

    });

    QObject::connect(addCodeButton, &QPushButton::clicked, this, [this](){
        QClipboard *cb = QApplication::clipboard();
        QString code(cb->text());
        if(code.isEmpty() ||
            (!code.startsWith("kikoplay:pool=") &&
             !code.startsWith("kikoplay:anime="))) return;
        PoolSignalBlock block;
        bool ret = GlobalObjects::danmuPool->getPool()->addPoolCode(
            code.mid(code.startsWith("kikoplay:pool=")?14:15),
            code.startsWith("kikoplay:anime="));
        if (ret) refreshItems();
        this->showMessage(ret?tr("Code Added"):tr("Code Error"),ret? NM_HIDE:NM_ERROR | NM_HIDE);
    });

    QObject::connect(exportButton, &QPushButton::clicked, this, [=](){
        for (auto item : poolItems)
        {
            item->setSelectState(true);
        }
        pageBtnSLayout->setCurrentIndex(1);
    });

    QObject::connect(cancelButton, &QPushButton::clicked, this, [=](){
        for (auto item : poolItems)
        {
            item->setSelectState(false);
        }
        pageBtnSLayout->setCurrentIndex(0);
    });

    QObject::connect(exportPoolButton, &QPushButton::clicked, this, [=](){
        QList<int> exportIds;
        for (auto item : poolItems)
        {
            if (item->getSelectStatus())
            {
                exportIds << item->getSource()->id;
            }
        }
        if (exportIds.isEmpty()) return;
        const QString fileName = QFileDialog::getSaveFileName(this, tr("Save Danmu"), curPool ? curPool->epTitle() : "", tr("Xml File (*.xml)"));
        if (!fileName.isEmpty())
        {
            GlobalObjects::danmuPool->getPool()->exportPool(fileName, true, true, exportIds);
        }
    });

    refreshItems();
}

void PoolEditor::refreshItems()
{
    for (auto item : poolItems)
    {
        item->deleteLater();
    }
    poolItems.clear();
    if (curPool)
    {
        curPool->disconnect(this);
        curPool = nullptr;
    }
    curPool = GlobalObjects::danmuPool->getPool();
    const auto &sources = curPool->sources();
    for (auto iter = sources.begin(); iter != sources.end(); ++iter)
    {
        PoolItem *poolItem = new PoolItem(&iter.value(),this);
        poolItems << poolItem;
        poolItemVLayout->insertWidget(0, poolItem);
    }
    pageBtnSLayout->setCurrentIndex(0);
    QObject::connect(curPool, &Pool::poolChanged, this, &PoolEditor::refreshItems);
}

PoolItem::PoolItem(const DanmuSource *sourceInfo, QWidget *parent) : QWidget(parent), src(sourceInfo)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName(QStringLiteral("PoolItem"));

    DanmuSourceTip *srcTip = new DanmuSourceTip(sourceInfo, this);

    ElidedLabel *name = new ElidedLabel(sourceInfo->title, this);
    name->setFont(QFont(GlobalObjects::normalFont,16));
    name->setFontColor(QColor(214, 214, 214));
    QString sourceName = QString("%1(%2)").arg(sourceInfo->title).arg(sourceInfo->count);
    name->setToolTip(sourceInfo->title);


    QCheckBox *itemSwitch = new ElaCheckBox(tr("Show"), this);
    itemSwitch->setChecked(sourceInfo->show);

    QLabel *danmuCountTip = new QLabel(this);
    danmuCountTip->setText(tr("Danmu Count: %1").arg(formatFixedDanmuCount(sourceInfo->count, danmuCountTip->fontMetrics())));

    FontIconButton *refreshBtn = new FontIconButton(QChar(0xe631), "", 13, 10, 0, this);
    refreshBtn->setObjectName(QStringLiteral("FontIconToolButton"));
    refreshBtn->setContentsMargins(0, 0, 0, 0);

    QLabel *delayLabel = new QLabel(tr("Delay(s): "),this);
    QSpinBox *delaySpinBox = new ElaSpinBox(this);
    delaySpinBox->setRange(INT_MIN,INT_MAX);
    delaySpinBox->setValue(sourceInfo->delay/1000);

    FontIconButton *timelineBtn = new FontIconButton(QChar(0xe656), sourceInfo->timelineInfo.empty() ? "" : QString::number(sourceInfo->timelineInfo.size()), 13, 10, 2, this);
    timelineBtn->setObjectName(QStringLiteral("FontIconToolButton"));
    timelineBtn->setContentsMargins(0, 0, 0, 0);

    FontIconButton *deleteButton= new FontIconButton(QChar(0xe60b), "", 13, 10, 0, this);
    deleteButton->setObjectName(QStringLiteral("FontIconCloseToolButton"));
    deleteButton->setContentsMargins(0, 0, 0, 0);


    QHBoxLayout *nameHLayout = new QHBoxLayout();
    nameHLayout->setSpacing(4);
    nameHLayout->addWidget(srcTip, 0, Qt::AlignVCenter);
    nameHLayout->addWidget(name);
    nameHLayout->addWidget(deleteButton, 0, Qt::AlignVCenter);

    QHBoxLayout *opHLayout = new QHBoxLayout();
    opHLayout->setSpacing(4);
    opHLayout->addWidget(danmuCountTip);
    opHLayout->addWidget(refreshBtn);
    opHLayout->addSpacing(20);
    opHLayout->addWidget(delayLabel);
    opHLayout->addWidget(delaySpinBox);
    opHLayout->addWidget(timelineBtn);
    opHLayout->addStretch(1);
    opHLayout->addWidget(itemSwitch);

    setMinimumWidth(opHLayout->sizeHint().width());

    QWidget *pageDefault = new QWidget(this);
    QVBoxLayout *itemVLayout = new QVBoxLayout(pageDefault);
    itemVLayout->setSpacing(2);
    itemVLayout->addLayout(nameHLayout);
    itemVLayout->addLayout(opHLayout);

    pageSelectStatus = new QLabel(this);
    pageSelectStatus->setObjectName(QStringLiteral("PoolItemSelectStatus"));
    pageSelectStatus->installEventFilter(this);
    GlobalObjects::iconfont->setPointSize(54);
    pageSelectStatus->setFont(*GlobalObjects::iconfont);
    pageSelectStatus->setAlignment(Qt::AlignCenter);

    QStackedLayout *sLayout = new QStackedLayout(this);
    sLayout->setContentsMargins(0, 0, 0, 0);
    sLayout->addWidget(pageDefault);
    sLayout->addWidget(pageSelectStatus);

    QObject::connect(this, &PoolItem::selectStateChanged, this, [=](bool on){
        if (on)
        {
            sLayout->setStackingMode(QStackedLayout::StackAll);
            sLayout->setCurrentIndex(1);
        }
        else
        {
            sLayout->setStackingMode(QStackedLayout::StackOne);
            sLayout->setCurrentIndex(0);
        }
    });

    QAction *viewDanmu=new QAction(tr("View Danmu"),this);
    QObject::connect(viewDanmu,&QAction::triggered,this,[sourceInfo](){
        DanmuView view(&GlobalObjects::danmuPool->getPool()->comments(),editor, sourceInfo->id);
        view.exec();
    });
    QAction *copyTimeline=new QAction(tr("Copy TimeLine Info"), this);
    QObject::connect(copyTimeline,&QAction::triggered,this,[sourceInfo](){
        if(sourceInfo->timelineInfo.empty()) return;
        timelineClipBoard=sourceInfo->timelineInfo;
        editor->showMessage(tr("The Timeline Info has been copied"));
    });
    QAction *pasteTimeline=new QAction(tr("Paste TimeLine Info"), this);
    QObject::connect(pasteTimeline,&QAction::triggered,this, [=](){
        if(timelineClipBoard.isEmpty()) return;
        auto timeline=sourceInfo->timelineInfo;
        for(auto &pair:timelineClipBoard)
        {
            int i=0;
            int c=timeline.count();
            while(i<c && timeline.at(i).first<pair.first) i++;
            if(i<c && timeline.at(i).first==pair.first) continue;
            timeline.insert(i,pair);
        }
        PoolSignalBlock block;
        GlobalObjects::danmuPool->getPool()->setTimeline(sourceInfo->id,timeline);
        timelineBtn->setText(sourceInfo->timelineInfo.isEmpty()? "" : QString::number(sourceInfo->timelineInfo.size()));
        editor->showMessage(tr("Pasted Timeline Info"));
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    ElaMenu *actionMenu = new ElaMenu(this);
    actionMenu->addAction(copyTimeline);
    actionMenu->addAction(pasteTimeline);
    actionMenu->addAction(viewDanmu);
    QObject::connect(this, &QWidget::customContextMenuRequested, this, [=](){
        if (pageSelectStatus->isVisible()) return;
        copyTimeline->setEnabled(!sourceInfo->timelineInfo.empty());
        pasteTimeline->setEnabled(!timelineClipBoard.isEmpty());
        actionMenu->exec(QCursor::pos());
    });


    QObject::connect(itemSwitch, &QCheckBox::stateChanged, this, [sourceInfo](int state){
        GlobalObjects::danmuPool->getPool()->setSourceVisibility(sourceInfo->id, state!=Qt::Unchecked);
    });

    QObject::connect(delaySpinBox, &QSpinBox::editingFinished, this, [=](){
        PoolSignalBlock block;
        GlobalObjects::danmuPool->getPool()->setDelay(sourceInfo->id,delaySpinBox->value()*1000);
    });

    QObject::connect(timelineBtn, &QPushButton::clicked, this, [=](){
        int curTime=GlobalObjects::mpvplayer->getTime();
        QVector<SimpleDanmuInfo> list;
        GlobalObjects::danmuPool->getPool()->exportSimpleInfo(sourceInfo->id,list);
        TimelineEdit timelineEdit(sourceInfo,list,this,curTime);
        if(QDialog::Accepted==timelineEdit.exec())
        {
            PoolSignalBlock block;
            GlobalObjects::danmuPool->getPool()->setTimeline(sourceInfo->id, timelineEdit.timelineInfo);
            timelineBtn->setText(sourceInfo->timelineInfo.isEmpty()? "" : QString::number(sourceInfo->timelineInfo.size()));
        }
    });

    QObject::connect(deleteButton, &QPushButton::clicked, this, [=](){
        PoolSignalBlock block;
        GlobalObjects::danmuPool->getPool()->deleteSource(sourceInfo->id);
        items->removeOne(this);
        this->deleteLater();
    });

    QObject::connect(refreshBtn, &QPushButton::clicked, this, [=](){
        QList<DanmuComment *> tmpList;
        refreshBtn->setEnabled(false);
        deleteButton->setEnabled(false);
        timelineBtn->setEnabled(false);
        delaySpinBox->setEnabled(false);
        this->parentWidget()->setEnabled(false);
        editor->showBusyState(true);
        PoolSignalBlock block;
        int addCount = GlobalObjects::danmuPool->getPool()->update(sourceInfo->id);
        if (addCount > 0)
        {
            editor->showMessage(tr("Add %1 New Danmu").arg(addCount));
            danmuCountTip->setText(tr("Danmu Count: %1").arg(formatFixedDanmuCount(sourceInfo->count, danmuCountTip->fontMetrics())));
        }
        editor->showBusyState(false);
        this->parentWidget()->setEnabled(true);
        refreshBtn->setEnabled(true);
        deleteButton->setEnabled(true);
        timelineBtn->setEnabled(true);
        delaySpinBox->setEnabled(true);
    });

}

void PoolItem::setSelectState(bool on)
{
    isSelect = false;
    pageSelectStatus->clear();
    emit this->selectStateChanged(on);
}

QString PoolItem::formatFixedDanmuCount(int count, const QFontMetrics &fm)
{
    const int targetWidth = fm.horizontalAdvance("000000");
    const QString baseStr = QString::number(count);
    QString paddedStr = baseStr;
    while (fm.horizontalAdvance(paddedStr) < targetWidth) {
        paddedStr = " " + paddedStr;
    }
    return paddedStr;
}

bool PoolItem::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == pageSelectStatus && event->type() == QEvent::MouseButtonRelease)
    {
        isSelect = !isSelect;
        pageSelectStatus->setText(isSelect? QChar(0xe680) : ' ');
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

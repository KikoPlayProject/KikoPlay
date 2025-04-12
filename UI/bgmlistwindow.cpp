#include "bgmlistwindow.h"
#include "Download/BgmList/bgmlist.h"
#include "MediaLibrary/animeworker.h"
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QButtonGroup>
#include <QAction>
#include <QActionGroup>
#include <QTreeView>
#include <QHeaderView>
#include <QMenu>
#include <QDesktopServices>
#include <QComboBox>
#include <QApplication>
#include <QStyledItemDelegate>
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/fonticonbutton.h"
#include "globalobjects.h"
#include "Common/logger.h"
#include "Extension/Script/scriptmanager.h"
namespace
{
    class TextColorDelegate: public QStyledItemDelegate
    {
    public:
        explicit TextColorDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent)
        { }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            QStyleOptionViewItem ViewOption(option);
            QColor itemForegroundColor = index.data(Qt::ForegroundRole).value<QColor>();
            if (itemForegroundColor.isValid())
            {
                if (itemForegroundColor != option.palette.color(QPalette::WindowText))
                    ViewOption.palette.setColor(QPalette::HighlightedText, itemForegroundColor);

            }
            QStyledItemDelegate::paint(painter, ViewOption, index);
        }
    };
}
BgmListWindow::BgmListWindow(QWidget *parent) : QWidget(parent)
{
    bgmList = new BgmList(this);
    bgmList->setObjectName(QStringLiteral("bgmListModel"));
    BgmListFilterProxyModel *bgmListProxyModel = new BgmListFilterProxyModel(this);
    bgmListProxyModel->setSourceModel(bgmList);

    FontIconButton *scriptBtn = new FontIconButton(QChar(0xe63a), "", 14, 10, 2, this);
    scriptBtn->setObjectName(QStringLiteral("FontIconToolButton"));
    scriptBtn->setContentsMargins(2, 2, 2, 2);
    scriptMenu = new ElaMenu(scriptBtn);
    scriptCheckGroup = new QActionGroup(this);
    QAction *actRefresh = scriptMenu->addAction(tr("Refresh"));
    scriptMenu->addSeparator();
    scriptBtn->setMenu(scriptMenu);

    QHBoxLayout *btnHLayout = new QHBoxLayout();
    btnHLayout->addWidget(scriptBtn);

    QButtonGroup *filterButtonGroup = new QButtonGroup(this);
    weekDay = QDate::currentDate().dayOfWeek() % 7;
    for (int i = 0; i < 8; ++i)
    {
        QPushButton *filterBtn = new QPushButton(btnTitles[i] + (i == weekDay ? tr("(Today)") : ""), this);
        filterBtn->setCheckable(true);
        filterBtn->setObjectName(QStringLiteral("BgmFilterBtn"));
        btnHLayout->addWidget(filterBtn);
        filterButtonGroup->addButton(filterBtn, i);
        weekDayBtnList.append(filterBtn);
    }
    QPushButton *focusBtn = new QPushButton(tr("Focus"),this);
    focusBtn->setCheckable(true);
    focusBtn->setObjectName(QStringLiteral("BgmFilterBtn"));
    btnHLayout->addWidget(focusBtn);
    QObject::connect(focusBtn, &QPushButton::clicked, this, [bgmListProxyModel](bool checked){
       bgmListProxyModel->setFocusFilter(checked);
    });
    QPushButton *newBtn = new QPushButton(tr("New"),this);
    newBtn->setCheckable(true);
    newBtn->setObjectName(QStringLiteral("BgmFilterBtn"));
    btnHLayout->addWidget(newBtn);
    QObject::connect(newBtn, &QPushButton::clicked, this, [bgmListProxyModel](bool checked){
       bgmListProxyModel->setNewBgmFilter(checked);
    });

    QObject::connect(filterButtonGroup,&QButtonGroup::idToggled,[bgmListProxyModel](int id, bool checked){
        if (checked) bgmListProxyModel->setWeekFilter(id);
    });
    filterButtonGroup->button(weekDay)->setChecked(true);
    btnHLayout->addStretch(1);

    QLabel *infoLabel = new QLabel(this);
    infoLabel->setObjectName(QStringLiteral("BgmInfoLabel"));
    btnHLayout->addWidget(infoLabel);

    seasonIdCombo = new ElaComboBox(this);
    seasonIdCombo->view()->setMinimumWidth(seasonIdCombo->view()->fontMetrics().horizontalAdvance("0000-00") +
                                           QApplication::style()->pixelMetric(QStyle::PixelMetric::PM_ScrollBarExtent) +
                                           seasonIdCombo->view()->autoScrollMargin());
    seasonIdCombo->addItem("0000-00");  //placeholder
    btnHLayout->addWidget(seasonIdCombo);

    auto setControlEnable = [=](bool on){
        scriptBtn->setEnabled(on);
        seasonIdCombo->setEnabled(on);
        bgmListView->setEnabled(on);
    };

    QObject::connect(seasonIdCombo, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, [=](int index){
        setControlEnable(false);
        bgmList->setSeason(seasonIdCombo->itemText(index));
        setControlEnable(true);
    });
    QObject::connect(bgmList, &BgmList::seasonsUpdated, this, [this](){
       seasonIdCombo->clear();
       seasonIdCombo->addItems(bgmList->seasonList());
       seasonIdCombo->setCurrentIndex(seasonIdCombo->count()-1);
    });

    QObject::connect(actRefresh, &QAction::triggered, this, [=](){
        setControlEnable(false);
        bgmList->refresh();
        bgmListProxyModel->invalidate();
        setControlEnable(true);
    });

    QObject::connect(bgmList, &BgmList::bgmStatusUpdated, this, [infoLabel](int type, const QString &msg){
        infoLabel->setText(msg);
    });

    bgmListView = new BgmTreeView(this);
    bgmListView->setModel(bgmListProxyModel);
    bgmListView->setItemDelegate(new TextColorDelegate(this));
    bgmListView->setObjectName(QStringLiteral("BgmListView"));
    bgmListView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    bgmListView->header()->setObjectName(QStringLiteral("BgmListHeader"));
    bgmListView->header()->setDefaultAlignment(Qt::AlignCenter);
    bgmListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    new FloatScrollBar(bgmListView->verticalScrollBar(), bgmListView);
    bgmListView->setFont(QFont(GlobalObjects::normalFont,12));
    bgmListView->setIndentation(0);
    bgmListView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(bgmListView, &QTreeView::doubleClicked, this, [=](const QModelIndex &index){
        const BgmItem &item=bgmList->bgmList().at(bgmListProxyModel->mapToSource(index).row());
        emit searchBgm(item.title);
    });
    QObject::connect(bgmListView, &BgmTreeView::normColorChanged, bgmList, &BgmList::setNormColor);
    QObject::connect(bgmListView, &BgmTreeView::hoverColorChanged, bgmList, &BgmList::setHoverColor);

    QAction *addToLibrary = new QAction(tr("Add To Library"), this);
    QObject::connect(addToLibrary, &QAction::triggered, this, [=](){
        QItemSelection selection = bgmListProxyModel->mapSelectionToSource(bgmListView->selectionModel()->selection());
        if (selection.empty()) return;
        const BgmItem &item=bgmList->bgmList().at(selection.indexes().last().row());
        AnimeWorker::instance()->addAnime(item.title);
    });
    QAction *onBangumi = new QAction(tr("Bangumi Info"), this);
    QObject::connect(onBangumi, &QAction::triggered, this, [=](){
        QItemSelection selection = bgmListProxyModel->mapSelectionToSource(bgmListView->selectionModel()->selection());
        if (selection.empty()) return;
        const BgmItem &item = bgmList->bgmList().at(selection.indexes().last().row());
        QDesktopServices::openUrl(QUrl(QString("http://bgm.tv/subject/%1").arg(item.bgmId)));
    });
    QMenu *bgmContextMenu = new ElaMenu(this);
    QObject::connect(bgmListView, &QTreeView::customContextMenuRequested, this, [=](){
        QItemSelection selection = bgmListProxyModel->mapSelectionToSource(bgmListView->selectionModel()->selection());
        if (selection.empty()) return;
        const BgmItem &item = bgmList->bgmList().at(selection.indexes().last().row());
        bgmContextMenu->clear();
        bgmContextMenu->addAction(addToLibrary);
        bgmContextMenu->addAction(onBangumi);
        if (!item.onAirSites.empty())
        {
            bgmContextMenu->addSeparator();
            for (int i = 0; i < item.onAirSites.count() && i < item.onAirURLs.size(); ++i)
            {
                QAction *siteAction = new QAction(item.onAirSites.at(i),bgmContextMenu);
                QString url(item.onAirURLs.at(i));
                QObject::connect(siteAction, &QAction::triggered, siteAction, [url](){
                   QDesktopServices::openUrl(QUrl(url));
                });
                bgmContextMenu->addAction(siteAction);
            }
        }
        bgmContextMenu->exec(QCursor::pos());
    });
    bgmListView->addAction(addToLibrary);

    QObject::connect(scriptCheckGroup, &QActionGroup::triggered, this, [=](QAction *act){
        setControlEnable(false);
        const QString scriptId = act->data().toString();
        bgmList->setScriptId(scriptId);
        GlobalObjects::appSetting->setValue("BgmCalendar/DefaultScriptId", scriptId);
        setControlEnable(true);
    });

    QObject::connect(GlobalObjects::scriptManager, &ScriptManager::scriptChanged, this, [=](ScriptType type){
        if (type == ScriptType::BGM_CALENDAR)
        {
            for (QAction *sAct : scriptActions)
            {
                scriptMenu->removeAction(sAct);
                scriptCheckGroup->removeAction(sAct);
            }
            qDeleteAll(scriptActions);
            scriptActions.clear();

            const auto &calendarScripts = GlobalObjects::scriptManager->scripts(ScriptType::BGM_CALENDAR);
            QString curScriptId = bgmList->getScriptId();
            for(const auto &s : calendarScripts)
            {
                QAction *sAct = scriptMenu->addAction(s->name());
                scriptCheckGroup->addAction(sAct);
                sAct->setCheckable(true);
                sAct->setData(s->id());
                sAct->setChecked(s->id() == curScriptId);
                scriptActions << sAct;
            }
        }
    });

    QGridLayout *bgmWindowGLayout = new QGridLayout(this);
    bgmWindowGLayout->addLayout(btnHLayout, 0, 0);
    bgmWindowGLayout->addWidget(bgmListView, 1, 0);
}

void BgmListWindow::showEvent(QShowEvent *)
{
    static bool firstShow = true;
    QTimer::singleShot(0, [this](){
        if (firstShow)
        {
            seasonIdCombo->setEnabled(false);
            firstShow = false;
            QString curScriptId = GlobalObjects::appSetting->value("BgmCalendar/DefaultScriptId", "Kikyou.b.Bgmlist").toString();
            const auto &calendarScripts = GlobalObjects::scriptManager->scripts(ScriptType::BGM_CALENDAR);
            int sIndex = -1, i = 0;
            for(const auto &s : calendarScripts)
            {
                if (s->id() == curScriptId)
                {
                    sIndex = i;
                }
                QAction *sAct = scriptMenu->addAction(s->name());
                scriptCheckGroup->addAction(sAct);
                sAct->setCheckable(true);
                sAct->setData(s->id());
                sAct->setChecked(s->id() == curScriptId);
                scriptActions << sAct;
                ++i;
            }
            if (sIndex == -1)
            {
                Logger::logger()->log(Logger::Script, tr("Bangumi Calendar Lost: %1").arg(curScriptId));
                if(!calendarScripts.empty())
                {
                    sIndex = 0;
                }
            }
            else
            {
                bgmList->setScriptId(scriptCheckGroup->checkedAction()->data().toString());
            }
            seasonIdCombo->setEnabled(true);
        }
        int cWeekDay=QDate::currentDate().dayOfWeek()%7;
        if (weekDay != cWeekDay)
        {
            weekDayBtnList[weekDay]->setText(btnTitles[weekDay]);
            weekDay=cWeekDay;
            weekDayBtnList[weekDay]->setText(btnTitles[weekDay] + tr("(Today)"));
        }
    });
}

void BgmListWindow::hideEvent(QHideEvent *)
{
    bgmList->save();
}

void BgmListWindow::resizeEvent(QResizeEvent *)
{
    int oneWidth = bgmListView->width() / 10;
    bgmListView->header()->resizeSection(0, 4*oneWidth);
    bgmListView->header()->resizeSection(1, 2*oneWidth);
    bgmListView->header()->resizeSection(2, 3*oneWidth);
    bgmListView->header()->resizeSection(3, 1*oneWidth);
}




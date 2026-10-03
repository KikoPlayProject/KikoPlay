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
#include <QPainter>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QRegularExpression>
#include "UI/ela/Def.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/fonticonbutton.h"
#include "globalobjects.h"
#include "Common/logger.h"
#include "Extension/Script/scriptmanager.h"
namespace
{
    QUrl bangumiUrl(const QString &bgmId)
    {
        static const QRegularExpression validId(QStringLiteral("\\A[1-9][0-9]*\\z"));
        if (!validId.match(bgmId).hasMatch()) return QUrl();
        return QUrl(QStringLiteral("https://bgm.tv/subject/%1").arg(bgmId));
    }

    void openBangumi(const QString &bgmId)
    {
        const QUrl url = bangumiUrl(bgmId);
        if (!url.isEmpty()) QDesktopServices::openUrl(url);
    }

    struct BgmTitleLayout
    {
        QRect textRect, buttonRect;
        QString text;
        int iconSize{0};
    };

    BgmTitleLayout titleLayout(const QStyleOptionViewItem &option, const QModelIndex &index)
    {
        BgmTitleLayout layout;
        if (index.column() != int(BgmList::Columns::TITLE) ||
            bangumiUrl(index.data(BgmList::BgmIdRole).toString()).isEmpty()) return layout;

        QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        const QRect contentRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option, option.widget);
        const int margin = qMax(2, option.fontMetrics.height() / 4);
        const QRect availableRect = contentRect.adjusted(margin, 0, -margin, 0);
        const int buttonSize = qMin(option.rect.height(), qMax(24, option.fontMetrics.height() + 8));
        if (availableRect.width() < buttonSize) return layout;

        const int gap = qMax(6, option.fontMetrics.height() / 3);
        layout.text = option.fontMetrics.elidedText(option.text, Qt::ElideRight,
                                                    qMax(0, availableRect.width() - buttonSize - gap));
        const int textWidth = option.fontMetrics.horizontalAdvance(layout.text);
        const int contentWidth = textWidth + (layout.text.isEmpty() ? 0 : gap) + buttonSize;
        const int x = availableRect.left() + (availableRect.width() - contentWidth) / 2;
        layout.textRect = QRect(x, availableRect.top(), textWidth, availableRect.height());
        layout.buttonRect = QRect(x + contentWidth - buttonSize,
                                  option.rect.top() + (option.rect.height() - buttonSize) / 2,
                                  buttonSize, buttonSize);
        layout.iconSize = qMin(buttonSize - 4, qMax(14, qRound(option.fontMetrics.height() * 0.8)));
        return layout;
    }

    class BgmListDelegate: public QStyledItemDelegate
    {
    public:
        explicit BgmListDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent)
        { }

        QRect buttonRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            QStyleOptionViewItem viewOption(option);
            initStyleOption(&viewOption, index);
            return titleLayout(viewOption, index).buttonRect;
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QStyleOptionViewItem viewOption(option);
            initStyleOption(&viewOption, index);
            const QVariant foreground = index.data(Qt::ForegroundRole);
            if (foreground.canConvert<QBrush>())
                viewOption.palette.setBrush(QPalette::HighlightedText, foreground.value<QBrush>());

            const BgmTitleLayout layout = titleLayout(viewOption, index);
            if (layout.buttonRect.isEmpty())
            {
                QStyledItemDelegate::paint(painter, viewOption, index);
                return;
            }

            QStyle *style = option.widget ? option.widget->style() : QApplication::style();
            viewOption.text.clear();
            style->drawControl(QStyle::CE_ItemViewItem, &viewOption, painter, option.widget);

            const auto *view = qobject_cast<const BgmTreeView *>(option.widget);
            const bool hovered = view && view->isBangumiButtonHovered(index);
            const bool pressed = view && view->isBangumiButtonPressed(index);
            const QColor textColor = viewOption.palette.color(option.state & QStyle::State_Selected ?
                                                                QPalette::HighlightedText : QPalette::Text);
            QColor iconColor = view ? view->getNormColor() : textColor;
            if (!iconColor.isValid()) iconColor = textColor;
            if (hovered) iconColor = view && view->getHoverColor().isValid() ? view->getHoverColor() : textColor;

            painter->save();
            painter->setClipRect(option.rect);
            painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
            painter->setFont(viewOption.font);
            painter->setPen(textColor);
            painter->drawText(layout.textRect, Qt::AlignVCenter | Qt::AlignLeft, layout.text);
            if (hovered)
            {
                QColor background(iconColor);
                background.setAlpha(pressed ? 65 : 35);
                painter->setPen(Qt::NoPen);
                painter->setBrush(background);
                painter->drawRoundedRect(layout.buttonRect, 4, 4);
            }
            QFont iconFont(QStringLiteral("ElaAwesome"));
            iconFont.setPixelSize(layout.iconSize);
            painter->setFont(iconFont);
            painter->setPen(iconColor);
            painter->drawText(layout.buttonRect, Qt::AlignCenter,
                              QChar(static_cast<ushort>(ElaIconType::ArrowUpRightFromSquare)));
            painter->restore();
        }

        bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option,
                       const QModelIndex &index) override
        {
            if (event && event->type() == QEvent::ToolTip && buttonRect(option, index).contains(event->pos()))
            {
                QToolTip::showText(event->globalPos(), BgmListWindow::tr("Open in Bangumi"), view->viewport());
                return true;
            }
            return QStyledItemDelegate::helpEvent(event, view, option, index);
        }
    };
}

BgmTreeView::BgmTreeView(QWidget *parent) : QTreeView(parent)
{
    setMouseTracking(true);
    setItemDelegate(new BgmListDelegate(this));
    connect(header(), &QHeaderView::sectionResized, this, [this](){
        updateBangumiHover(QPoint(-1, -1));
    });
}

QModelIndex BgmTreeView::bangumiIndexAt(const QPoint &pos) const
{
    if (!isEnabled() || !viewport()->rect().contains(pos)) return QModelIndex();
    const QModelIndex index = indexAt(pos);
    if (!index.isValid() || !(index.flags() & Qt::ItemIsEnabled)) return QModelIndex();
    QStyleOptionViewItem option;
    initViewItemOption(&option);
    option.rect = visualRect(index);
    const auto *delegate = static_cast<const BgmListDelegate *>(itemDelegate());
    return delegate->buttonRect(option, index).contains(pos) ? index : QModelIndex();
}

void BgmTreeView::updateBangumiHover(const QPoint &pos)
{
    const QModelIndex index = bangumiIndexAt(pos);
    if (hoveredBangumiIndex != index)
    {
        const QRect oldRect = visualRect(hoveredBangumiIndex);
        hoveredBangumiIndex = index;
        viewport()->update(oldRect);
        viewport()->update(visualRect(index));
    }
    if (index.isValid()) viewport()->setCursor(Qt::PointingHandCursor);
    else viewport()->unsetCursor();
}

void BgmTreeView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        pressedBangumiIndex = bangumiIndexAt(event->position().toPoint());
        bangumiPressActive = pressedBangumiIndex.isValid();
        lastLeftPressWasBangumi = bangumiPressActive;
        pressedBangumiId = pressedBangumiIndex.data(BgmList::BgmIdRole).toString();
        if (bangumiPressActive)
        {
            setCurrentIndex(pressedBangumiIndex);
            updateBangumiHover(event->position().toPoint());
            viewport()->update(visualRect(pressedBangumiIndex));
            event->accept();
            return;
        }
    }
    QTreeView::mousePressEvent(event);
}

void BgmTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && bangumiPressActive)
    {
        const QPersistentModelIndex pressedIndex = pressedBangumiIndex;
        const QString bgmId = pressedBangumiId;
        pressedBangumiIndex = QPersistentModelIndex();
        pressedBangumiId.clear();
        bangumiPressActive = false;
        viewport()->update(visualRect(pressedIndex));
        event->accept();
        if (pressedIndex.isValid() && pressedIndex == bangumiIndexAt(event->position().toPoint()) &&
            pressedIndex.data(BgmList::BgmIdRole).toString() == bgmId)
            emit bangumiClicked(bgmId);
        return;
    }
    QTreeView::mouseReleaseEvent(event);
}

void BgmTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton &&
        (lastLeftPressWasBangumi || bangumiIndexAt(event->position().toPoint()).isValid()))
    {
        // The first release already opened the link. Consume the second release as well.
        pressedBangumiIndex = QPersistentModelIndex();
        pressedBangumiId.clear();
        bangumiPressActive = true;
        event->accept();
        return;
    }
    QTreeView::mouseDoubleClickEvent(event);
}

void BgmTreeView::mouseMoveEvent(QMouseEvent *event)
{
    updateBangumiHover(event->position().toPoint());
    if (bangumiPressActive) event->accept();
    else QTreeView::mouseMoveEvent(event);
}

bool BgmTreeView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::Leave || event->type() == QEvent::Resize || event->type() == QEvent::EnabledChange)
        updateBangumiHover(QPoint(-1, -1));
    return QTreeView::viewportEvent(event);
}

void BgmTreeView::scrollContentsBy(int dx, int dy)
{
    QTreeView::scrollContentsBy(dx, dy);
    updateBangumiHover(viewport()->mapFromGlobal(QCursor::pos()));
}

void BgmTreeView::reset()
{
    hoveredBangumiIndex = QPersistentModelIndex();
    pressedBangumiIndex = QPersistentModelIndex();
    pressedBangumiId.clear();
    viewport()->unsetCursor();
    QTreeView::reset();
}

void BgmTreeView::doItemsLayout()
{
    QTreeView::doItemsLayout();
    updateBangumiHover(QPoint(-1, -1));
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
    bgmListView->setObjectName(QStringLiteral("BgmListView"));
    bgmListView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    bgmListView->header()->setObjectName(QStringLiteral("BgmListHeader"));
    bgmListView->header()->setDefaultAlignment(Qt::AlignCenter);
    bgmListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    new FloatScrollBar(bgmListView->verticalScrollBar(), bgmListView);
    QFont bgmListViewFont(GlobalObjects::normalFont);
    bgmListViewFont.setPointSizeF(GlobalObjects::fontSize(12));
    bgmListView->setFont(bgmListViewFont);
    bgmListView->setIndentation(0);
    bgmListView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(bgmListView, &QTreeView::doubleClicked, this, [=](const QModelIndex &index){
        const BgmItem &item=bgmList->bgmList().at(bgmListProxyModel->mapToSource(index).row());
        emit searchBgm(item.title);
    });
    QObject::connect(bgmListView, &BgmTreeView::normColorChanged, bgmList, &BgmList::setNormColor);
    QObject::connect(bgmListView, &BgmTreeView::hoverColorChanged, bgmList, &BgmList::setHoverColor);
    QObject::connect(bgmListView, &BgmTreeView::bangumiClicked, this, openBangumi);

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
        openBangumi(selection.indexes().last().data(BgmList::BgmIdRole).toString());
    });
    QMenu *bgmContextMenu = new ElaMenu(this);
    QObject::connect(bgmListView, &QTreeView::customContextMenuRequested, this, [=](){
        QItemSelection selection = bgmListProxyModel->mapSelectionToSource(bgmListView->selectionModel()->selection());
        if (selection.empty()) return;
        const BgmItem &item = bgmList->bgmList().at(selection.indexes().last().row());
        onBangumi->setEnabled(!bangumiUrl(item.bgmId).isEmpty());
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




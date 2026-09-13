#include "danmuview.h"
#include <QTreeView>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QActionGroup>
#include <QToolButton>
#include <QMenu>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QClipboard>
#include <QSignalBlocker>
#include <QShortcut>
#include <QLocale>
#include "UI/ela/ElaMenu.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/stylemanager.h"
#include "globalobjects.h"
#include "Play/Danmu/danmuviewmodel.h"
#include "qapplication.h"

namespace
{
using CommentModel = DanmuViewModel<DanmuComment *>;

class DanmuViewDelegate : public QStyledItemDelegate
{
public:
    explicit DanmuViewDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        // The comment keeps its original color, even while the row is selected.
        const QColor foreground = index.column() == CommentModel::TEXT
                ? index.data(Qt::ForegroundRole).value<QColor>() : QColor(180, 188, 196);
        opt.palette.setColor(QPalette::Text, foreground);
        opt.palette.setColor(QPalette::HighlightedText, foreground);
        const QString text = opt.text;
        if (index.column() == CommentModel::TYPE) opt.text.clear();
        const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        if (index.column() != CommentModel::TYPE || text.isEmpty()) return;
        painter->save();
        painter->setClipRect(option.rect);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setFont(opt.font);
        QColor badgeColor(180, 188, 196);
        if (index.data(CommentModel::TypeRole).toInt() != DanmuComment::Rolling)
        {
            auto *styleManager = StyleManager::getStyleManager();
            badgeColor = styleManager->enableThemeColor() ? styleManager->curThemeColor().lighter(135) : QColor(113, 195, 235);
        }
        QColor background = badgeColor;
        background.setAlpha(24);
        const int width = qMin(opt.fontMetrics.horizontalAdvance(text) + 14, qMax(0, option.rect.width() - 12));
        QRect badgeRect(0, 0, width, opt.fontMetrics.height() + 6);
        badgeRect.moveCenter(option.rect.center());
        painter->setPen(Qt::NoPen);
        painter->setBrush(background);
        painter->drawRoundedRect(badgeRect, 4, 4);
        painter->setPen(badgeColor);
        painter->drawText(badgeRect, Qt::AlignCenter, opt.fontMetrics.elidedText(text, Qt::ElideRight, qMax(0, width - 10)));
        painter->restore();
    }
};
}


DanmuView::DanmuView(const QVector<DanmuComment *> *danmuList, QWidget *parent, int sourceId):CFramelessDialog (tr("View Danmu"),parent)
{
    initView();
    DanmuViewModel<DanmuComment *> *model=new DanmuViewModel<DanmuComment *>(danmuList,this);
    proxyModel = new DanmuViewProxyModel(this);
    proxyModel->setSourceId(sourceId);
    proxyModel->setSourceModel(model);
    danmuView->setModel(proxyModel);

    int originCount = danmuList->size();
    if (sourceId != -1)
    {
        originCount = 0;
        for (auto &comment : *danmuList)
        {
            if (comment->source == sourceId) originCount++;
        }
    }
    initStats(originCount);
}

DanmuView::DanmuView(const QVector<QSharedPointer<DanmuComment> > *danmuList, QWidget *parent, int sourceId):CFramelessDialog (tr("View Danmu"),parent)
{
    initView();
    DanmuViewModel<QSharedPointer<DanmuComment> > *model=new DanmuViewModel<QSharedPointer<DanmuComment> >(danmuList,this);
    proxyModel = new DanmuViewProxyModel(this);
    proxyModel->setSourceId(sourceId);
    proxyModel->setSourceModel(model);
    danmuView->setModel(proxyModel);

    int originCount = danmuList->size();
    if (sourceId != -1)
    {
        originCount = 0;
        for (auto &comment : *danmuList)
        {
            if (comment->source == sourceId) originCount++;
        }
    }
    initStats(originCount);
}

void DanmuView::initView()
{
    danmuView = new QTreeView(this);
    danmuView->setRootIsDecorated(false);
    danmuView->setIndentation(0);
    danmuView->setUniformRowHeights(true);
    danmuView->setMouseTracking(true);
    danmuView->setAllColumnsShowFocus(true);
    danmuView->setSelectionBehavior(QAbstractItemView::SelectRows);
    danmuView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    danmuView->setItemDelegate(new DanmuViewDelegate(danmuView));
    danmuView->setSortingEnabled(true);
    QFont danmuViewFont(GlobalObjects::normalFont);
    danmuViewFont.setPointSizeF(GlobalObjects::fontSize(11));
    danmuView->setFont(danmuViewFont);
    danmuView->header()->setSortIndicator(0, Qt::SortOrder::AscendingOrder);
    danmuView->setObjectName(QStringLiteral("DanmuCommentView"));
    danmuView->setTextElideMode(Qt::ElideRight);
    danmuView->header()->setObjectName(QStringLiteral("DanmuCommentHeader"));
    QFont danmuViewHeaderFont(GlobalObjects::normalFont);
    danmuViewHeaderFont.setPixelSize(qRound(GlobalObjects::fontSize(13)));
    danmuView->header()->setFont(danmuViewHeaderFont);

    QAction *copy = new QAction(tr("Copy"), this);
    copy->setShortcut(QKeySequence::Copy);
    copy->setShortcutContext(Qt::WidgetShortcut);
    QObject::connect(copy, &QAction::triggered, this, [=](){
        QStringList texts;
        for (const auto &index : danmuView->selectionModel()->selectedRows(CommentModel::TEXT))
        {
            texts.append(index.data(Qt::ToolTipRole).toString());
        }
        if (!texts.isEmpty()) QApplication::clipboard()->setText(texts.join('\n'));
    });
    danmuView->addAction(copy);
    danmuView->setContextMenuPolicy(Qt::CustomContextMenu);
    ElaMenu *actionMenu = new ElaMenu(danmuView);
    actionMenu->addAction(copy);
    QAction *copyCell = actionMenu->addAction(tr("Copy Cell"));
    QObject::connect(danmuView, &QTreeView::customContextMenuRequested, this, [=](const QPoint &pos){
        const QModelIndex index = danmuView->indexAt(pos);
        if (!index.isValid()) return;
        if (!danmuView->selectionModel()->isRowSelected(index.row(), QModelIndex()))
            danmuView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        if (actionMenu->exec(danmuView->viewport()->mapToGlobal(pos)) == copyCell)
            QApplication::clipboard()->setText(index.data(Qt::ToolTipRole).toString());
    });

    QWidget *summaryPanel = new QWidget(this);
    summaryPanel->setObjectName(QStringLiteral("DanmuSummaryPanel"));
    summaryPanel->setAttribute(Qt::WA_StyledBackground);
    QHBoxLayout *summaryLayout = new QHBoxLayout(summaryPanel);
    summaryLayout->setContentsMargins(16, 12, 16, 12);
    summaryLayout->setSpacing(22);
    auto addStat = [=](const QString &title, bool primary) {
        QVBoxLayout *layout = new QVBoxLayout;
        layout->setSpacing(3);
        QLabel *label = new QLabel(title, summaryPanel);
        label->setObjectName(QStringLiteral("DanmuSecondaryLabel"));
        QFont labelFont(GlobalObjects::normalFont);
        labelFont.setPixelSize(qRound(GlobalObjects::fontSize(12)));
        label->setFont(labelFont);
        layout->addWidget(label);
        QHBoxLayout *values = new QHBoxLayout;
        values->setSpacing(5);
        QLabel *value = new QLabel(summaryPanel);
        value->setObjectName(primary ? QStringLiteral("DanmuStatValue") : QStringLiteral("DanmuTypeValue"));
        QFont valueFont(GlobalObjects::normalFont, -1, primary ? QFont::Medium : QFont::Normal);
        valueFont.setPixelSize(qRound(GlobalObjects::fontSize(primary ? 22 : 18)));
        value->setFont(valueFont);
        values->addWidget(value);
        if (!countLabel)
        {
            totalLabel = new QLabel(summaryPanel);
            totalLabel->setObjectName(QStringLiteral("DanmuSecondaryLabel"));
            totalLabel->setFont(labelFont);
            values->addWidget(totalLabel, 0, Qt::AlignBottom);
        }
        values->addStretch();
        layout->addLayout(values);
        summaryLayout->addLayout(layout, primary ? 2 : 1);
        return value;
    };
    countLabel = nullptr;
    countLabel = addStat(tr("Danmu"), true);
    senderLabel = addStat(tr("Senders"), true);
    typeLabels[0] = addStat(QObject::tr("Roll"), false);
    typeLabels[1] = addStat(QObject::tr("Top"), false);
    typeLabels[2] = addStat(QObject::tr("Bottom"), false);

    filterEdit = new DanmuFilterBox(this);
    typeCombo = new ElaComboBox(this);
    typeCombo->setObjectName(QStringLiteral("DanmuTypeFilter"));
    typeCombo->setAccessibleName(tr("Danmu Type"));
    typeCombo->addItem(tr("All Types"), -1);
    typeCombo->addItem(QObject::tr("Roll"), int(DanmuComment::Rolling));
    typeCombo->addItem(QObject::tr("Top"), int(DanmuComment::Top));
    typeCombo->addItem(QObject::tr("Bottom"), int(DanmuComment::Bottom));
    typeCombo->setMinimumHeight(36);
    typeCombo->setMinimumWidth(110);
    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->setSpacing(10);
    searchLayout->addWidget(filterEdit, 1);
    searchLayout->addWidget(typeCombo);
    QShortcut *findShortcut = new QShortcut(QKeySequence::Find, this);
    QObject::connect(findShortcut, &QShortcut::activated, this, [this]() { filterEdit->setFocus(); filterEdit->selectAll(); });

    emptyLabel = new QLabel(this);
    emptyLabel->setObjectName(QStringLiteral("DanmuEmptyLabel"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->hide();

    detailPanel = new QWidget(this);
    detailPanel->setObjectName(QStringLiteral("DanmuDetailPanel"));
    detailPanel->setAttribute(Qt::WA_StyledBackground);
    QVBoxLayout *detailLayout = new QVBoxLayout(detailPanel);
    detailLayout->setContentsMargins(12, 9, 12, 9);
    detailLayout->setSpacing(5);
    detailLabel = new QLabel(detailPanel);
    detailLabel->setWordWrap(true);
    detailLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailLabel->setMaximumHeight(detailLabel->fontMetrics().lineSpacing() * 3);
    detailMetaLabel = new QLabel(detailPanel);
    detailMetaLabel->setObjectName(QStringLiteral("DanmuSecondaryLabel"));
    QFont detailMetaLabelFont(GlobalObjects::normalFont);
    detailMetaLabelFont.setPixelSize(qRound(GlobalObjects::fontSize(12)));
    detailMetaLabel->setFont(detailMetaLabelFont);
    detailMetaLabel->setTextFormat(Qt::PlainText);
    detailMetaLabel->setWordWrap(true);
    detailMetaLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailLayout->addWidget(detailLabel);
    detailLayout->addWidget(detailMetaLabel);
    detailPanel->hide();

    QVBoxLayout *viewLayout = new QVBoxLayout(this);
    viewLayout->setContentsMargins(16, 12, 16, 12);
    viewLayout->setSpacing(12);
    viewLayout->addWidget(summaryPanel);
    viewLayout->addLayout(searchLayout);
    viewLayout->addWidget(danmuView, 1);
    viewLayout->addWidget(emptyLabel, 1);
    viewLayout->addWidget(detailPanel);
    setMinimumSize(700, 440);
    setSizeSettingKey("DialogSize/DanmuView", QSize(900, 600));
}

void DanmuView::initStats(int originCount)
{
    // Move sections visually so the model's column IDs and search fields stay stable.
    QHeaderView *header = danmuView->header();
    header->setStretchLastSection(false);
    header->moveSection(header->visualIndex(CommentModel::TEXT), 1);
    header->setMinimumSectionSize(52);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setSectionResizeMode(CommentModel::TEXT, QHeaderView::Stretch);
    const QFontMetrics metrics(danmuView->font());
    danmuView->setColumnWidth(CommentModel::TIME, qMax(78, metrics.horizontalAdvance(QStringLiteral("000:00")) + 24));
    danmuView->setColumnWidth(CommentModel::TYPE, qMax(78, metrics.horizontalAdvance(QObject::tr("Bottom")) + 26));
    danmuView->setColumnWidth(CommentModel::SENDER, 150);
    danmuView->setColumnWidth(CommentModel::DATETIME, qMax(112, metrics.horizontalAdvance(QStringLiteral("2026-09-11")) + 24));

    auto updateStats = [=](){
        const int rowCount = proxyModel->rowCount();
        QSet<QString> senders;
        int typeCounter[] = {0, 0, 0};
        for (int i = 0; i < rowCount; ++i)
        {
            QModelIndex senderIndex = proxyModel->index(i, DanmuViewModel<DanmuComment *>::Columns::SENDER);
            senders << proxyModel->data(senderIndex).toString();
            int type = proxyModel->data(senderIndex, DanmuViewModel<DanmuComment *>::Roles::TypeRole).toInt();
            if (type >= 0 && type < 3) typeCounter[type]++;
        }
        const QLocale locale;
        countLabel->setText(locale.toString(rowCount));
        totalLabel->setText(QStringLiteral("/ %1").arg(locale.toString(originCount)));
        senderLabel->setText(locale.toString(senders.size()));
        for (int i = 0; i < 3; ++i) typeLabels[i]->setText(locale.toString(typeCounter[i]));
        emptyLabel->setText(filterEdit->text().isEmpty() && typeCombo->currentData().toInt() == -1
                            ? tr("No comments to display") : tr("No matching comments"));
        emptyLabel->setVisible(rowCount == 0);
        danmuView->setVisible(rowCount != 0);
        updateDetails();
    };
    updateStats();

    QObject::connect(filterEdit, &DanmuFilterBox::filterChanged, this, [=](int type, const QString &keyword){
        proxyModel->setFilterKeyColumn(type);
        proxyModel->setFilterFixedString(keyword);
        updateStats();
    });
    QObject::connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int) {
        proxyModel->setTypeFilter(typeCombo->currentData().toInt());
        updateStats();
    });
    QObject::connect(danmuView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DanmuView::updateDetails);
    QObject::connect(danmuView->selectionModel(), &QItemSelectionModel::currentChanged, this, &DanmuView::updateDetails);
}

void DanmuView::updateDetails()
{
    const auto rows = danmuView->selectionModel()->selectedRows(CommentModel::TEXT);
    detailPanel->setVisible(rows.size() == 1);
    if (rows.size() != 1) return;
    const QModelIndex index = rows.first();
    detailLabel->setText(QStringLiteral("<font style='color: %1'>%2</font>").arg(index.data(Qt::ForegroundRole).value<QColor>().name(), index.data(Qt::ToolTipRole).toString()));
    detailLabel->setToolTip(index.data(Qt::ToolTipRole).toString());
    detailMetaLabel->setText(QStringLiteral("%1  ·  %2  ·  %3  ·  %4")
        .arg(index.sibling(index.row(), CommentModel::TIME).data(Qt::ToolTipRole).toString(),
             index.sibling(index.row(), CommentModel::TYPE).data(Qt::ToolTipRole).toString(),
             index.sibling(index.row(), CommentModel::SENDER).data(Qt::ToolTipRole).toString(),
             index.sibling(index.row(), CommentModel::DATETIME).data(Qt::ToolTipRole).toString()));
}

DanmuFilterBox::DanmuFilterBox(QWidget *parent): KLineEdit(parent)
  , filterTypeGroup(new QActionGroup(this))
{
    setObjectName(QStringLiteral("DanmuSearchEdit"));
    setClearButtonEnabled(true);
    QFont widgetFont(GlobalObjects::normalFont);
    widgetFont.setPointSizeF(GlobalObjects::fontSize(10));
    setFont(widgetFont);
    setMinimumHeight(36);

    QMenu *menu = new ElaMenu(this);

    filterTypeGroup->setExclusive(true);
    QAction *filterContent = menu->addAction(tr("Content"));
    filterContent->setData(QVariant(int(4)));
    filterContent->setCheckable(true);
    filterContent->setChecked(true);
    filterTypeGroup->addAction(filterContent);

    QAction *filterUser = menu->addAction(tr("User"));
    filterUser->setData(QVariant(int(2)));
    filterUser->setCheckable(true);
    filterTypeGroup->addAction(filterUser);

    QAction *filterType = menu->addAction(tr("Type"));
    filterType->setData(QVariant(int(1)));
    filterType->setCheckable(true);
    filterTypeGroup->addAction(filterType);

    QAction *filterTime = menu->addAction(tr("Time"));
    filterTime->setData(QVariant(int(0)));
    filterTime->setCheckable(true);
    filterTypeGroup->addAction(filterTime);

    QObject::connect(filterTypeGroup, &QActionGroup::triggered, this, [this](QAction *act){
        updateFilterField();
        emit filterChanged(act->data().toInt(),this->text());
    });
    QObject::connect(this, &QLineEdit::textChanged, this, [this](const QString &text){
        emit filterChanged(filterTypeGroup->checkedAction()->data().toInt(),text);
    });

    optionsButton = new QToolButton(this);
    optionsButton->setCursor(Qt::ArrowCursor);

    optionsButton->setFocusPolicy(Qt::StrongFocus);
    optionsButton->setObjectName(QStringLiteral("DanmuSearchFieldButton"));
    optionsButton->setFont(font());
    optionsButton->setAccessibleName(tr("Search Field"));
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    updateFilterField();
}

void DanmuFilterBox::updateFilterField()
{
    const QAction *action = filterTypeGroup->checkedAction();
    optionsButton->setText(action->text());
    optionsButton->setToolTip(tr("Search Field"));
    const QStringList placeholders = {tr("Search comments..."), tr("Search users..."), tr("Search types..."), tr("Search time...")};
    setPlaceholderText(placeholders.value(filterTypeGroup->actions().indexOf(filterTypeGroup->checkedAction())));
    setAccessibleName(placeholderText());
    layoutFieldButton();
}

void DanmuFilterBox::resizeEvent(QResizeEvent *event)
{
    KLineEdit::resizeEvent(event);
    layoutFieldButton();
}

void DanmuFilterBox::layoutFieldButton()
{
    if (!optionsButton) return;
    // QLineEdit's action slots have icon-sized geometry; reserve space for a text button instead.
    const int buttonWidth = optionsButton->sizeHint().width();
    optionsButton->setGeometry(6, 5, buttonWidth, qMax(0, height() - 10));
    setTextMargins(buttonWidth + 10, 0, 0, 0);
}

void DanmuFilterBox::setFilter(FilterType type, const QString &filterStr)
{
    const auto actions = filterTypeGroup->actions();
    if (type < CONTENT || type > TIME) return;
    actions[type]->setChecked(true);
    updateFilterField();
    {
        const QSignalBlocker blocker(this);
        setText(filterStr);
    }
    emit filterChanged(actions[type]->data().toInt(), filterStr);
}

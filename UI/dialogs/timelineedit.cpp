#include "timelineedit.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "UI/widgets/kpushbutton.h"
#include "Common/notifier.h"
#include "globalobjects.h"
#include <QHeaderView>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>

namespace {
QString timeText(qint64 ms)
{
    const qint64 value = qAbs(ms);
    QString text = QStringLiteral("%1%2:%3").arg(ms < 0 ? "-" : "")
        .arg(value / 60000, 2, 10, QLatin1Char('0')).arg(value / 1000 % 60, 2, 10, QLatin1Char('0'));
    if (value % 1000) text += QStringLiteral(".%1").arg(value % 1000, 3, 10, QLatin1Char('0'));
    return text;
}

QString secondsText(qint64 ms)
{
    QString text = QString::number(qAbs(ms) / 1000.0, 'f', 3);
    while (text.endsWith('0')) text.chop(1);
    if (text.endsWith('.')) text.chop(1);
    return (ms < 0 ? QStringLiteral("-") : QStringLiteral("+")) + text;
}

bool parseTime(const QString &text, int &ms, bool secondsOnly = false)
{
    static const QRegularExpression seconds(QStringLiteral("^([+-]?)(\\d+)(?:\\.(\\d{1,3}))?$"));
    static const QRegularExpression minutes(QStringLiteral("^([+-]?)(\\d+):([0-5]\\d)(?:\\.(\\d{1,3}))?$"));
    const bool hasMinutes = !secondsOnly && text.contains(':');
    const auto match = (hasMinutes ? minutes : seconds).match(text.trimmed());
    if (!match.hasMatch()) return false;
    bool ok;
    const qint64 whole = match.captured(2).toLongLong(&ok);
    if (!ok || whole > std::numeric_limits<int>::max()) return false;
    qint64 value = whole * (hasMinutes ? 60000 : 1000);
    if (hasMinutes) value += match.captured(3).toInt() * 1000;
    value += match.captured(hasMinutes ? 4 : 3).leftJustified(3, '0').toInt();
    if (match.captured(1) == "-") value = -value;
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) return false;
    ms = int(value);
    return true;
}

void sortRules(QVector<TimelineMarker> &rules, int delay)
{
    std::stable_sort(rules.begin(), rules.end(), [](const TimelineMarker &a, const TimelineMarker &b) { return a.start < b.start; });
    qint64 offset = delay;
    for (int i = 0; i < rules.size();)
    {
        int end = i;
        // for legacy rules, may share a start
        while (end < rules.size() && rules[end].start == rules[i].start) offset += rules[end++].delta;
        // Legacy rules may share a start; they all take effect together.
        while (i < end) rules[i++].offset = offset;
    }
}
}

TimeLineInfoModel::TimeLineInfoModel(int delay, QObject *parent) : QAbstractTableModel(parent), baseDelay(delay) {}

void TimeLineInfoModel::setRules(QVector<TimelineMarker> rules)
{
    beginResetModel();
    sortRules(rules, baseDelay);
    items = std::move(rules);
    endResetModel();
}

void TimeLineInfoModel::setPendingOffset(int start, int delta)
{
    pendingStart = start;
    pendingDelta = delta;
    if (!items.isEmpty()) emit dataChanged(index(0, 2), index(items.size() - 1, 2), {Qt::DisplayRole});
}

QVariant TimeLineInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= items.size()) return {};
    const auto &rule = items[index.row()];
    if (role == Qt::UserRole) return rule.id;
    if (role == Qt::ToolTipRole && index.column() == 2) return tr("Offset after this point, including the source delay.");
    if (role != Qt::DisplayRole) return {};
    if (index.column() == 0) return QStringLiteral("%1  %2").arg(index.row() + 1).arg(timeText(rule.start));
    const qint64 offset = rule.offset + (pendingStart <= rule.start ? pendingDelta : 0);
    return tr("%1 s").arg(secondsText(index.column() == 1 ? rule.delta : offset));
}

QVariant TimeLineInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    const QString headers[] = {tr("Original point"), tr("Adjustment"), tr("Total offset")};
    return section >= 0 && section < 3 ? headers[section] : QVariant();
}

void NearbyDanmuModel::refresh(const QVector<SimpleDanmuInfo> &comments, const QVector<DanmuTimeResult> &times, qint64 focus)
{
    beginResetModel();
    rows.clear();
    int right = std::upper_bound(times.cbegin(), times.cend(), focus,
        [](qint64 value, const DanmuTimeResult &time) { return value < time.sourceTimeMs; }) - times.cbegin();
    int left = right - 1;
    QVector<int> nearest;
    // Keep both sides of the strict threshold visible, even in a dense burst.
    for (int i = 0; i < 5; ++i)
    {
        if (left >= 0 && focus - times[left].sourceTimeMs <= 10000) nearest.append(left--);
        if (right < times.size() && times[right].sourceTimeMs - focus <= 10000) nearest.append(right++);
    }
    while (nearest.size() < 10)
    {
        const qint64 before = left >= 0 ? qAbs(times[left].sourceTimeMs - focus) : 10001;
        const qint64 after = right < times.size() ? qAbs(times[right].sourceTimeMs - focus) : 10001;
        if (qMin(before, after) > 10000) break;
        nearest.append(before <= after ? left-- : right++);
    }
    std::sort(nearest.begin(), nearest.end());
    for (int i : nearest) rows.append({times[i], comments[i].text});
    endResetModel();
}

QVariant NearbyDanmuModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rows.size()) return {};
    const auto &row = rows[index.row()];
    if (role == Qt::ToolTipRole) return row.text;
    if (role != Qt::DisplayRole) return {};
    if (index.column() == 0) return timeText(row.time.sourceTimeMs);
    if (index.column() == 1) return row.time.status == DanmuTimeStatus::Visible ? timeText(row.time.finalTimeMs) : tr("Not played");
    return row.text;
}

QVariant NearbyDanmuModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    const QString headers[] = {tr("Original time"), tr("Final time"), tr("Content")};
    return section >= 0 && section < 3 ? headers[section] : QVariant();
}

TimelineEdit::TimelineEdit(const DanmuSource *source, const QVector<SimpleDanmuInfo> &simpleComments, QWidget *parent, int curTime)
    : CFramelessDialog(tr("Timeline Edit"), parent, true), draft(*source)
{
    timelineInfo = source->timelineInfo;
    for (const auto &comment : simpleComments)
    {
        if (source->mapTime(comment.originTime).status != DanmuTimeStatus::OutsideClip) comments.append(comment);
    }
    std::sort(comments.begin(), comments.end(), [](const SimpleDanmuInfo &a, const SimpleDanmuInfo &b) { return a.originTime < b.originTime; });
    for (const auto &comment : comments)
    {
        originalTimes.append(source->mapTime(comment.originTime).sourceTimeMs);
    }
    sourceEnd = source->hasClip() ? source->clipDuration : qint64(source->duration) * 1000;
    if (!originalTimes.isEmpty()) sourceEnd = qMax(sourceEnd, originalTimes.last() + (sourceEnd > 0 ? 0 : 10000));

    timelineModel = new TimeLineInfoModel(source->delay, this);
    QVector<TimelineMarker> rules;
    for (const auto &rule : timelineInfo) rules.append({nextId++, rule.first, rule.second, 0});
    timelineModel->setRules(rules);

    nearbyModel = new NearbyDanmuModel(this);
    timelineBar = new TimeLineBar(this);

    auto makeView = [this](QAbstractItemModel *model) {
        auto *view = new QTreeView(this);
        view->setRootIsDecorated(false);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setAlternatingRowColors(true);
        view->setUniformRowHeights(true);
        QFont viewFont(GlobalObjects::normalFont);
        viewFont.setPointSizeF(GlobalObjects::fontSize(10));
        view->setFont(viewFont);
        view->setItemDelegate(new KTreeviewItemDelegate(view));
        view->setModel(model);
        view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        view->header()->setStretchLastSection(true);
        return view;
    };
    timelineView = makeView(timelineModel);
    timelineView->setObjectName(QStringLiteral("TimelineRules"));
    timelineView->setMinimumHeight(timelineView->header()->sizeHint().height() + 3 * (timelineView->fontMetrics().height() + 12));
    QTreeView *nearbyView = makeView(nearbyModel);
    nearbyView->setObjectName(QStringLiteral("TimelineNearby"));
    nearbyView->setSelectionMode(QAbstractItemView::NoSelection);

    KPushButton *newButton = new KPushButton(tr("Add adjustment"), this);
    auto *left = new QWidget(this);
    QVBoxLayout *leftVLayout = new QVBoxLayout(left);
    leftVLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *headingHLayout = new QHBoxLayout;
    headingHLayout->addWidget(new QLabel(tr("Adjustments"), this));
    headingHLayout->addStretch();
    headingHLayout->addWidget(newButton);
    leftVLayout->addLayout(headingHLayout);
    leftVLayout->addWidget(timelineView, 1);
    editTitle = new QLabel(this);
    leftVLayout->addWidget(editTitle);

    QGridLayout *fieldsGLayout = new QGridLayout;
    fieldsGLayout->setContentsMargins(0, 0, 0, 0);

    startEdit = new ElaLineEdit(this);
    deltaEdit = new ElaLineEdit(this);
    startEdit->setPlaceholderText(tr("e.g. 12:00.200"));
    deltaEdit->setPlaceholderText(tr("e.g. +20 or -8"));
    QLabel *startLabel = new QLabel(tr("Original time point"), this);
    QLabel *deltaLabel = new QLabel(tr("Adjustment (s)"), this);
    startLabel->setBuddy(startEdit);
    deltaLabel->setBuddy(deltaEdit);
    fieldsGLayout->addWidget(startLabel, 0, 0);
    fieldsGLayout->addWidget(deltaLabel, 0, 1);
    fieldsGLayout->addWidget(startEdit, 1, 0);
    fieldsGLayout->addWidget(deltaEdit, 1, 1);
    fieldsGLayout->setColumnStretch(0, 1);
    fieldsGLayout->setColumnStretch(1, 1);
    leftVLayout->addLayout(fieldsGLayout);

    QLabel *hint = new QLabel(tr("Positive: later; negative: earlier."), this);
    hint->setToolTip(tr("Only comments after the original point are affected."));
    hint->setWordWrap(true);
    leftVLayout->addWidget(hint);

    errorLabel = new QLabel(this);
    errorLabel->setWordWrap(true);
    leftVLayout->addWidget(errorLabel);

    QHBoxLayout *actionsHLayout = new QHBoxLayout;
    actionsHLayout->addStretch();

    deleteButton = new KPushButton(tr("Delete adjustment"), this);
    addButton = new KPushButton(tr("Add"), this);
    actionsHLayout->addWidget(deleteButton);
    actionsHLayout->addWidget(addButton);
    leftVLayout->addLayout(actionsHLayout);

    QWidget *right = new QWidget(this);
    QVBoxLayout *rightVLayout = new QVBoxLayout(right);
    rightVLayout->setContentsMargins(0, 0, 0, 0);
    rightVLayout->addWidget(new QLabel(tr("Nearby comments"), this));
    nearbyLabel = new QLabel(this);
    rightVLayout->addWidget(nearbyLabel);
    rightVLayout->addWidget(nearbyView, 1);
    emptyLabel = new QLabel(tr("No nearby comments. You can still adjust the timeline."), this);
    emptyLabel->setWordWrap(true);
    rightVLayout->addWidget(emptyLabel);

    QSplitter *splitter = new QSplitter(this);
    splitter->setObjectName(QStringLiteral("NormalSplitter"));
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setChildrenCollapsible(false);
    splitter->setSizes({440, 500});

    QVBoxLayout *dialogVLayout = new QVBoxLayout(this);
    if (source->hasClip()) dialogVLayout->addWidget(new QLabel(tr("Original times are relative to the source clip start."), this));
    dialogVLayout->addWidget(timelineBar);
    dialogVLayout->addWidget(splitter, 1);

    for (auto *button : findChildren<QPushButton *>()) button->setAutoDefault(false);
    QObject::connect(newButton, &QPushButton::clicked, this, [this] { beginNew(); });
    QObject::connect(addButton, &QPushButton::clicked, this, &TimelineEdit::addRule);
    QObject::connect(deleteButton, &QPushButton::clicked, this, &TimelineEdit::deleteRule);
    QObject::connect(startEdit, &QLineEdit::textEdited, this, &TimelineEdit::edit);
    QObject::connect(deltaEdit, &QLineEdit::textEdited, this, &TimelineEdit::edit);
    QObject::connect(timelineView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this](const QModelIndex &index) {
        if (index.isValid()) selectRule(index.data(Qt::UserRole).toInt());
    });
    QObject::connect(timelineBar, &TimeLineBar::ruleSelected, this, [this](int id) { if (id >= 0) selectRule(id); });
    QObject::connect(timelineBar, &TimeLineBar::addRequested, this, [this](int start) {
        for (const auto &rule : timelineModel->rules()) if (rule.start == start) { selectRule(rule.id); return; }
        beginNew(start);
    });

    if (curTime >= 0)
    {
        const int start = int(qMin(qint64(curTime) * 1000, qint64(std::numeric_limits<int>::max())));
        auto existing = std::find_if(rules.cbegin(), rules.cend(), [start](const TimelineMarker &rule) { return rule.start == start; });
        if (existing != rules.cend()) selectRule(existing->id);
        else beginNew(start);
    }
    else if (rules.isEmpty()) beginNew();
    else selectRule(rules.first().id);

    setSizeSettingKey(QStringLiteral("DialogSize/TimelineEdit"), QSize(800, 720));
}

void TimelineEdit::setError(const QString &message)
{
    errorLabel->setText(message);
    errorLabel->setVisible(!message.isEmpty());
}

void TimelineEdit::selectRule(int id)
{
    const auto &rules = timelineModel->rules();
    for (int i = 0; i < rules.size(); ++i)
    {
        if (rules[i].id != id) continue;
        selectedId = id;
        adding = pendingValid = false;
        focus = rules[i].start;
        startEdit->setText(timeText(rules[i].start));
        deltaEdit->setText(secondsText(rules[i].delta));
        editTitle->setText(tr("Edit adjustment"));
        deleteButton->show();
        addButton->hide();
        const QSignalBlocker blocker(timelineView->selectionModel());
        timelineView->setCurrentIndex(timelineModel->index(i, 0));
        timelineView->scrollTo(timelineModel->index(i, 0));
        setError({});
        refreshPreview();
        return;
    }
}

void TimelineEdit::beginNew(int start)
{
    adding = true;
    selectedId = -1;
    pendingValid = false;
    const QSignalBlocker blocker(timelineView->selectionModel());
    timelineView->setCurrentIndex({});
    timelineView->clearSelection();
    startEdit->setText(start >= 0 ? timeText(start) : QString());
    deltaEdit->setText(QStringLiteral("+0"));
    editTitle->setText(tr("New adjustment"));
    deleteButton->hide();
    addButton->show();
    addButton->setText(tr("Add"));
    addButton->setEnabled(start >= 0);
    if (start >= 0) { focus = start; edit(); }
    else { setError({}); refreshPreview(); }
    startEdit->setFocus();
}

void TimelineEdit::edit()
{
    int start, delta;
    if (!parseTime(startEdit->text(), start))
    {
        setError(tr("Enter an original time such as 12:00 or 12:00.200. The last valid preview is kept."));
        addButton->setEnabled(false);
        return;
    }
    if (!parseTime(deltaEdit->text(), delta, true))
    {
        setError(tr("Enter seconds such as +20 or -8. The last valid preview is kept."));
        addButton->setEnabled(false);
        return;
    }
    auto rules = timelineModel->rules();
    auto current = std::find_if(rules.begin(), rules.end(), [this](const TimelineMarker &rule) { return rule.id == selectedId; });
    const bool moving = current != rules.end() && current->start != start;
    const auto duplicate = std::find_if(rules.begin(), rules.end(), [=](const TimelineMarker &rule) { return rule.start == start && rule.id != selectedId; });
    if (moving && duplicate != rules.end())
    {
        setError(tr("An adjustment already exists at this original time."));
        return;
    }
    setError({});
    focus = start;
    if (adding)
    {
        pendingValid = duplicate == rules.end();
        pending = {-2, start, delta, 0};
        addButton->setEnabled(true);
        addButton->setText(pendingValid ? tr("Add") : tr("Edit existing adjustment"));
    }
    else if (current != rules.end())
    {
        current->start = start;
        current->delta = delta;
        const QSignalBlocker blocker(timelineView->selectionModel());
        timelineModel->setRules(rules);
        const auto &sorted = timelineModel->rules();
        for (int i = 0; i < sorted.size(); ++i) if (sorted[i].id == selectedId) timelineView->setCurrentIndex(timelineModel->index(i, 0));
    }
    refreshPreview();
}

void TimelineEdit::addRule()
{
    edit();
    if (!errorLabel->text().isEmpty()) return;
    if (!pendingValid)
    {
        int start;
        if (parseTime(startEdit->text(), start)) for (const auto &rule : timelineModel->rules()) {
            if (rule.start == start) { selectRule(rule.id); return; }
        }
        return;
    }
    auto rules = timelineModel->rules();
    pending.id = nextId++;
    rules.append(pending);
    {
        const QSignalBlocker blocker(timelineView->selectionModel());
        timelineModel->setRules(rules);
    }
    selectRule(pending.id);
}

void TimelineEdit::deleteRule()
{
    auto rules = timelineModel->rules();
    int row = 0;
    while (row < rules.size() && rules[row].id != selectedId) ++row;
    if (row == rules.size()) return;
    rules.removeAt(row);
    {
        const QSignalBlocker blocker(timelineView->selectionModel());
        timelineModel->setRules(rules);
    }
    if (rules.isEmpty()) beginNew();
    else selectRule(rules[qMin(row, int(rules.size()) - 1)].id);
}

void TimelineEdit::refreshPreview()
{
    auto rules = timelineModel->rules();
    if (adding && pendingValid) rules.append(pending);
    timelineModel->setPendingOffset(pending.start, adding && pendingValid ? pending.delta : 0);
    sortRules(rules, draft.delay);
    draft.timelineInfo.clear();
    qint64 end = sourceEnd;
    for (const auto &rule : rules)
    {
        draft.timelineInfo.append({rule.start, rule.delta});
        if (!draft.hasClip() && draft.duration <= 0) end = qMax(end, qint64(rule.start) + 10000);
    }
    times.clear();
    times.reserve(comments.size());
    QVector<qint64> adjusted;
    adjusted.reserve(comments.size());
    for (const auto &comment : comments)
    {
        const auto time = draft.mapTime(comment.originTime);
        times.append(time);
        if (time.status == DanmuTimeStatus::Visible) adjusted.append(time.finalTimeMs);
    }
    nearbyModel->refresh(comments, times, focus);
    nearbyLabel->setText(tr("Original %1 ±10 s · %2 shown").arg(timeText(focus)).arg(nearbyModel->rowCount()));
    emptyLabel->setVisible(nearbyModel->rowCount() == 0);
    timelineBar->setData(originalTimes, adjusted, rules, qMax(qint64(1000), end), draft.delay);
    timelineBar->setSelection(adding && pendingValid ? -2 : selectedId, focus);
}

void TimelineEdit::onAccept()
{
    int delta;
    if (adding && startEdit->text().trimmed().isEmpty() && parseTime(deltaEdit->text(), delta, true) && delta == 0)
        beginNew();
    else
        edit();
    if (!errorLabel->text().isEmpty())
    {
        showMessage(errorLabel->text(), NM_ERROR | NM_HIDE);
        return;
    }
    if (adding && !pendingValid && !startEdit->text().isEmpty())
    {
        addRule();
        showMessage(tr("An adjustment already exists at this original time."), NM_ERROR | NM_HIDE);
        return;
    }
    if (adding && pendingValid && pending.delta != 0) addRule();
    timelineInfo.clear();
    for (const auto &rule : timelineModel->rules()) timelineInfo.append({rule.start, rule.delta});
    CFramelessDialog::onAccept();
}

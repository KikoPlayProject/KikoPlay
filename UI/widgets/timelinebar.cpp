#include "timelinebar.h"
#include "UI/ela/ElaTheme.h"
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QToolTip>
#include <algorithm>
#include <limits>

namespace
{
QString timeText(qint64 ms)
{
    const bool negative = ms < 0;
    const quint64 value = negative ? quint64(-(ms + 1)) + 1 : quint64(ms);
    QString text = QStringLiteral("%1:%2").arg(value / 60000, 2, 10, QLatin1Char('0'))
        .arg(value / 1000 % 60, 2, 10, QLatin1Char('0'));
    if (value % 1000) text += QStringLiteral(".%1").arg(value % 1000, 3, 10, QLatin1Char('0'));
    return negative ? QLatin1Char('-') + text : text;
}
QString deltaText(int delta)
{
    QString text = QString::number(delta / 1000.0, 'f', 3);
    while (text.endsWith(QLatin1Char('0'))) text.chop(1);
    if (text.endsWith(QLatin1Char('.'))) text.chop(1);
    return delta >= 0 ? QLatin1Char('+') + text : text;
}
}

TimeLineBar::TimeLineBar(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(400, 220);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(tr("Danmu timeline adjustment"));
    setAccessibleDescription(tr("Click the before-adjustment track to add an adjustment point. Select numbered markers on either track."));
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this]() { update(); });
    rebuildBins();
}

void TimeLineBar::setData(const QVector<qint64> &original, const QVector<qint64> &adjusted,
                         const QVector<TimelineMarker> &rules, qint64 end, int delay)
{
    data[0] = original; data[1] = adjusted; markers = rules;
    sourceEnd = qMax<qint64>(0, end); baseDelay = delay;
    std::stable_sort(markers.begin(), markers.end(), [](const TimelineMarker &a, const TimelineMarker &b) { return a.start < b.start; });
    rangeStart = 0; rangeEnd = qMax<qint64>(10000, sourceEnd);
    auto include = [this](qint64 time) { rangeEnd = qMax(rangeEnd, time); };
    for (const auto &lane : data) for (qint64 time : lane) include(time);
    for (const auto &marker : markers) { include(marker.start); include(qint64(marker.start) + marker.offset); }

    // Source milliseconds cover [0, sourceEnd + 1); a strict > cut applies
    // from start + 1. Merge coverage so later negative shifts can fill gaps.
    QVector<QPair<qint64, qint64>> coverage;
    qint64 cursor = 0, offset = baseDelay, mappedEnd = 0;
    auto cover = [&](qint64 endTime) {
        if (endTime <= cursor) return;
        const qint64 first = cursor + offset, last = endTime + offset;
        include(first); include(last); mappedEnd = qMax(mappedEnd, last);
        if (last > 0) coverage.append(qMakePair(qMax<qint64>(0, first), last));
    };
    for (const auto &marker : markers)
    {
        const qint64 cut = qint64(marker.start) + 1;
        if (cut <= 0) { offset = marker.offset; continue; }
        if (cut > sourceEnd) break;
        cover(cut); cursor = cut; offset = marker.offset;
    }
    cover(sourceEnd + 1);
    std::sort(coverage.begin(), coverage.end());
    gaps.clear(); cursor = 0;
    for (const auto &interval : coverage)
    {
        if (interval.first > cursor) gaps.append(qMakePair(cursor, interval.first));
        cursor = qMax(cursor, interval.second);
    }
    if (cursor < mappedEnd) gaps.append(qMakePair(cursor, mappedEnd));
    rangeEnd = (rangeEnd / 10000 + 1) * 10000; // Keep exact endpoint samples inside a visible bin.
    rebuildBins(); update();
}

void TimeLineBar::setSelection(int id, qint64 focus)
{
    selectedId = id; focusTime = focus; update();
}

QRectF TimeLineBar::laneRect(int lane) const
{
    const qreal label = fontMetrics().height() + 5;
    const qreal laneHeight = (height() - 6 - label) / 2.0;
    return QRectF(10, 8 + label + lane * laneHeight, qMax(1, width() - 20), qMax<qreal>(1, laneHeight - label - 8));
}

qreal TimeLineBar::timeX(qint64 time) const
{
    const QRectF area = laneRect(0);
    return area.left() + (time - rangeStart) * area.width() / (rangeEnd - rangeStart);
}

qint64 TimeLineBar::timeAt(qreal x) const
{
    const QRectF area = laneRect(0);
    return rangeStart + qint64((x - area.left()) * (rangeEnd - rangeStart) / area.width());
}

qint64 TimeLineBar::offsetAt(qint64 time) const
{
    qint64 offset = baseDelay;
    // The selection guide describes the segment after a cut, not a danmu at the cut.
    for (const auto &marker : markers) { if (marker.start > time) break; offset = marker.offset; }
    return offset;
}

void TimeLineBar::rebuildBins()
{
    const int capacity = qMax(1, int(laneRect(0).width() / 3));
    const qint64 span = rangeEnd - rangeStart;
    binWidth = qMax<qint64>(10000, ((span + capacity * 10000 - 1) / (capacity * 10000)) * 10000);
    const int count = int(span / binWidth) + 1;
    maxCount = 1;
    for (int lane = 0; lane < 2; ++lane)
    {
        bins[lane].fill(0, count);
        for (qint64 time : data[lane])
        {
            if (time < 0) continue;
            const int bin = int((time - rangeStart) / binWidth);
            if (bin >= 0 && bin < count) maxCount = qMax(maxCount, ++bins[lane][bin]);
        }
    }
    rebuildMarkers();
}

void TimeLineBar::rebuildMarkers()
{
    hits.clear();
    const qreal labelHeight = fontMetrics().height() + 3;
    for (int lane = 0; lane < 2; ++lane)
    {
        const QRectF area = laneRect(lane);
        qreal occupied[2] = { area.left() - 4, area.left() - 4 };
        int numberOfRule = 0;
        for (int i = 0; i < markers.size(); ++i)
        {
            const auto &marker = markers[i];
            const QString number = marker.id == -2 ? QStringLiteral("+") : QString::number(++numberOfRule);
            const qint64 markerTime = qint64(marker.start) + (lane ? marker.offset : 0);
            if (markerTime < 0 || markerTime > rangeEnd) continue;
            QString text = tr("%1  %2 s").arg(number, deltaText(marker.delta));
            qreal w = fontMetrics().horizontalAdvance(text) + 10;
            qreal left = qBound(area.left(), timeX(qint64(marker.start) + (lane ? marker.offset : 0)) - w / 2, qMax(area.left(), area.right() - w));
            int row = occupied[0] + 3 <= left ? 0 : 1;
            if (occupied[row] + 3 > left) { text = number; w = fontMetrics().horizontalAdvance(text) + 10; left = qBound(area.left(), timeX(qint64(marker.start) + (lane ? marker.offset : 0)) - w / 2, area.right() - w); }
            hits.append({QRectF(left, area.top() + row * labelHeight, w, labelHeight), i, lane, text});
            occupied[row] = qMax(occupied[row], left + w);
        }
    }
}

int TimeLineBar::hitAt(const QPointF &pos) const
{
    int match = -1;
    for (int i = 0; i < hits.size(); ++i)
        if (hits[i].rect.contains(pos)) { match = i; if (markers[hits[i].index].id == selectedId) return i; }
    return match;
}

void TimeLineBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto theme = eTheme->getThemeMode();
    const QColor text = ElaThemeColor(theme, BasicText), blue = ElaThemeColor(theme, PrimaryNormal);
    const QColor gapColor = theme == ElaThemeType::Dark ? QColor(228, 184, 109) : QColor(161, 112, 29);
    const QColor gapBackground = theme == ElaThemeType::Dark ? QColor(69, 57, 33) : QColor(248, 237, 211);
    const qreal labelHeight = fontMetrics().height() + 3;
    for (int lane = 0; lane < 2; ++lane)
    {
        const QRectF area = laneRect(lane);
        painter.setPen(text);
        const QString title = lane ? tr("After adjustment (final)") : tr("Before adjustment · click to add");
        painter.drawText(QRectF(area.left(), area.top() - labelHeight - 2, area.width() * .55, labelHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         fontMetrics().elidedText(title, Qt::ElideRight, int(area.width() * .55)));
        painter.drawText(QRectF(area.left() + area.width() * .55, area.top() - labelHeight - 2, area.width() * .45, labelHeight), Qt::AlignRight | Qt::AlignVCenter,
                         tr("%1 s / bin · max %2").arg(binWidth / 1000).arg(maxCount));
        painter.setPen(Qt::NoPen); painter.setBrush(ElaThemeColor(theme, BasicBase)); painter.drawRoundedRect(area, 5, 5);
        const QRectF graph = area.adjusted(0, labelHeight * 2, 0, 0);
        painter.save(); painter.setClipRect(graph);
        for (int bin = 0; bin < bins[lane].size(); ++bin)
        {
            const qreal left = timeX(rangeStart + bin * binWidth), barHeight = graph.height() * bins[lane][bin] / maxCount;
            painter.fillRect(QRectF(left, graph.bottom() - barHeight, qMax<qreal>(1, timeX(rangeStart + (bin + 1) * binWidth) - left - 1), barHeight), blue);
        }
        if (lane) for (const auto &gap : gaps)
        {
            const QRectF blank(timeX(gap.first), graph.top(), timeX(gap.second) - timeX(gap.first), graph.height());
            painter.fillRect(blank, gapBackground);
            painter.fillRect(blank, QBrush(gapColor, Qt::BDiagPattern));
        }
        painter.restore();
        if (focusTime >= 0)
        {
            const qreal x = timeX(focusTime + (lane ? offsetAt(focusTime) : 0));
            if (x >= area.left() && x <= area.right()) { painter.setPen(QPen(blue, 1, Qt::DashLine)); painter.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom())); }
        }
    }
    // Draw the selected label last, keeping it accessible in crowded regions.
    for (int selected = 0; selected < 2; ++selected) for (const auto &hit : hits)
    {
        const auto &marker = markers[hit.index]; const bool active = marker.id == selectedId;
        if (active != bool(selected)) continue;
        const QRectF area = laneRect(hit.lane);
        const qreal x = timeX(qint64(marker.start) + (hit.lane ? marker.offset : 0));
        painter.setPen(QPen(active ? blue : ElaThemeColor(theme, BasicBorder), active ? 2 : 1));
        painter.drawLine(QPointF(x, hit.rect.bottom()), QPointF(x, area.bottom()));
        painter.setBrush(active ? blue : ElaThemeColor(theme, BasicBase)); painter.drawRoundedRect(hit.rect, 3, 3);
        painter.setPen(active ? ElaThemeColor(theme, BasicTextInvert) : text); painter.drawText(hit.rect, Qt::AlignCenter, hit.text);
    }
    painter.setPen(text);
    const QRectF area = laneRect(1); const int ticks = qMax(2, qMin(5, width() / 150));
    const int tickWidth = qMax(60, (width() - 20) / ticks);
    for (int i = 0; i < ticks; ++i)
    {
        const qint64 time = rangeStart + (rangeEnd - rangeStart) * i / (ticks - 1);
        const qreal left = qBound(area.left(), timeX(time) - tickWidth / 2, area.right() - tickWidth);
        painter.drawText(QRectF(left, area.bottom() + 3, tickWidth, labelHeight), Qt::AlignCenter, timeText(time));
    }
}

void TimeLineBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event); rebuildBins();
}

void TimeLineBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    const int hit = hitAt(event->position());
    if (hit >= 0) { emit ruleSelected(markers[hits[hit].index].id); return; }
    const qint64 time = timeAt(event->position().x());
    if (laneRect(0).contains(event->position()) && time >= 0 && time <= sourceEnd)
        emit addRequested(int(qMin<qint64>(time / 1000, std::numeric_limits<int>::max() / 1000) * 1000));
}

void TimeLineBar::mouseMoveEvent(QMouseEvent *event)
{
    const int hit = hitAt(event->position());
    QString tip;
    if (hit >= 0)
    {
        const auto &marker = markers[hits[hit].index];
        tip = tr("After cut %1 → final times after %2\nChange from previous: %3 s")
            .arg(timeText(marker.start), timeText(qint64(marker.start) + marker.offset), deltaText(marker.delta));
    }
    else for (int lane = 0; lane < 2; ++lane) if (laneRect(lane).contains(event->position()))
    {
        const int bin = qBound(0, int((timeAt(event->position().x()) - rangeStart) / binWidth), int(bins[lane].size()) - 1);
        const qint64 first = rangeStart + bin * binWidth;
        tip = tr("%1 %2–%3\n%4 danmu").arg(lane ? tr("After adjustment (final)") : tr("Before adjustment"), timeText(first), timeText(qMin(rangeEnd, first + binWidth))).arg(bins[lane][bin]);
        if (!lane && timeAt(event->position().x()) >= 0 && timeAt(event->position().x()) <= sourceEnd) tip += QLatin1Char('\n') + tr("Click to add an adjustment point");
    }
    if (tip.isEmpty()) QToolTip::hideText();
    else QToolTip::showText(event->globalPosition().toPoint(), tip, this);
}

void TimeLineBar::leaveEvent(QEvent *event)
{
    QToolTip::hideText(); QWidget::leaveEvent(event);
}

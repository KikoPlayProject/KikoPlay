#include "downloadspeedwidget.h"

#include <QHelpEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <cmath>
#include "Download/util.h"

namespace
{
constexpr qint64 HistoryDurationMs = 60000;
constexpr qint64 ScaleShrinkDelayMs = 5000;
}

DownloadSpeedWidget::DownloadSpeedWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMaximumWidth(380);
    setMouseTracking(true);
    setAccessibleName(tr("Download speed"));
    hide();
}

void DownloadSpeedWidget::setSamples(const QVector<DownloadSpeedSample> &samples, qint64 nowMs, int staleAfterMs)
{
    m_nowMs = nowMs;
    m_staleAfterMs = qMax(0, staleAfterMs);
    m_samples.clear();
    qint64 peak = 0;
    for (const DownloadSpeedSample &sample : samples)
    {
        if (sample.timestampMs < nowMs - HistoryDurationMs || sample.timestampMs > nowMs)
            continue;
        m_samples.append(sample);
        peak = qMax(peak, sample.bytesPerSecond);
    }
    updateScale(peak);
    setVisible(peak > 0);
    if (peak <= 0)
        hideOwnToolTip();
    update();
}

void DownloadSpeedWidget::clear()
{
    m_samples.clear();
    m_nowMs = 0;
    m_ceiling = 1.0;
    m_shrinkSinceMs = -1;
    hideOwnToolTip();
    hide();
    update();
}

QSize DownloadSpeedWidget::sizeHint() const
{
    return QSize(340, qMax(120, fontMetrics().height() * 5 + 24));
}

QSize DownloadSpeedWidget::minimumSizeHint() const
{
    return QSize(220, qMax(100, fontMetrics().height() * 4 + 24));
}

QColor DownloadSpeedWidget::textColor() const
{
    return m_textColor.isValid() ? m_textColor : palette().color(QPalette::Text);
}

QColor DownloadSpeedWidget::gridColor() const
{
    if (m_gridColor.isValid())
        return m_gridColor;
    QColor color = textColor();
    color.setAlpha(35);
    return color;
}

double DownloadSpeedWidget::niceCeiling(double value)
{
    if (value <= 0.0)
        return 1.0;
    double unit = 1.0;
    while (value * 1.08 / unit >= 1024.0 && unit < 1099511627776.0)
        unit *= 1024.0;
    const double scaled = value * 1.08 / unit;
    const double magnitude = std::pow(10.0, std::floor(std::log10(scaled)));
    const double normalized = scaled / magnitude;
    const double step = normalized <= 1.0 ? 1.0 : normalized <= 2.0 ? 2.0 : normalized <= 5.0 ? 5.0 : 10.0;
    return step * magnitude * unit;
}

void DownloadSpeedWidget::updateScale(qint64 peak)
{
    const double target = niceCeiling(static_cast<double>(peak));
    if (target >= m_ceiling)
    {
        m_ceiling = target;
        m_shrinkSinceMs = -1;
    }
    else if (m_shrinkSinceMs < 0)
    {
        m_shrinkSinceMs = m_nowMs;
    }
    else if (m_nowMs - m_shrinkSinceMs >= ScaleShrinkDelayMs)
    {
        m_ceiling = target;
        m_shrinkSinceMs = -1;
    }
}

QRectF DownloadSpeedWidget::plotRect() const
{
    const QFontMetrics metrics(font());
    const int top = metrics.height() + 6;
    const int bottom = height() - 4;
    return QRectF(4, top, qMax(0, width() - 8), qMax(0, bottom - top));
}

QPointF DownloadSpeedWidget::samplePosition(const DownloadSpeedSample &sample, const QRectF &plot) const
{
    const double elapsed = static_cast<double>(sample.timestampMs - (m_nowMs - HistoryDurationMs));
    const double speed = qMax<qint64>(0, sample.bytesPerSecond);
    return QPointF(plot.left() + elapsed / HistoryDurationMs * plot.width(),
                   plot.bottom() - speed / m_ceiling * plot.height());
}

void DownloadSpeedWidget::paintEvent(QPaintEvent *)
{
    if (m_samples.isEmpty())
        return;
    const QRectF plot = plotRect();
    if (plot.width() < 1.0 || plot.height() < 1.0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QFontMetrics metrics(font());
    for (int tick = 0; tick <= 2; ++tick)
    {
        const qreal y = plot.bottom() - plot.height() * tick / 2.0;
        painter.setPen(QPen(gridColor(), 1.0));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }

    painter.save();
    painter.setClipRect(plot.adjusted(-1.5, -1.5, 1.5, 1.5));
    QLinearGradient gradient(0, plot.top(), 0, plot.bottom());
    QColor topFill = m_lineColor, bottomFill = m_lineColor;
    topFill.setAlpha(70);
    bottomFill.setAlpha(12);
    gradient.setColorAt(0, topFill);
    gradient.setColorAt(1, bottomFill);
    const QPen linePen(m_lineColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    QPainterPath line;
    QPointF segmentFirst, segmentLast;
    int segmentCount = 0;
    const auto drawSegment = [&]() {
        if (segmentCount == 0)
            return;
        if (segmentCount > 1)
        {
            QPainterPath fill = line;
            fill.lineTo(segmentLast.x(), plot.bottom());
            fill.lineTo(segmentFirst.x(), plot.bottom());
            fill.closeSubpath();
            painter.fillPath(fill, gradient);
            painter.setPen(linePen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(line);
        }
        else
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_lineColor);
            painter.drawEllipse(segmentFirst, 2.0, 2.0);
        }
    };
    for (const DownloadSpeedSample &sample : m_samples)
    {
        const QPointF point = samplePosition(sample, plot);
        if (segmentCount == 0 || sample.startsSegment)
        {
            drawSegment();
            line = QPainterPath(point);
            segmentFirst = point;
            segmentCount = 1;
        }
        else
        {
            line.lineTo(point);
            ++segmentCount;
        }
        segmentLast = point;
    }
    drawSegment();
    painter.restore();

    const DownloadSpeedSample &latest = m_samples.constLast();
    const bool stale = m_nowMs - latest.timestampMs > m_staleAfterMs;
    const QString speed = stale ? QStringLiteral("--") : formatSize(true, qMax<qint64>(0, latest.bytesPerSecond));
    const QString currentLabel = QStringLiteral("\u2193 %1").arg(speed);
    painter.setPen(textColor());
    painter.drawText(QRectF(plot.left(), 0, plot.width(), metrics.height()),
                     Qt::AlignRight | Qt::AlignVCenter, currentLabel);
}

bool DownloadSpeedWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
    {
        const auto *helpEvent = static_cast<QHelpEvent *>(event);
        const QRectF plot = plotRect();
        int nearest = -1;
        qreal distance = 7.0;
        if (plot.contains(helpEvent->pos()))
        {
            for (int i = 0; i < m_samples.size(); ++i)
            {
                const qreal candidate = qAbs(samplePosition(m_samples.at(i), plot).x() - helpEvent->pos().x());
                if (candidate < distance)
                {
                    distance = candidate;
                    nearest = i;
                }
            }
        }
        if (nearest >= 0)
        {
            const DownloadSpeedSample &sample = m_samples.at(nearest);
            const QString age = QString::number(qMax<qint64>(0, m_nowMs - sample.timestampMs) / 1000.0, 'f', 1);
            QToolTip::showText(helpEvent->globalPos(), tr("%1 s ago\n%2").arg(age, formatSize(true, qMax<qint64>(0, sample.bytesPerSecond))), this);
            m_toolTipShown = true;
        }
        else
        {
            hideOwnToolTip();
            event->ignore();
        }
        return true;
    }
    if (event->type() == QEvent::Hide)
        hideOwnToolTip();
    return QWidget::event(event);
}

void DownloadSpeedWidget::hideOwnToolTip()
{
    if (m_toolTipShown)
    {
        QToolTip::hideText();
        m_toolTipShown = false;
    }
}

void DownloadSpeedWidget::leaveEvent(QEvent *event)
{
    hideOwnToolTip();
    QWidget::leaveEvent(event);
}

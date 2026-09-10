#ifndef TIMELINEBAR_H
#define TIMELINEBAR_H

#include <QWidget>
#include <QVector>

struct TimelineMarker
{
    int id;
    int start;
    int delta;
    qint64 offset; // Total offset strictly after this start, including base delay and duplicate starts.
};

class TimeLineBar : public QWidget
{
    Q_OBJECT
public:
    explicit TimeLineBar(QWidget *parent = nullptr);
    void setData(const QVector<qint64> &original, const QVector<qint64> &adjusted,
                 const QVector<TimelineMarker> &rules, qint64 sourceEnd, int baseDelay);
    void setSelection(int id, qint64 focus);
    QSize sizeHint() const override { return QSize(760, 230); }

signals:
    void addRequested(int sourceMs);
    void ruleSelected(int id);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct MarkerHit { QRectF rect; int index; int lane; QString text; };
    QVector<qint64> data[2];
    QVector<int> bins[2];
    QVector<TimelineMarker> markers;
    QVector<QPair<qint64, qint64>> gaps;
    QVector<MarkerHit> hits;
    qint64 sourceEnd = 0, rangeStart = 0, rangeEnd = 10000, binWidth = 10000;
    qint64 focusTime = -1;
    int baseDelay = 0, selectedId = -1, maxCount = 1;
    QRectF laneRect(int lane) const;
    qreal timeX(qint64 time) const;
    qint64 timeAt(qreal x) const;
    qint64 offsetAt(qint64 time) const;
    int hitAt(const QPointF &pos) const;
    void rebuildBins();
    void rebuildMarkers();
};

#endif // TIMELINEBAR_H

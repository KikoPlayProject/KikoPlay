#ifndef DOWNLOADSPEEDWIDGET_H
#define DOWNLOADSPEEDWIDGET_H

#include <QColor>
#include <QWidget>
#include "Download/downloadspeedhistory.h"

class DownloadSpeedWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor)
public:
    explicit DownloadSpeedWidget(QWidget *parent = nullptr);

    void setSamples(const QVector<DownloadSpeedSample> &samples, qint64 nowMs, int staleAfterMs = 3000);
    void clear();
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    QColor lineColor() const { return m_lineColor; }
    QColor textColor() const;
    QColor gridColor() const;
    void setLineColor(const QColor &color) { m_lineColor = color; update(); }
    void setTextColor(const QColor &color) { m_textColor = color; update(); }
    void setGridColor(const QColor &color) { m_gridColor = color; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRectF plotRect() const;
    QPointF samplePosition(const DownloadSpeedSample &sample, const QRectF &plot) const;
    void updateScale(qint64 peak);
    static double niceCeiling(double value);
    void hideOwnToolTip();

    QVector<DownloadSpeedSample> m_samples;
    qint64 m_nowMs = 0;
    qint64 m_shrinkSinceMs = -1;
    int m_staleAfterMs = 3000;
    double m_ceiling = 1.0;
    QColor m_lineColor = QColor(126, 178, 43);
    QColor m_textColor;
    QColor m_gridColor;
    bool m_toolTipShown = false;
};

#endif // DOWNLOADSPEEDWIDGET_H

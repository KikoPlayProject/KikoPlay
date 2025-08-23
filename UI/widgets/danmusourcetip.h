#ifndef DANMUSOURCETIP_H
#define DANMUSOURCETIP_H

#include <QWidget>
struct DanmuSource;

class DanmuSourceTip : public QWidget
{
    Q_OBJECT
public:
    explicit DanmuSourceTip(const DanmuSource *sourceInfo, bool showCount = false, QWidget *parent = nullptr);
    void setSource(const DanmuSource *sourceInfo, bool showCount = false);

signals:
    void clicked();

    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    // QWidget interface
public:
    QSize sizeHint() const;

private:
    QString text;
    QColor bgColor;

};

#endif // DANMUSOURCETIP_H

#ifndef DANMUSOURCETIP_H
#define DANMUSOURCETIP_H

#include <QWidget>
struct DanmuSource;

class DanmuSourceTip : public QWidget
{
    Q_OBJECT
public:
    explicit DanmuSourceTip(const DanmuSource *sourceInfo, QWidget *parent = nullptr);
    void setSource(const DanmuSource *sourceInfo);

    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event);

    // QWidget interface
public:
    QSize sizeHint() const;

private:
    QString text;
    QColor bgColor;

};

#endif // DANMUSOURCETIP_H

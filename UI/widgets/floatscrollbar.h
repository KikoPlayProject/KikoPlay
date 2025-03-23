#ifndef FLOATSCROLLBAR_H
#define FLOATSCROLLBAR_H

#include <QScrollBar>
class QAbstractScrollArea;
class FloatScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit FloatScrollBar(QScrollBar* originScrollBar, QAbstractScrollArea* parent = nullptr, bool autoHide = true);

private:
    QScrollBar *_originScrollBar{nullptr};
    QAbstractScrollArea *_originScrollArea{nullptr};
    bool _autoHide{true};
protected:
    virtual bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // FLOATSCROLLBAR_H

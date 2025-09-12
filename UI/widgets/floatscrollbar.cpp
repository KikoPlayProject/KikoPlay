#include "floatscrollbar.h"
#include <QEvent>
#include <QAbstractScrollArea>


FloatScrollBar::FloatScrollBar(QScrollBar *originScrollBar, QAbstractScrollArea *parent, bool autoHide) : QScrollBar(parent), _autoHide(autoHide)
{
    setObjectName(QStringLiteral("FloatScrollBar"));
    setContextMenuPolicy(Qt::NoContextMenu);
    Qt::Orientation orientation = originScrollBar->orientation();
    setOrientation(orientation);
    orientation == Qt::Horizontal ? parent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff) : parent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    parent->installEventFilter(this);

    _originScrollArea = parent;
    _originScrollBar = originScrollBar;
    setRange(_originScrollBar->minimum(), _originScrollBar->maximum());
    setVisible(_originScrollBar->maximum() > 0);
    setSingleStep(_originScrollBar->singleStep());
    setPageStep(_originScrollBar->pageStep());

    QObject::connect(_originScrollBar, &QScrollBar::valueChanged, this, &QScrollBar::setValue);
    QObject::connect(this, &QScrollBar::valueChanged, _originScrollBar, &QScrollBar::setValue);
    QObject::connect(_originScrollBar, &QScrollBar::rangeChanged, this, [=](int min, int max){
        setRange(min, max);
        if (_forceHide) return;
        if (parent->underMouse() && _autoHide) setVisible(max > 0);
        else if (!_autoHide) setVisible(max > 0);
    });
    hide();
}

bool FloatScrollBar::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::Show:
    case QEvent::Resize:
    case QEvent::LayoutRequest:
    {
        raise();
        setSingleStep(_originScrollBar->singleStep());
        setPageStep(_originScrollBar->pageStep());
        int handleWidth = 8;
        if (orientation() == Qt::Horizontal)
        {
            setGeometry(0, _originScrollArea->height() - handleWidth, _originScrollArea->width(), handleWidth);
        }
        else
        {
            setGeometry(_originScrollArea->width() - handleWidth, 0, handleWidth, _originScrollArea->height());
        }
        break;
    }
    case QEvent::Enter:
    {
        if (maximum() > 0 && _autoHide && !_forceHide)
            show();
        break;
    }
    case QEvent::Leave:
    {
        if (maximum() > 0 && _autoHide && !_forceHide)
            hide();
        break;
    }
    default:
    {
        break;
    }
    }
    return QScrollBar::eventFilter(watched, event);
}

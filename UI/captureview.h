#ifndef CAPTUREVIEW_H
#define CAPTUREVIEW_H

#include "framelessdialog.h"
#include <QGraphicsView>
class CaptureListModel;
struct AnimeImage;
class ImageView : public QGraphicsView
{
    Q_OBJECT
public:
    ImageView(QWidget *parent=nullptr);
    void setPixmap(const QPixmap &pixmap);
    virtual void resize();

signals:
    void navigate(bool next);
private:
    QGraphicsPixmapItem *pixmapItem;
    QGraphicsScene *scene;
    qreal scaleX,scaleY;
    bool isScale,initResize;
    // QWidget interface
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

class CaptureView : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit CaptureView(CaptureListModel *captureModel, int curRow, QWidget *parent = nullptr);
private:
    ImageView *view;
    int curRow;
    QPixmap curPixmap;
    const AnimeImage *curItem;
    void setCapture();
    // QWidget interface
protected:
    virtual void resizeEvent(QResizeEvent *event);

    // CFramelessDialog interface
protected:
    virtual void onClose();
};

#endif // CAPTUREVIEW_H

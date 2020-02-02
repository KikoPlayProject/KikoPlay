#include "captureview.h"
#include "MediaLibrary/capturelistmodel.h"
#include "globalobjects.h"
#include <QGraphicsPixmapItem>
#include <QVBoxLayout>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMouseEvent>
#include <QWheelEvent>
CaptureView::CaptureView(CaptureListModel *captureModel, int curRow, QWidget *parent) : CFramelessDialog("",parent,false,true,false)
{
    this->curRow=curRow;
    view=new ImageView(this);
    QVBoxLayout *viewVLayout=new QVBoxLayout(this);
    viewVLayout->setContentsMargins(0,0,0,0);
    viewVLayout->addWidget(view);
    curPixmap=captureModel->getFullCapture(curRow);
    curItem=captureModel->getCaptureItem(curRow);
    setCapture();
    QObject::connect(view,&ImageView::navigate,this,[captureModel,this](bool next){
        int nRow=this->curRow+(next?1:-1);
        curItem=captureModel->getCaptureItem(nRow);
        if(curItem)
        {
            curPixmap=captureModel->getFullCapture(nRow);
            this->curRow=nRow;
            setCapture();
        }
    });
    QAction* actCopy = new QAction(tr("Copy"),this);
    QObject::connect(actCopy, &QAction::triggered,[this](bool)
    {
        QApplication::clipboard()->setPixmap(curPixmap);
    });
    QAction* actSave = new QAction(tr("Save"),this);
    QObject::connect(actSave, &QAction::triggered,[this](bool)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Capture"),curItem->info,
                                    tr("JPEG Images (*.jpg);;PNG Images (*.png)"));
        if(!fileName.isEmpty())
        {
            curPixmap.save(fileName);
        }
    });
    QAction* actRemove = new QAction(tr("Remove"),this);
    QObject::connect(actRemove, &QAction::triggered,[captureModel,this](bool)
    {
        int nRow=this->curRow+1;
        const CaptureItem *nItem=captureModel->getCaptureItem(nRow);
        if(!nItem)
        {
            nRow=this->curRow-1;
            nItem=captureModel->getCaptureItem(nRow);
        }
        captureModel->deleteRow(this->curRow);
        if(!nItem)
        {
            onClose();
        }
        else
        {
            this->curRow=nRow;
            this->curItem=nItem;
            curPixmap=captureModel->getFullCapture(nRow);
            setCapture();
        }
    });
    view->setContextMenuPolicy(Qt::ActionsContextMenu);
    view->addAction(actCopy);
    view->addAction(actSave);
    view->addAction(actRemove);
    resize(GlobalObjects::appSetting->value("DialogSize/CaptureView",QSize(600*logicalDpiX()/96,340*logicalDpiY()/96)).toSize());
}

void CaptureView::setCapture()
{
    view->setPixmap(curPixmap);
    setTitle(QString("%2   %1").arg(QDateTime::fromSecsSinceEpoch(curItem->timeId).toString("yyyy-MM-dd hh:mm:ss"),curItem->info));
}

void CaptureView::resizeEvent(QResizeEvent *event)
{
    CFramelessDialog::resizeEvent(event);
    view->resize();
}

void CaptureView::onClose()
{
    GlobalObjects::appSetting->setValue("DialogSize/CaptureView",size());
    CFramelessDialog::onClose();
}

ImageView::ImageView(QWidget *parent):QGraphicsView (parent),isScale(false),initResize(false)
{
    setObjectName(QStringLiteral("captureImageView"));
    setDragMode(QGraphicsView::ScrollHandDrag);
    setCursor(Qt::ArrowCursor);
    scene=new QGraphicsScene(this);
    setScene(scene);
    pixmapItem=new QGraphicsPixmapItem;
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    scene->addItem(pixmapItem);
}

void ImageView::setPixmap(const QPixmap &pixmap)
{
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    resetMatrix();
    pixmapItem->setPixmap(pixmap);
    fitInView(scene->itemsBoundingRect() ,Qt::KeepAspectRatio);
    scaleX=transform().m11();
    scaleY=transform().m22();
    isScale=false;
}

void ImageView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton && pixmapItem->contains(event->pos()))
    {
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        if(isScale)
        {
            fitInView(scene->itemsBoundingRect() ,Qt::KeepAspectRatio);
        }
        else
        {
            scale(2,2);
        }
        isScale=!isScale;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if(event->modifiers().testFlag(Qt::ControlModifier))
    {
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        if(event->delta()>0)
        {
            scale(1.1,1.1);
            isScale=true;
        }
        else
        {
            if(transform().m11()<=scaleX || transform().m22()<=scaleY)
            {
                resetMatrix();
                scale(scaleX,scaleY);
                isScale=false;
            }
            else
            {
                scale(0.9,0.9);
            }
        }
    }
    else if(!isScale)
    {
        emit navigate(event->delta()<0);
    }
    else
    {
        QGraphicsView::wheelEvent(event);
    }
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    if(!initResize)
    {
        resize();
        initResize=true;
    }
    QGraphicsView::resizeEvent(event);
}

void ImageView::resize()
{
    fitInView(scene->itemsBoundingRect() ,Qt::KeepAspectRatio);
    scaleX=transform().m11();
    scaleY=transform().m22();
}

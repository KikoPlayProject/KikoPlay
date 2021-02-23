#include "captureview.h"
#include "MediaLibrary/capturelistmodel.h"
#include "Play/Video/simpleplayer.h"
#include "globalobjects.h"
#include <QGraphicsPixmapItem>
#include <QStackedLayout>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QWindow>
#include "Common/notifier.h"

CaptureView::CaptureView(CaptureListModel *model, int curRow, QWidget *parent) : CFramelessDialog("",parent,false,true,false), captureModel(model)
{
    this->curRow=curRow;
    view=new ImageView(this);
    smPlayer = new SimplePlayer();
    smPlayer->setWindowFlags(Qt::FramelessWindowHint);
    QWindow *native_wnd  = QWindow::fromWinId(smPlayer->winId());
    QWidget *playerWindowWidget=QWidget::createWindowContainer(native_wnd);
    playerWindowWidget->setParent(this);
    playerWindowWidget->setMinimumSize(400*logicalDpiX()/96, 225*logicalDpiY()/96);
    smPlayer->show();

    QObject::connect(smPlayer, &SimplePlayer::stateChanged, this, [=](SimplePlayer::PlayState state){
       if(state == SimplePlayer::EndReached)
       {
           smPlayer->seek(0);
           smPlayer->setState(SimplePlayer::Play);
       }
    });

    QStackedLayout *viewSLayout=new QStackedLayout(this);
    viewSLayout->setContentsMargins(0,0,0,0);
    viewSLayout->addWidget(view);
    viewSLayout->addWidget(playerWindowWidget);

    curItem=model->getCaptureItem(curRow);
    setCapture();

    QObject::connect(view,&ImageView::navigate, this, &CaptureView::navigate);
    QObject::connect(smPlayer,&SimplePlayer::mouseWheel, this, [=](int delta){
        navigate(delta<0);
    });

    QAction* actCopy = new QAction(tr("Copy"),this);
    QObject::connect(actCopy, &QAction::triggered,[this](bool)
    {
        if(curItem && curItem->type == AnimeImage::CAPTURE)
        {
            QApplication::clipboard()->setPixmap(curPixmap);
        }
    });
    QAction* actSave = new QAction(tr("Save"),this);
    QObject::connect(actSave, &QAction::triggered,[this](bool)
    {
        if(curItem->type == AnimeImage::CAPTURE)
        {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save Capture"), curItem->info, "JPEG Images (*.jpg);;PNG Images (*.png)");
            if(!fileName.isEmpty())
            {
                curPixmap.save(fileName);
            }
        }
        else if(curItem->type == AnimeImage::SNIPPET)
        {
            QString snippetFilePath(captureModel->getSnippetFile(this->curRow));
            if(snippetFilePath.isEmpty())
            {
                showMessage(tr("Snippet %1 Lost").arg(curItem->timeId), NM_HIDE | NM_ERROR);
                return;
            }
            else
            {
                QFileInfo snippetFile(snippetFilePath);
                QString fileName = QFileDialog::getSaveFileName(this, tr("Save Snippet"), curItem->info, QString("(*.%1)").arg(snippetFile.suffix()));
                if(!fileName.isEmpty())
                {
                    QFile::copy(snippetFilePath, fileName);
                }
            }
        }
    });
    QAction* actRemove = new QAction(tr("Remove"),this);
    QObject::connect(actRemove, &QAction::triggered,[this](bool)
    {
        int nRow=this->curRow+1;
        const AnimeImage *nItem=captureModel->getCaptureItem(nRow);
        if(!nItem)
        {
            nRow=this->curRow-1;
            nItem=captureModel->getCaptureItem(nRow);
        }
        else
        {
            nRow = this->curRow;
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
            setCapture();
        }
    });
    view->setContextMenuPolicy(Qt::ActionsContextMenu);
    view->addAction(actCopy);
    view->addAction(actSave);
    view->addAction(actRemove);
    smPlayer->setContextMenuPolicy(Qt::ActionsContextMenu);
    smPlayer->addAction(actSave);
    smPlayer->addAction(actRemove);
    resize(GlobalObjects::appSetting->value("DialogSize/CaptureView",QSize(600*logicalDpiX()/96,340*logicalDpiY()/96)).toSize());
}

CaptureView::~CaptureView()
{
    delete smPlayer;
}


void CaptureView::setCapture()
{
    if(!curItem) return;
    if(curItem->type == AnimeImage::CAPTURE)
    {
        curPixmap=captureModel->getFullCapture(curRow);
        smPlayer->setState(SimplePlayer::Stop);
        static_cast<QStackedLayout *>(layout())->setCurrentIndex(0);
        view->setPixmap(curPixmap);
    }
    else if(curItem->type == AnimeImage::SNIPPET)
    {
        static_cast<QStackedLayout *>(layout())->setCurrentIndex(1);
        QString snippetFile(captureModel->getSnippetFile(curRow));
        if(snippetFile.isEmpty())
        {
            QTimer::singleShot(0, [=](){
                showMessage(tr("Snippet %1 Lost").arg(curItem->timeId), NM_HIDE | NM_ERROR);
            });
        }
        else
        {
            smPlayer->setMedia(snippetFile);
        }
        smPlayer->setFocus();
    }
    setTitle(QString("%2   %1").arg(QDateTime::fromMSecsSinceEpoch(curItem->timeId).toString("yyyy-MM-dd hh:mm:ss"),curItem->info));
}

void CaptureView::navigate(bool next)
{
    int nRow=this->curRow+(next?1:-1);
    curItem=captureModel->getCaptureItem(nRow);
    if(curItem)
    {
        this->curRow=nRow;
        setCapture();
    }
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

void CaptureView::wheelEvent(QWheelEvent *event)
{
    navigate(event->angleDelta().y()<0);
    event->accept();
}

ImageView::ImageView(QWidget *parent):QGraphicsView(parent),isScale(false),initResize(false)
{
    setObjectName(QStringLiteral("captureImageView"));
    setDragMode(QGraphicsView::ScrollHandDrag);
    setCursor(Qt::ArrowCursor);
    setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
    scene=new QGraphicsScene(this);
    setScene(scene);
    pixmapItem=new QGraphicsPixmapItem;
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    scene->addItem(pixmapItem);
}

void ImageView::setPixmap(const QPixmap &pixmap)
{
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    resetTransform();
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
                resetTransform();
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
        event->accept();
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

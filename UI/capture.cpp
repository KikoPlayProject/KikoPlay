#include "capture.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "MediaLibrary/animeworker.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlistitem.h"
Capture::Capture(QImage &captureImage, QWidget *parent, const PlayListItem *item) : CFramelessDialog("",parent)
{
    setResizeable(false);
    imgLabel=new QLabel(this);
    imgLabel->setScaledContents(true);
    aspectRatio = double(captureImage.width())/captureImage.height();
    double w=500*logicalDpiX()/96;
    double h=w/aspectRatio;
    imgLabel->setPixmap(QPixmap::fromImage(captureImage));
    imgLabel->setGeometry(2,2,w,h);
    imgLabel->lower();

    buttonContainer=new QWidget(this);
    buttonContainer->setGeometry(0,h+2,w,50*logicalDpiY()/96);
    QPushButton *copyToClipboard = new KPushButton(tr("Copy to Clipboard"), buttonContainer);
    QObject::connect(copyToClipboard, &QPushButton::clicked, this, [&captureImage,this](){
       QApplication::clipboard()->setImage(captureImage);
       CFramelessDialog::onAccept();
    });
    QPushButton *saveToFile = new KPushButton(tr("Save"), buttonContainer);
    QObject::connect(saveToFile, &QPushButton::clicked, this, [this,&captureImage](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Capture"),"",
                                    tr("JPEG Images (*.jpg);;PNG Images (*.png)"));
        if(!fileName.isEmpty())
        {
            captureImage.save(fileName);
            CFramelessDialog::onAccept();
        }
    });
    QPushButton *addToLibrary = new KPushButton(tr("Add To Library"), buttonContainer);
    addToLibrary->setEnabled(item && !item->animeTitle.isEmpty());
    QObject::connect(addToLibrary,&QPushButton::clicked,[&captureImage,item,this](){
        int curTime=GlobalObjects::mpvplayer->getTime();
        int cmin=curTime/60;
        int cls=curTime-cmin*60;
        QString info=QString("%1:%2 - %3").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')).arg(item->title);
        AnimeWorker::instance()->saveCapture(item->animeTitle, info, captureImage);
        CFramelessDialog::onAccept();
    });
    QHBoxLayout *btnHLayout = new QHBoxLayout(buttonContainer);
    btnHLayout->addWidget(copyToClipboard);
    btnHLayout->addWidget(addToLibrary);
    btnHLayout->addStretch(1);
    btnHLayout->addWidget(saveToFile);
    hExtend = buttonContainer->layout()->sizeHint().height() + 8;
    resize(w+4,h+hExtend);
}

void Capture::resizeEvent(QResizeEvent *event)
{
    double w=500*logicalDpiX()/96;
    double h=w/aspectRatio;
    imgLabel->setGeometry(2,2,w,h);
    imgLabel->lower();
    hExtend = buttonContainer->layout()->sizeHint().height() + 8;
    buttonContainer->setGeometry(0,h+2,w,hExtend);
    CFramelessDialog::resizeEvent(event);
}

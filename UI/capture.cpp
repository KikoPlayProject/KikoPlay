#include "capture.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
Capture::Capture(QImage &captureImage, QWidget *parent) : CFramelessDialog("",parent)
{
    setResizeable(false);
    QLabel *imgLabel=new QLabel(this);
    imgLabel->setScaledContents(true);
    double aspectRatio = double(captureImage.width())/captureImage.height();
    double w=500*logicalDpiX()/96;
    double h=w/aspectRatio;
    imgLabel->setPixmap(QPixmap::fromImage(captureImage));
    imgLabel->setGeometry(2,2,w,h);
    imgLabel->lower();

    QWidget *buttonContainer=new QWidget(this);
    buttonContainer->setGeometry(0,h+2,w,50*logicalDpiY()/96);
    QPushButton *copyToClipboard=new QPushButton(tr("Copy to Clipboard"),buttonContainer);
    QObject::connect(copyToClipboard,&QPushButton::clicked,[&captureImage,this](){
       QApplication::clipboard()->setImage(captureImage);
       CFramelessDialog::onAccept();
    });
    QPushButton *saveToFile=new QPushButton(tr("Save"),buttonContainer);
    QObject::connect(saveToFile,&QPushButton::clicked,[this,&captureImage](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Capture"),"",
                                    tr("JPEG Images (*.jpg);;PNG Images (*.png)"));
        if(!fileName.isEmpty())
        {
            captureImage.save(fileName);
            CFramelessDialog::onAccept();
        }
    });
    QHBoxLayout *btnHLayout=new QHBoxLayout(buttonContainer);
    btnHLayout->addWidget(copyToClipboard);
    btnHLayout->addItem(new QSpacerItem(1,1,QSizePolicy::MinimumExpanding));
    btnHLayout->addWidget(saveToFile);

    resize(w+4,h+50*logicalDpiY()/96);
}

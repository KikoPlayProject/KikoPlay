#include "gifcapture.h"
#include "Play/Video/simpleplayer.h"
#include "Play/Video/mpvplayer.h"
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QGridLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QProcess>
#include <QSharedPointer>
#include "globalobjects.h"
#include "Common/notifier.h"

GIFCapture::GIFCapture(const QString &fileName, bool showDuration,  QWidget *parent) : CFramelessDialog(tr("GIF Capture"),parent)
{
    smPlayer = QSharedPointer<SimplePlayer>::create();
    smPlayer->setWindowFlags(Qt::FramelessWindowHint);
    QWindow *native_wnd  = QWindow::fromWinId(smPlayer->winId());
    QWidget *playerWindowWidget=QWidget::createWindowContainer(native_wnd);
    playerWindowWidget->setMouseTracking(true);
    playerWindowWidget->setParent(this);
    playerWindowWidget->setMinimumSize(400*logicalDpiX()/96, 225*logicalDpiY()/96);
    smPlayer->show();

    QSpinBox *durationSpin = new QSpinBox(this);
    durationSpin->setRange(1, 15);
    QLabel *durationTip = new QLabel(tr("Snippet Length(1~15s)"), this);
    if(!showDuration)
    {
        durationTip->hide();
        durationSpin->hide();
    }

    QLabel *frameSizeTip = new QLabel(tr("Frame Size"), this);
    QLineEdit *frameW = new QLineEdit(this);
    QLineEdit *frameH = new QLineEdit(this);
    QLabel *timesTip = new QLabel(tr("x"), this);
    QCheckBox *constrainScale = new QCheckBox(tr("Constrain Scale"), this);
    constrainScale->setChecked(true);

    QSpinBox *frameRateSpin = new QSpinBox(this);
    frameRateSpin->setRange(1, 60);
    frameRateSpin->setValue(15);
    QLabel *frameRateTip = new QLabel(tr("Frame Rate"), this);

    QIntValidator *validator = new QIntValidator(this);
    validator->setBottom(1);

    frameH->setValidator(validator);
    frameW->setValidator(validator);

    saveFile = new QPushButton(tr("Save To File"), this);

    QGridLayout *sGLayout = new QGridLayout(this);
    sGLayout->addWidget(playerWindowWidget, 0,  0, 1, 11);

    sGLayout->addWidget(frameSizeTip, 1, 0);

    sGLayout->addWidget(frameW, 1, 1);
    sGLayout->addWidget(timesTip, 1, 2);
    sGLayout->addWidget(frameH, 1, 3);
    sGLayout->addWidget(constrainScale, 1, 4);

    sGLayout->addWidget(frameRateTip, 1, 5);
    sGLayout->addWidget(frameRateSpin, 1, 6);

    sGLayout->addWidget(durationTip, 1, 7);
    sGLayout->addWidget(durationSpin, 1, 8);
    sGLayout->addWidget(saveFile, 1, 10);

    sGLayout->setColumnStretch(9, 1);
    sGLayout->setRowStretch(0, 1);
    sGLayout->setContentsMargins(0, 0, 0, 0);

    double curTime = fileName.isEmpty()? GlobalObjects::mpvplayer->getTime() : 0;
    if(showDuration)
    {
        QObject::connect(durationSpin,&QSpinBox::editingFinished,[=](){
           smPlayer->seek(curTime);
        });
        QObject::connect(smPlayer.data(), &SimplePlayer::positionChanged, this, [=](double val){
           int duration = durationSpin->value();
           if(val >= curTime + duration)
           {
               smPlayer->seek(curTime);
           }
        });
    }
    QObject::connect(smPlayer.data(), &SimplePlayer::stateChanged, this, [=](SimplePlayer::PlayState state){
       if(state == SimplePlayer::EndReached)
       {
           smPlayer->seek(curTime);
           smPlayer->setState(SimplePlayer::Play);
       }
    });
    QObject::connect(smPlayer.data(), &SimplePlayer::durationChanged, this, [=](){
        smPlayer->seek(curTime);
        smPlayer->setMute(true);
        QSize videoSize = smPlayer->getVideoSize();
        frameW->setText(QString::number(videoSize.width()));
        frameH->setText(QString::number(videoSize.height()));
    });
    QObject::connect(constrainScale, &QCheckBox::stateChanged, this, [=](int state){
       if(state == Qt::Checked)
       {
           QSize videoSize = smPlayer->getVideoSize();
           double cw = frameW->text().toInt();
           int nh = cw * videoSize.height() / videoSize.width();
           frameH->setText(QString::number(nh));
       }
    });
    QObject::connect(frameH, &QLineEdit::textEdited, this, [=](const QString &text){
        if(constrainScale->isChecked())
        {
            QSize videoSize = smPlayer->getVideoSize();
            double ch = text.toInt();
            int nw = ch * videoSize.width() / videoSize.height();
            frameW->setText(QString::number(nw));
        }
    });
    QObject::connect(frameW, &QLineEdit::textEdited, this, [=](const QString &text){
        if(constrainScale->isChecked())
        {
            QSize videoSize = smPlayer->getVideoSize();
            double cw = text.toInt();
            int nh = cw * videoSize.height() / videoSize.width();
            frameH->setText(QString::number(nh));
        }
    });
    QObject::connect(saveFile, &QPushButton::clicked, this, [=](){

        const QString &inputFile = fileName.isEmpty()?GlobalObjects::mpvplayer->getCurrentFile():fileName;
        int rate = frameRateSpin->value();
        int w = frameW->text().toInt(), h = frameH->text().toInt();
        if(w == 0 || h == 0 )
        {
            showMessage(tr("Frame Width or Height Should not be 0"), NM_ERROR | NM_HIDE);
            return;
        }
        int duration = showDuration?durationSpin->value():-1;
        QString saveFileName = QFileDialog::getSaveFileName(this, tr("Save GIF"), "", "GIF Images (*.gif)");
        if(saveFileName.isEmpty()) return;
        if(ffmpegCut(inputFile, saveFileName, w, h, rate, curTime, duration))
        {
            QProcess::startDetached("Explorer", {"/select,", QDir::toNativeSeparators(saveFileName)});
        }
    });
    smPlayer->setMedia(fileName.isEmpty()?GlobalObjects::mpvplayer->getCurrentFile():fileName);
    setSizeSettingKey("DialogSize/GIFCapture",QSize(500*logicalDpiX()/96,280*logicalDpiY()/96));
}

bool GIFCapture::ffmpegCut(const QString &input, const QString &output, int w, int h, int r, double start, int duration)
{
    QString ffmpegPath = GlobalObjects::appSetting->value("Play/FFmpeg", "ffmpeg").toString();
    QStringList arguments;
    arguments << "-ss" << QString::number(start);
    if(duration > 0)
        arguments << "-t" << QString::number(duration);
    arguments << "-i" << input;
    QString filter;
    if(r > 0)
        filter.append(QString("fps=%1,").arg(r));
    filter.append(QString("scale=%1:%2:flags=lanczos[x];").arg(w).arg(h));
    arguments << "-filter_complex" << filter + "[x]split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse";
    arguments << "-y";
    arguments << output;

    QProcess ffmpegProcess;
    QEventLoop eventLoop;
    bool success = true;
    QObject::connect(&ffmpegProcess, &QProcess::errorOccurred, this, [this, &eventLoop, &success](QProcess::ProcessError error){
       if(error == QProcess::FailedToStart)
       {
           showMessage(tr("Start FFmpeg Failed"), NM_ERROR | NM_HIDE);
       }
       success = false;
       eventLoop.quit();
    });
    QObject::connect(&ffmpegProcess, (void (QProcess:: *)(int, QProcess::ExitStatus))&QProcess::finished, this,
                     [this, &eventLoop, &success](int exitCode, QProcess::ExitStatus exitStatus){
       success = (exitStatus == QProcess::NormalExit && exitCode == 0);
       if(!success)
       {
           showMessage(tr("Generate Failed, FFmpeg exit code: %1").arg(exitCode), NM_ERROR | NM_HIDE);
       }
       eventLoop.quit();
    });
    QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardOutput, this, [&](){
       qInfo()<<ffmpegProcess.readAllStandardOutput();
    });
    QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardError, this, [&]() {
        QString content(ffmpegProcess.readAllStandardError());
        qInfo() << content.replace("\\n", "\n");
    });
    showBusyState(true);
    saveFile->setEnabled(false);
    QTimer::singleShot(0, [&]() {
        ffmpegProcess.start(ffmpegPath, arguments);
    });
    eventLoop.exec();
    showBusyState(false);
    saveFile->setEnabled(true);
    return success;
}

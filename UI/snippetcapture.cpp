#include "snippetcapture.h"
#include "Play/Video/simpleplayer.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Playlist/playlist.h"
#include "MediaLibrary/animeworker.h"
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QFileDialog>
#include <QProcess>
#include <QSharedPointer>
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaSpinBox.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "Common/notifier.h"
#include "Common/logger.h"

namespace
{
    const QString duration2Str(int duration, bool hour = false)
    {
        int cmin=duration/60;
        int cls=duration-cmin*60;
        if(hour)
        {
            int ch = cmin/60;
            cmin = cmin - ch*60;
            return QString("%1:%2:%3").arg(ch, 2, 10, QChar('0')).arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0'));
        }
        else
        {
            return QString("%1:%2").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0'));
        }
    }
}

SnippetCapture::SnippetCapture(QWidget *parent) : CFramelessDialog("",parent)
{
    smPlayer = QSharedPointer<SimplePlayer>::create();
    smPlayer->setWindowFlags(Qt::FramelessWindowHint);
    QWindow *native_wnd  = QWindow::fromWinId(smPlayer->winId());
    QWidget *playerWindowWidget=QWidget::createWindowContainer(native_wnd);
    playerWindowWidget->setMouseTracking(true);
    playerWindowWidget->setParent(this);
    playerWindowWidget->setMinimumSize(400, 225);
    smPlayer->show();

    QSpinBox *durationSpin = new ElaSpinBox(this);
    QCheckBox *retainAudioCheck = new ElaCheckBox(tr("Retain Audio"), this);
    QLabel *durationTip = new QLabel(tr("Snippet Length(1~20s)"), this);
    saveFile = new KPushButton(tr("Save To File"), this);
    addToLibrary=new KPushButton(tr("Add To Library"), this);

    QGridLayout *sGLayout = new QGridLayout(this);
    sGLayout->addWidget(playerWindowWidget, 0,  0, 1, 6);
    sGLayout->addWidget(durationTip, 1, 0);
    sGLayout->addWidget(durationSpin, 1, 1);
    sGLayout->addWidget(retainAudioCheck, 1, 2);
    sGLayout->addWidget(saveFile, 1, 4);
    sGLayout->addWidget(addToLibrary, 1, 5);
    sGLayout->setColumnStretch(3, 1);
    sGLayout->setRowStretch(0, 1);

    double curTime = GlobalObjects::mpvplayer->getTime();
    setTitle(tr("Snippet Capture - %1").arg(duration2Str(curTime)));
    durationSpin->setRange(1, 20);
    retainAudioCheck->setChecked(true);
    const PlayListItem *item=GlobalObjects::playlist->getCurrentItem();
    addToLibrary->setEnabled(item && !item->animeTitle.isEmpty());

    QObject::connect(durationSpin,&QSpinBox::editingFinished,[=](){
       smPlayer->seek(curTime);
    });
    QObject::connect(retainAudioCheck, &QCheckBox::stateChanged, this, [=](int state){
       smPlayer->setMute(state!=Qt::Checked);
    });
    QObject::connect(smPlayer.data(), &SimplePlayer::positionChanged, this, [=](double val){
       int duration = durationSpin->value();
       if(val >= curTime + duration)
       {
           smPlayer->seek(curTime);
       }
    });
    QObject::connect(smPlayer.data(), &SimplePlayer::stateChanged, this, [=](SimplePlayer::PlayState state){
       if(state == SimplePlayer::EndReached)
       {
           smPlayer->seek(curTime);
           smPlayer->setState(SimplePlayer::Play);
       }
    });
    QObject::connect(smPlayer.data(), &SimplePlayer::durationChanged, this, [=](){
        smPlayer->seek(curTime);
    });
    QObject::connect(saveFile, &QPushButton::clicked, this, [=](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Snippet"), GlobalObjects::mpvplayer->getCurrentFile());
        if(fileName.isEmpty()) return;
        if(ffmpegCut(curTime, GlobalObjects::mpvplayer->getCurrentFile(), durationSpin->value(), retainAudioCheck->isChecked(), fileName))
        {
#ifdef Q_OS_WIN
            QProcess::startDetached("Explorer", {"/select,", QDir::toNativeSeparators(fileName)});
#else
            QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(QFileInfo(fileName).absolutePath())));
#endif
        }
    });
    QObject::connect(addToLibrary, &QPushButton::clicked, this, [=](){
        QTemporaryFile tmpImg("XXXXXX.jpg");
        if(tmpImg.open())
        {
            smPlayer->seek(curTime);
            smPlayer->screenshot(tmpImg.fileName());
            QImage captureImage(tmpImg.fileName());
            qint64 timeId = QDateTime::currentDateTime().toMSecsSinceEpoch();
            QString suffix(QFileInfo(GlobalObjects::mpvplayer->getCurrentFile()).suffix());
            QString snippetPath(GlobalObjects::appSetting->value("Play/SnippetPath", GlobalObjects::context()->dataPath + "/snippet").toString());
            QDir dir;
            if(!dir.exists(snippetPath)) dir.mkpath(snippetPath);
            QString fileName(QString("%1/%2.%3").arg(snippetPath, QString::number(timeId), suffix));

            if(ffmpegCut(curTime, GlobalObjects::mpvplayer->getCurrentFile(), durationSpin->value(), retainAudioCheck->isChecked(), fileName))
            {
                const PlayListItem *item=GlobalObjects::playlist->getCurrentItem();
                QString info(QString("%1, %2s - %3").arg(duration2Str(curTime),QString::number(durationSpin->value()), item->title));
                AnimeWorker::instance()->saveSnippet(item->animeTitle, info, timeId, captureImage);
                CFramelessDialog::onAccept();
            }
        }


    });
    smPlayer->setVolume(GlobalObjects::mpvplayer->getVolume());
    smPlayer->setMedia(GlobalObjects::mpvplayer->getCurrentFile());
    setSizeSettingKey("DialogSize/SnippetCapture",QSize(500*logicalDpiX()/96,280*logicalDpiY()/96));
}


bool SnippetCapture::ffmpegCut(double start, const QString &input, int duration, bool retainAudio, const QString &out)
{
    QString ffmpegPath = GlobalObjects::appSetting->value("Play/FFmpeg", "ffmpeg").toString();
    QStringList arguments;
    arguments << "-ss" << QString::number(start);
    arguments << "-i" << input;
    arguments << "-t" << QString::number(duration);
    if(!retainAudio)
        arguments << "-an";
    arguments << "-y";
    arguments << out;

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
        QString content(ffmpegProcess.readAllStandardOutput());
        Logger::logger()->log(Logger::APP, content);
    });
    QObject::connect(&ffmpegProcess, &QProcess::readyReadStandardError, this, [&]() {
        QString content(ffmpegProcess.readAllStandardError());
        Logger::logger()->log(Logger::APP, content);
    });
    showBusyState(true);
    bool addEnable = addToLibrary->isEnabled();
    if(addEnable) addToLibrary->setEnabled(false);
    saveFile->setEnabled(false);
    QTimer::singleShot(0, [&]() {
        ffmpegProcess.start(ffmpegPath, arguments);
    });
    eventLoop.exec();
    showBusyState(false);
    if(addEnable) addToLibrary->setEnabled(true);
    saveFile->setEnabled(true);
    return success;
}

#include "mpvplayer.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QCoreApplication>
#include <QDebug>
#include <QMap>

#include "Play/Danmu/danmurender.h"

MPVPlayer::MPVPlayer(QWidget *parent) : QOpenGLWidget(parent),state(PlayState::Stop),
    danmuRender(nullptr),danmuHide(false),mute(false)
{
    mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
    if (!mpv)
        throw std::runtime_error("could not create mpv context");

    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "msg-level", "all=v");
    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    // Make use of the MPV_SUB_API_OPENGL_CB API.
    mpv::qt::set_option_variant(mpv, "vo", "opengl-cb");

    // Request hw decoding, just for testing.
    mpv::qt::set_option_variant(mpv, "hwdec", "auto");

    //mpv::qt::set_option_variant(mpv, "display-fps", "60");
    //mpv::qt::set_option_variant(mpv, "video-sync", "display-resample");
    mpv::qt::set_option_variant(mpv, "vf", "lavfi=\"fps=fps=60:round=down\"");
	mpv::qt::set_option_variant(mpv, "keep-open", "yes");

    mpv_gl = (mpv_opengl_cb_context *)mpv_get_sub_api(mpv, MPV_SUB_API_OPENGL_CB);
    if (!mpv_gl)
        throw std::runtime_error("OpenGL not compiled in");
    mpv_opengl_cb_set_update_callback(mpv_gl, MPVPlayer::on_update, (void *)this);
    QObject::connect(this, &MPVPlayer::frameSwapped, this,&MPVPlayer::swapped);

    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "playback-time", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "eof-reached", MPV_FORMAT_FLAG);
    //mpv_observe_property(mpv, 0, "sid", MPV_FORMAT_INT64);
    //mpv_observe_property(mpv, 0, "aid", MPV_FORMAT_INT64);

    mpv_set_wakeup_callback(mpv, MPVPlayer::wakeup, this);
    QObject::connect(&refreshTimer,&QTimer::timeout,[this](){
       //maybeUpdate();
       double curTime = mpv::qt::get_property(mpv,QStringLiteral("playback-time")).toDouble();
       emit positionChanged(curTime*1000);
    });
    //refreshTimer.start(1000);
}

MPVPlayer::~MPVPlayer()
{
    makeCurrent();
    if (mpv_gl)
        mpv_opengl_cb_set_update_callback(mpv_gl, NULL, NULL);
    // Until this call is done, we need to make sure the player remains
    // alive. This is done implicitly with the mpv::qt::Handle instance
    // in this class.
    mpv_opengl_cb_uninit_gl(mpv_gl);
}

MPVPlayer::VideoSizeInfo MPVPlayer::getVideoSizeInfo()
{
    VideoSizeInfo sizeInfo;
    sizeInfo.width = mpv::qt::get_property(mpv,"width").toInt();
    sizeInfo.height = mpv::qt::get_property(mpv,"height").toInt();
    sizeInfo.dwidth = mpv::qt::get_property(mpv,"dwidth").toInt();
    sizeInfo.dheight = mpv::qt::get_property(mpv,"dheight").toInt();
    return sizeInfo;
}

QMap<QString, QMap<QString, QString> > MPVPlayer::getMediaInfo()
{
    QMap<QString, QMap<QString, QString> > mediaInfo;
    if(currentFile.isEmpty())return mediaInfo;
    QFileInfo fi(currentFile);

    QMap<QString,QString> fileInfo;
    fileInfo.insert(tr("General"),fi.fileName());
    fileInfo.insert(tr("Title"),mpv::qt::get_property(mpv,"media-title").toString());
    float fileSize=fi.size();
    QStringList units={"B","KB","MB","GB","TB"};
    for(int i=0;i<units.size();++i)
    {
        if(fileSize<1024.0)
        {
            fileInfo.insert(tr("File Size"),QString().setNum(fileSize,'f',2)+units[i]);
            break;
        }
        fileSize/=1024.0;
    }
    fileInfo.insert(tr("Date created"),fi.created().toString());
    int duration=mpv::qt::get_property(mpv,"duration").toDouble() * 1000;
    QTime time = QTime::fromMSecsSinceStartOfDay(duration);
    fileInfo.insert(tr("Media length"),duration>=3600000?time.toString("h:mm:ss"):duration>=60000?time.toString("mm:ss"):time.toString("00:ss"));
    mediaInfo.insert(tr("File"),fileInfo);

    QMap<QString,QString> videoInfo;
    videoInfo.insert(tr("General"),mpv::qt::get_property(mpv,"video-codec").toString());
    videoInfo.insert(tr("Video Output"),QString("%0 (hwdec %1)").arg(mpv::qt::get_property(mpv,"current-vo").toString())
                                                                .arg(mpv::qt::get_property(mpv,"hwdec-current").toString()));
    videoInfo.insert(tr("Resolution"),QString("%0 x %1").arg(mpv::qt::get_property(mpv,"width").toInt())
                                                      .arg(mpv::qt::get_property(mpv,"height").toInt()));
    videoInfo.insert(tr("Actual FPS"),QString().setNum(mpv::qt::get_property(mpv,"estimated-vf-fps").toDouble()));
    videoInfo.insert(tr("A/V Sync"),QString().setNum(mpv::qt::get_property(mpv,"avsync").toDouble(),'f',2));
    videoInfo.insert(tr("Bitrate"),QString("%0 bps").arg(mpv::qt::get_property(mpv,"video-bitrate").toDouble()));
    mediaInfo.insert(tr("Video"),videoInfo);

    QMap<QString,QString> audioInfo;
    audioInfo.insert(tr("General"),QString("%0 track(s) %1").arg(audioTrack.ids.count()).arg(mpv::qt::get_property(mpv,"audio-codec").toString()));
    audioInfo.insert(tr("Audio Output"),mpv::qt::get_property(mpv,"current-ao").toString());
    QMap<QString,QVariant> audioParameters=mpv::qt::get_property(mpv,"audio-params").toMap();
    audioInfo.insert(tr("Sample Rate"),audioParameters["samplerate"].toString());
    audioInfo.insert(tr("Channels"),audioParameters["channel-count"].toString());
    audioInfo.insert(tr("Bitrate"),QString("%0 bps").arg(mpv::qt::get_property(mpv,"audio-bitrate").toDouble()));
    mediaInfo.insert(tr("Audio"),audioInfo);

    QMap<QString,QString> metaInfo;
    QMap<QString,QVariant> metadata=mpv::qt::get_property(mpv,"metadata").toMap();
    for(auto iter=metadata.begin();iter!=metadata.end();++iter)
		metaInfo.insert(iter.key(),iter.value().toString());
    mediaInfo.insert(tr("Meta Data"),metaInfo);

    return mediaInfo;
}

void MPVPlayer::setMedia(QString file)
{
    if(!setMPVCommand(QStringList() << "loadfile" << file))
    {
        currentFile=file;
		state = PlayState::Play;
        refreshTimer.start(1000);
        QCoreApplication::processEvents();
		emit stateChanged(state);
    }
}

void MPVPlayer::setState(MPVPlayer::PlayState newState)
{
    if(newState==state)return;
    switch (newState)
    {
    case PlayState::Play:
    {
        setMPVProperty("pause",false);
    }
        break;
    case PlayState::Pause:
    {
        setMPVProperty("pause",true);
    }
        break;
    case PlayState::Stop:
    {
        setMPVCommand(QVariantList()<<"stop");
        refreshTimer.stop();
        state=PlayState::Stop;
        emit stateChanged(state);
    }
        break;
    default:
        break;
    }
}

void MPVPlayer::seek(int pos,bool relative)
{
    QCoreApplication::instance()->processEvents();
	if (relative)
	{
		setMPVCommand(QVariantList() << "seek" << pos);
		double curTime = mpv::qt::get_property(mpv, "playback-time").toDouble();
		emit positionJumped(curTime * 1000);
	}
	else
	{
		setMPVCommand(QVariantList() << "seek" << (double)pos / 1000 << "absolute");
		emit positionJumped(pos);
	}
}

void MPVPlayer::frameStep(bool forward)
{
	setMPVCommand(QVariantList()<<(forward?"frame-step":"frame-back-step"));
}

void MPVPlayer::setVolume(int vol)
{
    volume = qBound(0, vol, 100);
    setMPVProperty("ao-volume",volume);
}

void MPVPlayer::setMute(bool mute)
{
    setMPVProperty("ao-mute",mute);
    this->mute=mute;
}

void MPVPlayer::setDanmuRender(DanmuRender *render)
{
    this->danmuRender=render;
}

void MPVPlayer::hideDanmu(bool hide)
{
    danmuHide=hide;
}

void MPVPlayer::addSubtitle(QString path)
{
	setMPVCommand(QVariantList() << "sub-add" << path);
	audioTrack.desc_str.clear();
	audioTrack.ids.clear();
	subtitleTrack.desc_str.clear();
	subtitleTrack.ids.clear();
	loadTracks();
	emit trackInfoChange(1);
}

void MPVPlayer::setTrackId(int type, int id)
{
    setMPVProperty(type==0?"aid":"sid",type==0?audioTrack.ids[id]:subtitleTrack.ids[id]);
}

void MPVPlayer::hideSubtitle(bool on)
{
    setMPVProperty("sub-visibility",on?"no":"yes");
}

void MPVPlayer::setSubDelay(int delay)
{
    setMPVProperty("sub-delay",delay);
}

void MPVPlayer::setSpeed(int index)
{
    if(index>=speedLevel.count())return;
	double speed = speedLevel.at(index).toDouble();
    setMPVProperty("speed",speed);
}

void MPVPlayer::setVideoAspect(int index)
{
    setMPVProperty("video-aspect", videoAspcetVal[index]);
}

void MPVPlayer::screenshot(QString filename)
{
    setMPVCommand(QVariantList() << "screenshot-to-file" << filename);
}

void MPVPlayer::initializeGL()
{
    int r = mpv_opengl_cb_init_gl(mpv_gl, NULL, MPVPlayer::get_proc_address, NULL);
    if (r < 0)
        throw std::runtime_error("could not initialize OpenGL");
}

void MPVPlayer::paintGL()
{
    mpv_opengl_cb_draw(mpv_gl, defaultFramebufferObject(), width(), -height());
    if(danmuRender && !danmuHide)
    {
        QOpenGLFramebufferObject::bindDefault();
        QOpenGLPaintDevice fboPaintDevice(width(), height());
        QPainter painter(&fboPaintDevice);
        painter.setRenderHints(QPainter::SmoothPixmapTransform);
        danmuRender->drawDanmu(painter);
    }
}

void MPVPlayer::swapped()
{
    mpv_opengl_cb_report_flip(mpv_gl, 0);
    if(danmuRender && state==PlayState::Play)
    {
        float step(0.f);
        if(elapsedTimer.isValid())
            step=elapsedTimer.restart();
        else
            elapsedTimer.start();
        //qDebug()<<"FPS:"<<1000/step;
        //if(step>17.f)step=16.f;

            danmuRender->moveDanmu(step);
    }
}

void MPVPlayer::on_mpv_events()
{
    while (mpv)
    {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;
        handle_mpv_event(event);
    }
}

void MPVPlayer::maybeUpdate()
{
    if (window()->isMinimized())
    {
        makeCurrent();
        paintGL();
        context()->swapBuffers(context()->surface());
        swapped();
        doneCurrent();
    } else
    {
        update();
    }
}

void MPVPlayer::handle_mpv_event(mpv_event *event)
{
    switch (event->event_id)
    {
    case MPV_EVENT_PROPERTY_CHANGE:
    {
        mpv_event_property *prop = (mpv_event_property *)event->data;
        if (strcmp(prop->name, "playback-time") == 0)
        {
            if (prop->format == MPV_FORMAT_DOUBLE)
            {
                double time = *(double *)prop->data;
                //qDebug()<<"new time:"<<time;
                if(state==PlayState::Pause)emit positionChanged(time*1000);
            }
        }
        else if (strcmp(prop->name, "duration") == 0)
        {
            if (prop->format == MPV_FORMAT_DOUBLE)
            {
                double time = *(double *)prop->data;
                emit durationChanged(time*1000);
            }
        }
        else if (strcmp(prop->name, "pause") == 0)
        {
            if (prop->format == MPV_FORMAT_FLAG)
            {
                int flag = *(int *)prop->data;
                state=flag?PlayState::Pause:PlayState::Play;
                //if(!refreshTimer.isActive())
               //     refreshTimer.start(1000);
                if(state==PlayState::Pause)
                {
                    elapsedTimer.invalidate();
                    refreshTimer.stop();
                }
                else
                {
                    refreshTimer.start(1000);
                }
                emit stateChanged(state);
            }
        }
        else if (strcmp(prop->name, "eof-reached") == 0)
        {
            if (prop->format == MPV_FORMAT_FLAG)
            {
                int flag = *(int *)prop->data;
                if(flag)
                {
                    state = PlayState::EndReached;
                    refreshTimer.stop();
					emit stateChanged(state);
                }
            }
        }
        /*else if (strcmp(prop->name,"sid"))
		{
			if (prop->format == MPV_FORMAT_INT64)
				subtitleTrack.current=subtitleTrack.ids.indexOf(*(int*)prop->data);
		}
		else if (strcmp(prop->name, "aid"))
		{
			if (prop->format == MPV_FORMAT_INT64)
				audioTrack.current = audioTrack.ids.indexOf(*(int*)prop->data);
        }*/
        break;
    }
    case MPV_EVENT_FILE_LOADED:
        setMPVProperty("pause", false);
        setMPVProperty("ao-volume",volume);
		setMPVProperty("ao-mute", mute);
		loadTracks();
		emit trackInfoChange(0);
		emit trackInfoChange(1);
        break;
    case MPV_EVENT_END_FILE:
        update();
        break;
    default: ;
        // Ignore uninteresting or unknown events.
    }
}

void MPVPlayer::on_update(void *ctx)
{
    QMetaObject::invokeMethod((MPVPlayer*)ctx, "maybeUpdate");
}

void MPVPlayer::wakeup(void *ctx)
{
    QMetaObject::invokeMethod((MPVPlayer*)ctx, "on_mpv_events", Qt::QueuedConnection);
}

void *MPVPlayer::get_proc_address(void *ctx, const char *name)
{
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx)
        return NULL;
    return (void *)glctx->getProcAddress(QByteArray(name));
}

void MPVPlayer::loadTracks()
{
    QVariantList allTracks=mpv::qt::get_property(mpv,"track-list").toList();
	audioTrack.desc_str.clear();
	audioTrack.ids.clear();
	subtitleTrack.desc_str.clear();
	subtitleTrack.ids.clear();
	for (QVariant &track : allTracks)
	{
		QMap<QString, QVariant> trackMap = track.toMap();
		QString type(trackMap["type"].toString());
		if (type == "audio")
		{
			QString title(trackMap["title"].toString());
			audioTrack.desc_str.append(title.isEmpty()? trackMap["id"].toString():title);
			audioTrack.ids.append(trackMap["id"].toInt());
		}
		else if (type == "sub")
		{
			QString title(trackMap["title"].toString());
			subtitleTrack.desc_str.append(title.isEmpty() ? trackMap["id"].toString() : title);
			subtitleTrack.ids.append(trackMap["id"].toInt());
		}
	}
}

int MPVPlayer::setMPVCommand(const QVariant &params)
{
    return mpv::qt::get_error(mpv::qt::command_variant(mpv, params));
}

void MPVPlayer::setMPVProperty(const QString &name, const QVariant &value)
{
    int val(0);
    val = mpv::qt::set_property(mpv,name,value);
}

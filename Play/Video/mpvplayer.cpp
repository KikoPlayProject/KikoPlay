#include "mpvplayer.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QCoreApplication>
#include <QDebug>
#include <QMap>

#include "Play/Danmu/Render/danmurender.h"
#include "globalobjects.h"
namespace
{
const char *vShaderDanmu =
        "attribute mediump vec4 a_VtxCoord;\n"
        "attribute mediump vec2 a_TexCoord;\n"
        "varying mediump vec2 v_vTexCoord;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = a_VtxCoord;\n"
        "    v_vTexCoord = a_TexCoord;\n"
        "}\n";

const char *fShaderDanmu =
        "#ifdef GL_ES\n"
        "precision lowp float;\n"
        "#endif\n"
        "varying mediump vec2 v_vTexCoord;\n"
        "uniform sampler2D u_SamplerD;\n"
        "uniform float alpha;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor.rgba = texture2D(u_SamplerD, v_vTexCoord).bgra;\n"
        "    gl_FragColor.a *= alpha;\n"
        "}\n";
}
MPVPlayer::MPVPlayer(QWidget *parent) : QOpenGLWidget(parent),state(PlayState::Stop),
    mute(false),danmuHide(false),currentDuration(0)
{
    mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
    if (!mpv)
        throw std::runtime_error("could not create mpv context");

    //mpv_set_option_string(mpv, "terminal", "yes");
    //mpv_set_option_string(mpv, "msg-level", "all=v");
    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    mpv_request_log_messages(mpv, "v");
    QStringList options=GlobalObjects::appSetting->value("Play/MPVParameters",
                                                         "#Make sure the danmu is smooth\n"
                                                         "vf=lavfi=\"fps=fps=60:round=down\"\n"
                                                         "hwdec=auto").toString().split('\n');
    for(const QString &option:options)
    {
        QString opt(option.trimmed());
        if(opt.startsWith('#'))continue;
        int eqPos=opt.indexOf('=');
		mpv::qt::set_option_variant(mpv, opt.left(eqPos), opt.mid(eqPos+1));
    }
    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "keep-open", "yes");  
    // Make use of the MPV_SUB_API_OPENGL_CB API.
    mpv::qt::set_option_variant(mpv, "vo", "opengl-cb");
    /* for svp test-------------------
    mpv::qt::set_option_variant(mpv,"input-ipc-server","mpvpipe");
    mpv::qt::set_option_variant(mpv,"hwdec-codecs","all");
    mpv::qt::set_option_variant(mpv,"hr-seek-framedrop","no");
    mpv::qt::set_option_variant(mpv,"no-resume-playback","");
    */

    mpv_gl = (mpv_opengl_cb_context *)mpv_get_sub_api(mpv, MPV_SUB_API_OPENGL_CB);
    if (!mpv_gl) throw std::runtime_error("OpenGL not compiled in");
    mpv_opengl_cb_set_update_callback(mpv_gl, MPVPlayer::on_update, (void *)this);
    QObject::connect(this, &MPVPlayer::frameSwapped, this,&MPVPlayer::swapped);

    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "playback-time", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "eof-reached", MPV_FORMAT_FLAG);

    mpv_set_wakeup_callback(mpv, MPVPlayer::wakeup, this);
    QObject::connect(&refreshTimer,&QTimer::timeout,[this](){
       double curTime = mpv::qt::get_property(mpv,QStringLiteral("playback-time")).toDouble();
       emit positionChanged(curTime*1000);
    });
}

MPVPlayer::~MPVPlayer()
{
    makeCurrent();
    if (mpv_gl) mpv_opengl_cb_set_update_callback(mpv_gl, nullptr, nullptr);
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
        if(fileSize<1024.f)
        {
            fileInfo.insert(tr("File Size"),QString().setNum(fileSize,'f',2)+units[i]);
            break;
        }
        fileSize/=1024.f;
    }
    fileInfo.insert(tr("Date created"),fi.birthTime().toString());
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

void MPVPlayer::drawTexture(GLuint texture, float alpha, const QRectF &rect)
{
    static GLfloat vtx[8];
    static GLfloat tex[8]={0,0,1,0,0,1,1,1};
    GLfloat h = 2.f / width(), v = 2.f / height();
    GLfloat l = rect.left()*h - 1, r = rect.right()*h - 1, t = 1 - rect.top()*v, b = 1 - rect.bottom()*v;
    vtx[0] = l; vtx[1] = t;
    vtx[2] = r; vtx[3] = t;
    vtx[4] = l; vtx[5] = b;
    vtx[6] = r; vtx[7] = b;

	danmuShader.bind();
	danmuShader.setUniformValue("alpha", alpha);
    danmuShader.setUniformValue("u_SamplerD", 0);
    danmuShader.setAttributeArray(0, vtx, 2);
    danmuShader.setAttributeArray(1, tex, 2);
    danmuShader.enableAttributeArray(0);
    danmuShader.enableAttributeArray(1);

    QOpenGLFunctions *glFuns=context()->functions();
    glFuns->glEnable(GL_BLEND);
    glFuns->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glFuns->glActiveTexture(GL_TEXTURE0);
    glFuns->glBindTexture(GL_TEXTURE_2D, texture);
    glFuns->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void MPVPlayer::setMedia(QString file)
{
    if(!setMPVCommand(QStringList() << "loadfile" << file))
    {
        currentFile=file;
		state = PlayState::Play;
        refreshTimer.start(timeRefreshInterval);
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
    int r = mpv_opengl_cb_init_gl(mpv_gl, nullptr, MPVPlayer::get_proc_address, nullptr);
    if (r < 0)
        throw std::runtime_error("could not initialize OpenGL");

    danmuShader.addShaderFromSourceCode(QOpenGLShader::Vertex, vShaderDanmu);
    danmuShader.addShaderFromSourceCode(QOpenGLShader::Fragment, fShaderDanmu);
    danmuShader.link();
    danmuShader.bind();
    danmuShader.bindAttributeLocation("a_VtxCoord", 0);
    danmuShader.bindAttributeLocation("a_TexCoord", 1);

    emit initContext();
}

void MPVPlayer::paintGL()
{
    mpv_opengl_cb_draw(mpv_gl, defaultFramebufferObject(), width(), -height());
    if(!danmuHide)
    {
        QOpenGLFramebufferObject::bindDefault();
        QOpenGLPaintDevice fboPaintDevice(width(), height());
        QPainter painter(&fboPaintDevice);
        painter.beginNativePainting();
        GlobalObjects::danmuRender->drawDanmu();
        painter.endNativePainting();
    }
}

void MPVPlayer::swapped()
{
    mpv_opengl_cb_report_flip(mpv_gl, 0);
    if(state==PlayState::Play)
    {
        float step(0.f);
        if(elapsedTimer.isValid())
            step=elapsedTimer.restart();
        else
            elapsedTimer.start();
        GlobalObjects::danmuRender->moveDanmu(step);
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
    }
    else
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
                if(state==PlayState::Pause) emit positionChanged(time*1000);
            }
        }
        else if (strcmp(prop->name, "duration") == 0)
        {
            if (prop->format == MPV_FORMAT_DOUBLE)
            {
                double time = *(double *)prop->data;
                currentDuration=time;
                emit durationChanged(time*1000);
            }
        }
        else if (strcmp(prop->name, "pause") == 0)
        {
            if (prop->format == MPV_FORMAT_FLAG)
            {
                int flag = *(int *)prop->data;
                state=flag?PlayState::Pause:PlayState::Play;
                if(state==PlayState::Pause)
                {
                    elapsedTimer.invalidate();
                    refreshTimer.stop();
                }
                else
                {
                    refreshTimer.start(timeRefreshInterval);
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
        break;
    }
    case MPV_EVENT_FILE_LOADED:
    {
        setMPVProperty("pause", false);
        setMPVProperty("ao-volume",volume);
		setMPVProperty("ao-mute", mute);
		loadTracks();
		emit trackInfoChange(0);
		emit trackInfoChange(1);
        break;
    }
    case MPV_EVENT_END_FILE:
    {
        update();
        break;
    }
    case MPV_EVENT_LOG_MESSAGE:
    {
        struct mpv_event_log_message *msg = (struct mpv_event_log_message *)event->data;
        emit showLog(QString("[%1]%2: %3").arg(msg->prefix).arg(msg->level).arg(msg->text));
        break;
    }
    default:
        break;
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
    if (!glctx) return nullptr;
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
    mpv::qt::set_property(mpv,name,value);
}

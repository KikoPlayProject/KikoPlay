#include "mpvplayer.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <QMap>
#include <clocale>
#include <Common/logger.h>
#include "Play/Danmu/Render/danmurender.h"
#include "globalobjects.h"
namespace
{
const char *vShaderDanmu =
        "attribute mediump vec4 a_VtxCoord;\n"
        "attribute mediump vec2 a_TexCoord;\n"
        "attribute mediump float a_Tex;\n"
        "varying mediump vec2 v_vTexCoord;\n"
        "varying float texId;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = a_VtxCoord;\n"
        "    v_vTexCoord = a_TexCoord;\n"
        "    texId = a_Tex;\n"
        "}\n";

const char *fShaderDanmu =
        "#ifdef GL_ES\n"
        "precision lowp float;\n"
        "#endif\n"
        "varying mediump vec2 v_vTexCoord;\n"
        "varying float texId;\n"
        "uniform sampler2D u_SamplerD[16];\n"
        "uniform float alpha;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor.rgba = texture2D(u_SamplerD[int(texId)], v_vTexCoord).bgra;\n"
        "    gl_FragColor.a *= alpha;\n"
        "}\n";
const char *vShaderDanmu_Old =
        "attribute mediump vec4 a_VtxCoord;\n"
        "attribute mediump vec2 a_TexCoord;\n"
        "varying mediump vec2 v_vTexCoord;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = a_VtxCoord;\n"
        "    v_vTexCoord = a_TexCoord;\n"
        "}\n";

const char *fShaderDanmu_Old =
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
#ifdef Q_OS_WIN
#pragma comment (lib,"user32.lib")
#pragma comment (lib,"gdi32.lib")
static QString get_color_profile(HWND hwnd)
{
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEXW mi;
    mi.cbSize = sizeof mi;
    GetMonitorInfo(monitor, (MONITORINFO*)&mi);
    QString name;

    HDC ic = CreateICW(mi.szDevice, nullptr, nullptr, nullptr);
    if(ic)
    {
        wchar_t wname[MAX_PATH + 1];
        DWORD bufSize(MAX_PATH);
        if (GetICMProfile(ic, &bufSize, wname))
            name = QString::fromWCharArray(wname);
    }
    if (ic)
        DeleteDC(ic);
    return name;
}
#endif
}
MPVPlayer::MPVPlayer(QWidget *parent) : QOpenGLWidget(parent),state(PlayState::Stop),
    mute(false),danmuHide(false),oldOpenGLVersion(false),currentDuration(0), mpvPreview(nullptr), previewThread(nullptr)
{
    std::setlocale(LC_NUMERIC, "C");
    mpv = mpv_create();
    if (!mpv)
        throw std::runtime_error("could not create mpv context");

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    mpv_request_log_messages(mpv, "v");
    QStringList options=GlobalObjects::appSetting->value(
         "Play/MPVParameters",
         "#Make sure the danmu is smooth\n"
         "vf=lavfi=\"fps=fps=60:round=down\"\n"
         "hwdec=auto").toString().split('\n');
    for(const QString &option:options)
    {
        QString opt(option.trimmed());
        if(opt.startsWith('#'))continue;
        int eqPos=opt.indexOf('=');
        if(eqPos==-1) eqPos = opt.length();
        QString key(opt.left(eqPos)), val(opt.mid(eqPos+1));
        mpv::qt::set_option_variant(mpv, key, val);
        optionsMap.insert(key, val);
    }
    using ShortCutInfo = QPair<QString, QPair<QString,QString>>;
    auto shortcutList = GlobalObjects::appSetting->value("Play/MPVShortcuts").value<QList<ShortCutInfo>>();
    for(auto &s : shortcutList)
    {
        modifyShortcut(s.first, s.first, s.second.first);
    }

    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "keep-open", "yes");  

    QObject::connect(this, &MPVPlayer::frameSwapped, this,&MPVPlayer::swapped);

    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "playback-time", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "eof-reached", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "sub-delay", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "speed", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "brightness", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "contrast", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "saturation", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "gamma", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "hue", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "sharpen", MPV_FORMAT_DOUBLE);

    mpv_set_wakeup_callback(mpv, MPVPlayer::wakeup, this);
    QObject::connect(&refreshTimer,&QTimer::timeout,[this](){
       double curTime = mpv::qt::get_property(mpv,QStringLiteral("playback-time")).toDouble();
       emit positionChanged(curTime*1000);
    });

    if(GlobalObjects::appSetting->value("Play/ShowPreview", true).toBool())
    {
        mpvPreview = new MPVPreview(QSize(220*logicalDpiX()/96,124*logicalDpiY()/96));
        previewThread = new QThread();
        previewThread->start();
        mpvPreview->moveToThread(previewThread);
        QObject::connect(mpvPreview, &MPVPreview::previewDown, this, &MPVPlayer::refreshPreview);
    }
}

MPVPlayer::~MPVPlayer()
{
    makeCurrent();
    if (mpv_gl) mpv_render_context_free(mpv_gl);
    mpv_terminate_destroy(mpv);
    if(mpvPreview)
    {
        previewThread->quit();
        previewThread->wait();
        mpvPreview->deleteLater();
    }
}

QString MPVPlayer::getMPVProperty(const QString &property, bool &hasError)
{
     auto ret = mpv::qt::get_property(mpv, property);
     hasError = true;
     if(mpv::qt::is_error(ret)) return "";
     hasError = false;
     return ret.toString();
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

QString MPVPlayer::expandMediaInfo(const QString &text)
{
    return mpv::qt::command(mpv, QVariantList() << "expand-text" << text).toString();
}

void MPVPlayer::setOptions()
{
    if(optionsMap.contains("icc-profile-auto"))
    {
#ifdef Q_OS_WIN
        if(this->parent())
        {
            QWidget *pWidget = dynamic_cast<QWidget *>(this->parent());
            if(pWidget)
            {
                QString iccProfile(get_color_profile((HWND)pWidget->winId()));
                mpv::qt::set_option_variant(mpv, "icc-profile",iccProfile);
            }
        }
#endif
    }
}

void MPVPlayer::drawTexture(QList<const DanmuObject *> &objList, float alpha)
{
    static QVector<GLfloat> vtx(6*2*64),tex(6*2*64),texId(6*64);
    static int Textures[] ={GL_TEXTURE0,GL_TEXTURE1,GL_TEXTURE2,GL_TEXTURE3,
                            GL_TEXTURE4,GL_TEXTURE5,GL_TEXTURE6,GL_TEXTURE7,
                           GL_TEXTURE8,GL_TEXTURE9,GL_TEXTURE10,GL_TEXTURE11,
                           GL_TEXTURE12,GL_TEXTURE13,GL_TEXTURE14,GL_TEXTURE15};
    static GLuint u_SamplerD[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    static QHash<GLuint, int> texHash;
    if(objList.size()>vtx.size()/12)
    {
        vtx.resize(objList.size()*12);
        tex.resize(objList.size()*12);
        texId.resize(objList.size()*6);
    }
    QOpenGLFunctions *glFuns=context()->functions();
    GLfloat h = 2.f / (width()*devicePixelRatioF()), v = 2.f / (height()*devicePixelRatioF());
    int allowTexCount=oldOpenGLVersion?0:15;
    while(!objList.isEmpty())
    {
        texHash.clear();
        int i=0;
        int textureId=0;
        while(!objList.isEmpty())
        {
            if(textureId>allowTexCount) break;
            const DanmuObject *obj=objList.takeFirst();

            GLfloat l = obj->x*h - 1,r = (obj->x+obj->drawInfo->width)*h - 1,
                    t = 1 - obj->y*v,b = 1 - (obj->y+obj->drawInfo->height)*v;

            vtx[i]=l;     vtx[i+1]=t;
            vtx[i+2] = r; vtx[i+3] = t;
            vtx[i+4] = l; vtx[i+5] = b;

            vtx[i+6] = r;  vtx[i+7] = t;
            vtx[i+8] = l;  vtx[i+9] = b;
            vtx[i+10] = r; vtx[i+11] = b;

            tex[i]=obj->drawInfo->l;     tex[i+1]=obj->drawInfo->t;
            tex[i+2] = obj->drawInfo->r; tex[i+3] = obj->drawInfo->t;
            tex[i+4] = obj->drawInfo->l; tex[i+5] = obj->drawInfo->b;

            tex[i+6] = obj->drawInfo->r;  tex[i+7] = obj->drawInfo->t;
            tex[i+8] = obj->drawInfo->l;  tex[i+9] = obj->drawInfo->b;
            tex[i+10] = obj->drawInfo->r; tex[i+11] = obj->drawInfo->b;


            int texIndex=texHash.value(obj->drawInfo->texture,-1);
            if(texIndex<0)
            {
                texIndex=textureId++;
                glFuns->glActiveTexture(Textures[texIndex]);
                glFuns->glBindTexture(GL_TEXTURE_2D, obj->drawInfo->texture);
                texHash.insert(obj->drawInfo->texture, texIndex);
            }

            texId[i/2]=texIndex;
            texId[i/2+1]=texIndex;
            texId[i/2+2]=texIndex;
            texId[i/2+3]=texIndex;
            texId[i/2+4]=texIndex;
            texId[i/2+5]=texIndex;

            i+=12;
        }

        danmuShader.bind();
        danmuShader.setUniformValue("alpha", alpha);
        danmuShader.setAttributeArray(0, vtx.constData(), 2);
        danmuShader.setAttributeArray(1, tex.constData(), 2);
        danmuShader.enableAttributeArray(0);
        danmuShader.enableAttributeArray(1);
        if(oldOpenGLVersion)
        {
            danmuShader.setUniformValue("u_SamplerD", 0);
        }
        else
        {
            danmuShader.setUniformValueArray("u_SamplerD", u_SamplerD,16);
            danmuShader.setAttributeArray(2,texId.constData(),1);
            danmuShader.enableAttributeArray(2);
        }

        glFuns->glEnable(GL_BLEND);
        glFuns->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glFuns->glDrawArrays(GL_TRIANGLES, 0, i/2);
    }
}

void MPVPlayer::modifyShortcut(const QString &key, const QString &newKey, const QString &command)
{
    if(key != newKey)
    {
        mpvShortcuts.remove(key);
    }
    if(newKey.isEmpty() || command.isEmpty()) return;
    QList<QStringList> commands;
    QStringList commandStrs = command.split(';', Qt::SkipEmptyParts);
    for(const QString &cStr : commandStrs)
    {
        QStringList commandParts;
        QString curPart;
        int state = 0;
        bool escape = false;
        for(QChar c : cStr)
        {
            if(state==0) {
                if(escape) {
                    curPart.append('\\');
                    curPart.append(c);
                    escape = false;
                } else if(c.isSpace()) {
					if(!curPart.isEmpty())
						commandParts.append(curPart);
                    state = 1;
                    curPart.clear();
                } else if(c=='\'') {
                    state = 2;
                } else if(c=='"') {
                    state = 3;
                } else {
                    curPart.append(c);
                    escape = (c=='\\');
                }
            } else if(state==1) {
                if(!c.isSpace()) {
                    curPart.append(c);
                    state = 0;
                }
            } else if(state==2) {
                if(escape) {
                    curPart.append('\\');
                    curPart.append(c);
                    escape = false;
                } else if(c=='\'') {
                    state = 0;
                } else {
                    curPart.append(c);
                    escape = (c=='\\');
                }
            } else if(state==3) {
                if(escape) {
                    curPart.append('\\');
                    curPart.append(c);
                    escape = false;
                } else if(c=='"') {
                    state = 0;
                } else {
                    curPart.append(c);
                    escape = (c=='\\');
                }
            }
        }
        if(!curPart.isEmpty()) commandParts.append(curPart);
        commands.append(commandParts);
    }
    mpvShortcuts[newKey] = {commands, command};
}

int MPVPlayer::runShortcut(const QString &key)
{
    if(!mpvShortcuts.contains(key)) return -1;
    const auto &shortcut = mpvShortcuts[key];
    int ret = 0;
    for(const auto &command : shortcut.first)
    {
        ret = setMPVCommand(command);
    }
    return ret;
}

void MPVPlayer::setMedia(const QString &file)
{
    if(!setMPVCommand(QStringList() << "loadfile" << file))
    {
        currentFile=file;
		state = PlayState::Play;      
        refreshTimer.start(timeRefreshInterval);
        setMPVProperty("pause",false);
        setMPVProperty("ao-volume",volume);
        setMPVProperty("ao-mute", mute);
        if(mpvPreview) mpvPreview->reset(file);
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
        currentFile = "";
        if(mpvPreview) mpvPreview->reset();
        update();
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

void MPVPlayer::addSubtitle(const QString &path)
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

void MPVPlayer::setSpeed(double speed)
{
    speed = std::max(speed, 0.0);
    setMPVProperty("speed",speed);
}

void MPVPlayer::setVideoAspect(int index)
{
    setMPVProperty("video-aspect", videoAspcetVal[index]);
}

void MPVPlayer::screenshot(const QString &filename)
{
    setMPVCommand(QVariantList() << "screenshot-to-file" << filename);
}

void MPVPlayer::setBrightness(int val)
{
    mpv::qt::set_property(mpv, "brightness", qBound(-100, val, 100));
}

void MPVPlayer::setContrast(int val)
{
    mpv::qt::set_property(mpv, "contrast", qBound(-100, val, 100));
}

void MPVPlayer::setSaturation(int val)
{
    mpv::qt::set_property(mpv, "saturation", qBound(-100, val, 100));
}

void MPVPlayer::setGamma(int val)
{
    mpv::qt::set_property(mpv, "gamma", qBound(-100, val, 100));
}

void MPVPlayer::setHue(int val)
{
    mpv::qt::set_property(mpv, "hue", qBound(-100, val, 100));
}

void MPVPlayer::setSharpen(int val)
{
    mpv::qt::set_property(mpv, "sharpen", qBound(-4.0, val / 100.0, 4.0));
}

void MPVPlayer::initializeGL()
{
    QOpenGLFunctions *glFuns=context()->functions();
    glFuns->initializeOpenGLFunctions();
    mpv_opengl_init_params gl_init_params{get_proc_address, nullptr};
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
        throw std::runtime_error("failed to initialize mpv GL context");
    mpv_render_context_set_update_callback(mpv_gl, MPVPlayer::on_update, reinterpret_cast<void *>(this));

    const char *version = reinterpret_cast<const char*>(glFuns->glGetString(GL_VERSION));
    qInfo()<<"OpenGL Version:"<<version;
    int mainVersion=0;
	while (version && *version >= '0' && *version <= '9')
	{
		mainVersion = mainVersion * 10 + *version - '0';
		++version;
	}
    oldOpenGLVersion = mainVersion<4;
    if(oldOpenGLVersion) qInfo()<<"Unsupport sampler2D Array";
    bool useSample2DArray = GlobalObjects::appSetting->value("Play/Sampler2DArray",true).toBool();
	if (!oldOpenGLVersion && !useSample2DArray) oldOpenGLVersion = true;
    if(oldOpenGLVersion)
    {
        danmuShader.addShaderFromSourceCode(QOpenGLShader::Vertex, vShaderDanmu_Old);
        danmuShader.addShaderFromSourceCode(QOpenGLShader::Fragment, fShaderDanmu_Old);
        danmuShader.link();
        danmuShader.bind();
        danmuShader.bindAttributeLocation("a_VtxCoord", 0);
        danmuShader.bindAttributeLocation("a_TexCoord", 1);
    }
    else
    {
        danmuShader.addShaderFromSourceCode(QOpenGLShader::Vertex, vShaderDanmu);
        danmuShader.addShaderFromSourceCode(QOpenGLShader::Fragment, fShaderDanmu);
        danmuShader.link();
        danmuShader.bind();
        danmuShader.bindAttributeLocation("a_VtxCoord", 0);
        danmuShader.bindAttributeLocation("a_TexCoord", 1);
        danmuShader.bindAttributeLocation("a_Tex", 2);
    }
    doneCurrent();
    emit initContext();
}

void MPVPlayer::paintGL()
{
    mpv_opengl_fbo mpfbo{static_cast<int>(defaultFramebufferObject()),
                static_cast<int>(width()*devicePixelRatioF()),  static_cast<int>(height()*devicePixelRatioF()), 0};
    int flip_y{1};

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    // See render_gl.h on what OpenGL environment mpv expects, and
    // other API details.
    mpv_render_context_render(mpv_gl, params);
    if(!danmuHide)
    {
        QOpenGLFramebufferObject::bindDefault();
        QOpenGLPaintDevice fboPaintDevice(width()*devicePixelRatioF(), height()*devicePixelRatioF());
        QPainter painter(&fboPaintDevice);
        painter.beginNativePainting();
        GlobalObjects::danmuRender->drawDanmu();
        painter.endNativePainting();
    }
}

void MPVPlayer::swapped()
{
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
    static QHash<QString, std::function<void()>> propertyFunc = {
    {
        "playback-time",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_DOUBLE)
            {
                double time = *(double *)prop->data;
                if(state==PlayState::Pause) emit positionChanged(time*1000);
            }
        }
    },
    {
        "duration",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_DOUBLE)
            {
                double time = *(double *)prop->data;
                currentDuration=time;
                emit durationChanged(time*1000);
            }
        }
    },
    {
        "pause",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
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
    },
    {
        "eof-reached",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
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
    },
    {
        "chapter-list",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_NODE) {
                QVariantList chapters=mpv::qt::node_to_variant((mpv_node *)prop->data).toList();
                this->chapters.clear();
                for(auto &cp : chapters)
                {
                    auto mp = cp.toMap();
                    this->chapters.append(ChapterInfo({mp["title"].toString(), static_cast<int>(mp["time"].toInt()*1000)}));
                }
                emit chapterChanged();
            }
        }
    },
    {
        "track-list",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            QVariantList allTracks=mpv::qt::node_to_variant((mpv_node *)prop->data).toList();
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
            emit trackInfoChange(0); //audio
            emit trackInfoChange(1); //subtitle
        }
    },
    {
        "sub-delay",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t sub_delay = *(int64_t *)prop->data;
                emit subDelayChanged(sub_delay);
            }
        }
    },
    {
        "speed",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_DOUBLE) {
                double speed = *(double *)prop->data;
                emit speedChanged(speed);
            }
        }
    },
    {
        "brightness",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t brightness = *(int64_t *)prop->data;
                emit brightnessChanged(brightness);
            }
        }
    },
    {
        "contrast",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t contrast = *(int64_t *)prop->data;
                emit contrastChanged(contrast);
            }
        }
    },
    {
        "saturation",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t saturation = *(int64_t *)prop->data;
                emit saturationChanged(saturation);
            }
        }
    },
    {
        "gamma",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t gamma = *(int64_t *)prop->data;
                emit gammaChanged(gamma);
            }
        }
    },
    {
        "hue",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t hue = *(int64_t *)prop->data;
                emit hueChanged(hue);
            }
        }
    },
    {
        "sharpen",
        [this, event](){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_DOUBLE) {
                double sharpen = *(double *)prop->data;
                emit sharpenChanged(sharpen);
            }
        }
    }
    };
    switch (event->event_id)
    {
    case MPV_EVENT_PROPERTY_CHANGE:
    {
        mpv_event_property *prop = (mpv_event_property *)event->data;
        if(propertyFunc.contains(prop->name))
        {
            propertyFunc[prop->name]();
        }
        break;
    }
    case MPV_EVENT_FILE_LOADED:
    {        
        emit fileChanged();
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
        Logger::logger()->log(Logger::MPV, QString("[%1][%2]: %3").arg(msg->prefix, msg->level, msg->text).trimmed());
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

void MPVPlayer::loadChapters()
{
    QVariantList chapters=mpv::qt::get_property(mpv,"chapter-list").toList();
    this->chapters.clear();
    for(auto &cp : chapters)
    {
		auto mp = cp.toMap();
        this->chapters.append(ChapterInfo({mp["title"].toString(), static_cast<int>(mp["time"].toInt()*1000)}));
    }
}

int MPVPlayer::setMPVCommand(const QVariant &params)
{
    return mpv::qt::get_error(mpv::qt::command(mpv, params));
}

void MPVPlayer::setMPVProperty(const QString &name, const QVariant &value)
{
    mpv::qt::set_property(mpv,name,value);
}

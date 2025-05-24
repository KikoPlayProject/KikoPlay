#include "mpvplayer.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QCoreApplication>
#include <QApplication>
//#include <QDesktopWidget>
#include <QDebug>
#include <QMap>
#include <clocale>
#include <Common/logger.h>
#include <Common/notifier.h>
#include "Play/Danmu/Render/danmurender.h"
#include "globalobjects.h"
#ifdef QT_DEBUG
#include "Common/counter.h"
#endif
#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#define SETTING_KEY_DANMUHIDE "Play/HideDanmu"
#define SETTING_KEY_VIDEO_ASPECT "Play/VidoeAspectRatio"
#define SETTING_KEY_PLAY_SPEED "Play/PlaySpeed"
#define SETTING_KEY_BRIGHTNESS "Play/Brightness"
#define SETTING_KEY_CONTRAST "Play/Contrast"
#define SETTING_KEY_SATURATION "Play/Saturation"
#define SETTING_KEY_GAMMA "Play/Gamma"
#define SETTING_KEY_HUE "Play/Hue"
#define SETTING_KEY_SHARPEN "Play/Sharpen"
#define SETTING_KEY_CLICK_BEHAVIOR "Play/ClickBehavior"
#define SETTING_KEY_DB_CLICK_BEHAVIOR "Play/DBClickBehavior"
#define SETTING_KEY_SHOW_PREVIEW "Play/ShowPreview"
#define SETTING_KEY_SHOW_RECENT "Play/ShowRecent"
#define SETTING_KEY_USE_SAMPLE2DARRAY "Play/Sampler2DArray"
#define SETTING_KEY_SUB_AUTO "Play/SubAuto"
#define SETTING_KEY_SUB_FONT "Play/SubFont"
#define SETTING_KEY_SUB_FONT_SIZE "Play/SubFontSize"
#define SETTING_KEY_SUB_BOLD "Play/SubBold"
#define SETTING_KEY_SUB_COLOR "Play/SubColor"
#define SETTING_KEY_SUB_OUTLINE_COLOR "Play/SubOutlineColor"
#define SETTING_KEY_SUB_BACK_COLOR "Play/SubBackColor"
#define SETTING_KEY_SUB_OUTLINE_SIZE "Play/SubOutlineSize"
#define SETTING_KEY_SUB_BORDER_STYLE "Play/SubBorderStyle"


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

static QSet<QString> optionsBeforeInit{"config", "config-dir", "input-conf", "load-scripts", "script", "scripts"};

}
MPVPlayer::MPVPlayer(QWidget *parent) : QOpenGLWidget(parent),state(PlayState::Stop),
    mute(false),curIsLocalFile(false),danmuHide(false),oldOpenGLVersion(false), cacheSpeed(0),currentDuration(0), mpvPreview(nullptr), previewThread(nullptr)
{
    std::setlocale(LC_NUMERIC, "C");
    mpv = mpv_create();
    if (!mpv) throw std::runtime_error("could not create mpv context");

    loadSettings();
    loadOptions();
    const QMap<QString, QString> &optionsMap = optionsGroupMap[curOptionGroup];
    for(auto iter = optionsMap.cbegin(); iter != optionsMap.cend(); ++iter)
    {
        if(optionsBeforeInit.contains(iter.key()))
        {
            int ret = mpv::qt::set_option_variant(mpv, iter.key(), iter.value());
            if(ret != MPV_ERROR_SUCCESS)
            {
                Logger::logger()->log(Logger::MPV, QString("[kiko][option error]: %1, %2").arg(iter.key(), QString::number(ret)));
            }
        }
    }

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    mpv_request_log_messages(mpv, "v");
    for (auto iter = optionsMap.cbegin(); iter != optionsMap.cend(); ++iter)
    {
        if(optionsBeforeInit.contains(iter.key())) continue;
        int ret = mpv::qt::set_option_variant(mpv, iter.key(), iter.value());
        if(ret != MPV_ERROR_SUCCESS)
        {
            Logger::logger()->log(Logger::MPV, QString("[kiko][option error]: %1, %2").arg(iter.key(), QString::number(ret)));
        }
    }

    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "keep-open", "yes");
    mpv_set_option_string(mpv, "vo", "libmpv");

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
    mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "seekable", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "paused-for-cache", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "cache-buffering-state", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "cache-speed", MPV_FORMAT_INT64);

    mpv_set_wakeup_callback(mpv, MPVPlayer::wakeup, this);
    refreshTimestamp = QDateTime::currentMSecsSinceEpoch();
    QObject::connect(&refreshTimer,&QTimer::timeout,[this](){
        if(currentFile.isEmpty()) return;
        qint64 ct = QDateTime::currentMSecsSinceEpoch();
        if(ct - refreshTimestamp > timeRefreshInterval)
        {
            qInfo()<<"force refresh delta: "<<ct-refreshTimestamp;
            refreshTimestamp = ct;
            double curTime = mpv::qt::get_property(mpv,QStringLiteral("playback-time")).toDouble();
            emit positionChanged(curTime*1000);
        }
    });

    if (isShowPreview)
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

int MPVPlayer::getCurrentTrack(MPVPlayer::TrackType type) const
{
    const QVector<TrackInfo> &tracks = type==AudioTrack?audioTracks:subTracks;
    int curId = mpv::qt::get_property(mpv, type==AudioTrack?"aid":"sid").toInt();
    int index = -1;
    for(int i = 0; i < tracks.size(); ++i)
    {
        if(tracks[i].id == curId)
        {
            index = i;
            break;
        }
    }
    return index;
}

int MPVPlayer::getExternalTrackCount(MPVPlayer::TrackType type) const
{
    const QVector<TrackInfo> &tracks = type==AudioTrack?audioTracks:subTracks;
    int c = 0;
    for(const TrackInfo &t : tracks) c += t.isExternal?1:0;
    return c;
}

QString MPVPlayer::getMPVProperty(const QString &property, int &errCode)
{
    auto ret = getMPVPropertyVariant(property, errCode);
    return ret.toString();
}

QVariant MPVPlayer::getMPVPropertyVariant(const QString &property, int &errCode)
{
    auto ret = mpv::qt::get_property(mpv, property);
    errCode = mpv::qt::get_error(ret);
    return ret;
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

void MPVPlayer::setIccProfileOption()
{
    if(optionsGroupMap[curOptionGroup].contains("icc-profile-auto"))
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

void MPVPlayer::drawTexture(QVector<const DanmuObject *> &objList, float alpha)
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
#ifdef QT_DEBUG
        Counter::instance()->countValue("texture.frame", textureId);
#endif
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

void MPVPlayer::setMedia(const QString &file)
{
    if(!setMPVCommand(QStringList() << "loadfile" << file))
    {
        currentFile=file;
        state = PlayState::Play;
        refreshTimestamp = QDateTime::currentMSecsSinceEpoch();
        refreshTimer.start(timeRefreshInterval);
        setMPVProperty("pause",false);
        setMPVProperty("volume",volume);
        setMPVProperty("mute", mute);
        curIsLocalFile = QFileInfo::exists(file);
        if(mpvPreview && curIsLocalFile) mpvPreview->reset(file);
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
    volume = qBound(0, vol, 130);
    setMPVProperty("volume",volume);
}

void MPVPlayer::setMute(bool mute)
{
    setMPVProperty("mute",mute);
    this->mute=mute;
}

void MPVPlayer::hideDanmu(bool hide)
{
    danmuHide = hide;
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMUHIDE, hide);
}

void MPVPlayer::addSubtitle(const QString &path)
{
    setMPVCommand(QVariantList() << "sub-add" << path << "cached");
	loadTracks();
    //emit trackInfoChange(SubTrack);
}

void MPVPlayer::addAudioTrack(const QString &path)
{
    setMPVCommand(QVariantList() << "audio-add" << path << "cached");
    loadTracks();
    //emit trackInfoChange(AudioTrack);
}

void MPVPlayer::clearExternalAudio()
{
    bool changed = false;
    for(int i = 0; i < audioTracks.size(); ++i)
    {
        if(audioTracks[i].isExternal)
        {
            changed = true;
            setMPVCommand(QVariantList() << "audio-remove" << audioTracks[i].id);
        }
    }
    if(changed)
    {
        loadTracks();
        emit trackInfoChange(AudioTrack);
    }
}

void MPVPlayer::setTrackId(TrackType type, int index)
{
    if(index < 0 || index >= (type==0?audioTracks.size():subTracks.size())) return;
    setMPVProperty(type==AudioTrack?"aid":"sid",type==AudioTrack?audioTracks[index].id:subTracks[index].id);
}

void MPVPlayer::hideSubtitle(bool on)
{
    setMPVProperty("sub-visibility",on?"no":"yes");
}

void MPVPlayer::setSubDelay(int delay)
{
    setMPVProperty("sub-delay",delay);
}

void MPVPlayer::clearExternalSub()
{
    bool changed = false;
    for(int i = 0; i < subTracks.size(); ++i)
    {
        if(subTracks[i].isExternal)
        {
            changed = true;
            setMPVCommand(QVariantList() << "sub-remove" << subTracks[i].id);
        }
    }
    if(changed)
    {
        loadTracks();
        emit trackInfoChange(SubTrack);
    }
}

void MPVPlayer::setSpeed(double speed)
{
    speed = qMax(speed, 0.0);
    playSpeed = speed;
    setMPVProperty("speed",speed);
    GlobalObjects::appSetting->setValue(SETTING_KEY_PLAY_SPEED, playSpeed);
}

void MPVPlayer::setVideoAspect(int index)
{
    setMPVProperty("video-aspect", videoAspectVal[index]);
    videoAspectIndex = index;
    GlobalObjects::appSetting->setValue(SETTING_KEY_VIDEO_ASPECT, index);
}

void MPVPlayer::screenshot(const QString &filename)
{
    setMPVCommand(QVariantList() << "screenshot-to-file" << filename);
}

void MPVPlayer::setBrightness(int val)
{
    val = qBound(-100, val, 100);
    mpv::qt::set_property(mpv, "brightness", val);
    GlobalObjects::appSetting->setValue(SETTING_KEY_BRIGHTNESS, val);
}

void MPVPlayer::setContrast(int val)
{
    val = qBound(-100, val, 100);
    mpv::qt::set_property(mpv, "contrast", val);
    GlobalObjects::appSetting->setValue(SETTING_KEY_CONTRAST, val);
}

void MPVPlayer::setSaturation(int val)
{
    val = qBound(-100, val, 100);
    mpv::qt::set_property(mpv, "saturation", val);
    GlobalObjects::appSetting->setValue(SETTING_KEY_SATURATION, val);
}

void MPVPlayer::setGamma(int val)
{
    val = qBound(-100, val, 100);
    mpv::qt::set_property(mpv, "gamma", val);
    GlobalObjects::appSetting->setValue(SETTING_KEY_GAMMA, val);
}

void MPVPlayer::setHue(int val)
{
    val = qBound(-100, val, 100);
    mpv::qt::set_property(mpv, "hue", val);
    GlobalObjects::appSetting->setValue(SETTING_KEY_HUE, val);
}

void MPVPlayer::setSharpen(int val)
{
    double v = qBound(-4.0, val / 100.0, 4.0);
    mpv::qt::set_property(mpv, "sharpen", v);
    GlobalObjects::appSetting->setValue(SETTING_KEY_SHARPEN, (int)(v * 100));
}

void MPVPlayer::setClickBehavior(int val)
{
    clickBehavior = val;
    GlobalObjects::appSetting->setValue(SETTING_KEY_CLICK_BEHAVIOR, clickBehavior);
}

void MPVPlayer::setDbClickBehavior(int val)
{
    dbClickBehaivior = val;
    GlobalObjects::appSetting->setValue(SETTING_KEY_DB_CLICK_BEHAVIOR, dbClickBehaivior);
}

void MPVPlayer::setShowPreview(bool on)
{
    GlobalObjects::appSetting->setValue(SETTING_KEY_SHOW_PREVIEW, on);
}

void MPVPlayer::setShowRecent(bool on)
{
    isShowRecent = on;
    GlobalObjects::appSetting->setValue(SETTING_KEY_SHOW_RECENT, on);
    emit showRecentChanged(on);
}

void MPVPlayer::setUseSample2DArray(bool on)
{
    useSample2DArray = on;
    GlobalObjects::appSetting->setValue(SETTING_KEY_USE_SAMPLE2DARRAY, on);
}

void MPVPlayer::setSubAuto(const QString val)
{
    subAuto = val;
    setMPVProperty("sub-auto", subAuto);
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_AUTO, val);
}

void MPVPlayer::setSubFont(const QString val)
{
    subFont = val;
    setMPVProperty("sub-font", subFont);
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_FONT, val);
}

void MPVPlayer::setSubFontSize(int val)
{
    subFontSize = val;
    setMPVProperty("sub-font-size", subFontSize);
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_FONT_SIZE, val);
}

void MPVPlayer::setSubBold(bool on)
{
    subBold = on;
    setMPVProperty("sub-bold", subBold ? "yes" : "no");
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_BOLD, subBold);
}

void MPVPlayer::setSubColor(QColor color)
{
    subColor = color.rgba();
    setMPVProperty("sub-color", color.name(QColor::HexArgb));
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_COLOR, subColor);
}

void MPVPlayer::setSubOutlineColor(QColor color)
{
    subOutlineColor = color.rgba();
    setMPVProperty("sub-outline-color", color.name(QColor::HexArgb));
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_OUTLINE_COLOR, subOutlineColor);
}

void MPVPlayer::setSubBackColor(QColor color)
{
    subBackColor = color.rgba();
    setMPVProperty("sub-back-color", color.name(QColor::HexArgb));
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_BACK_COLOR, subBackColor);
}

void MPVPlayer::setSubOutlineSize(double size)
{
    subOutlineSize = size;
    setMPVProperty("sub-outline-size", subOutlineSize);
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_OUTLINE_SIZE, subOutlineSize);
}

void MPVPlayer::setSubBorderStyle(const QString val)
{
    subBorderStyle = val;
    setMPVProperty("sub-border-style", subBorderStyle);
    GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_BORDER_STYLE, val);
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
    if (oldOpenGLVersion) qInfo()<<"Unsupport sampler2D Array";
    useSample2DArray = GlobalObjects::appSetting->value(SETTING_KEY_USE_SAMPLE2DARRAY, true).toBool();
	if (!oldOpenGLVersion && !useSample2DArray) oldOpenGLVersion = true;
    if (oldOpenGLVersion)
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
    if (state == PlayState::Stop)
    {
        QOpenGLFunctions *glFuns=context()->functions();
        glFuns->glClearColor(0, 0, 0, 0);
        return;
    }
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
    if (!danmuHide)
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
    static QHash<QString, std::function<void(mpv_event *event)>> propertyFunc = {
    {
        "playback-time",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_DOUBLE)
            {
                refreshTimestamp = QDateTime::currentMSecsSinceEpoch();
                double time = *(double *)prop->data;
                emit positionChanged(time*1000);
            }
        }
    },
    {
        "duration",
        [this](mpv_event *event){
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
        [this](mpv_event *event){
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
                    refreshTimestamp = QDateTime::currentMSecsSinceEpoch();
                    refreshTimer.start(timeRefreshInterval);
                }
                emit stateChanged(state);
            }
        }
    },
    {
        "eof-reached",
        [this](mpv_event *event){
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
        [this](mpv_event *event){
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
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            QVariantList allTracks=mpv::qt::node_to_variant((mpv_node *)prop->data).toList();
            loadTracks(allTracks);
            emit trackInfoChange(AudioTrack);
            emit trackInfoChange(SubTrack);
        }
    },
    {
        "sub-delay",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t sub_delay = *(int64_t *)prop->data;
                emit subDelayChanged(sub_delay);
            }
        }
    },
    {
        "speed",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_DOUBLE) {
                playSpeed = *(double *)prop->data;
                emit speedChanged(playSpeed);
            }
        }
    },
    {
        "brightness",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t brightness = *(int64_t *)prop->data;
                emit brightnessChanged(brightness);
            }
        }
    },
    {
        "contrast",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t contrast = *(int64_t *)prop->data;
                emit contrastChanged(contrast);
            }
        }
    },
    {
        "saturation",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t saturation = *(int64_t *)prop->data;
                emit saturationChanged(saturation);
            }
        }
    },
    {
        "gamma",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t gamma = *(int64_t *)prop->data;
                emit gammaChanged(gamma);
            }
        }
    },
    {
        "hue",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t hue = *(int64_t *)prop->data;
                emit hueChanged(hue);
            }
        }
    },
    {
        "sharpen",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_DOUBLE) {
                double sharpen = *(double *)prop->data;
                emit sharpenChanged(sharpen);
            }
        }
    },
    {
        "volume",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64) {
                int64_t volume = *(int64_t *)prop->data;
                emit volumeChanged(volume);
            }
        }
    },
    {
        "mute",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_FLAG)
            {
                int flag = *(int *)prop->data;
                this->mute=flag;
                emit muteChanged(flag);
            }
        }
    },
    {
        "seekable",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_FLAG)
            {
                int flag = *(int *)prop->data;
                this->seekable=flag;
                emit seekableChanged(flag);
            }
        }
    },
    {
        "cache-buffering-state",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64)
            {
                int64_t state = *(int64_t *)prop->data;
                emit bufferingStateChanged(state);
            }
        }
    },
    {
        "cache-speed",
        [this](mpv_event *event){
            mpv_event_property *prop = (mpv_event_property *)event->data;
            if (prop->format == MPV_FORMAT_INT64)
            {
                cacheSpeed = *(int64_t *)prop->data;
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
            propertyFunc[prop->name](event);
        }
        break;
    }
    case MPV_EVENT_FILE_LOADED:
    {        
        emit fileChanged();
        if (!curIsLocalFile)
        {
            const QVariantMap info = {
                {"type", "fileinfo"},
            };
            Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, "", 0, info);
        }
        break;
    }
    case MPV_EVENT_START_FILE:
    {
        const QVariantMap info = {
            {"timeout", 180000},
            {"type", "fileinfo"},
        };
        Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, "", 0, info);
        if (!curIsLocalFile)
        {
            Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, tr("Opening: %1").arg(currentFile), 0, info);
        }
        break;
    }
    case MPV_EVENT_END_FILE:
    {
        const QVariantMap info = {
            {"type", "fileinfo"},
        };
        Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, "", 0, info);
        struct mpv_event_end_file *ev = (struct mpv_event_end_file *)event->data;
        if (ev->reason == MPV_END_FILE_REASON_ERROR)
        {
            Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, tr("Open Failed: %1").arg(ev->error), 0, info);
        }
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

void MPVPlayer::loadOptions()
{
    QVector<QStringList> optionsGroupList = GlobalObjects::appSetting->value("Play/MPVParameterGroups").value<QVector<QStringList>>();
    optionGroupKeys = GlobalObjects::appSetting->value("Play/ParameterGroupKeys").toStringList();

    if(optionGroupKeys.size() < optionsGroupList.size())
    {
        optionsGroupList.resize(optionGroupKeys.size());
    }
    if (!GlobalObjects::appSetting->value("Play/PredefineOptionLoaded", false).toBool())
    {
        loadPredefineOptions(optionGroupKeys, optionsGroupList);
    }

    curOptionGroup = GlobalObjects::appSetting->value("Play/DefaultParameterGroup", "default").toString();
    QFile defaultOptionFile(":/res/mpvOptions");
    defaultOptionFile.open(QFile::ReadOnly);
    QStringList defaultOptions = GlobalObjects::appSetting->value("Play/MPVParameters", QString(defaultOptionFile.readAll())).toString().split('\n');
    optionGroupKeys.prepend("default");
    optionsGroupList.prepend(defaultOptions);

    optionsGroupMap.clear();
    for(int i = 0; i < optionGroupKeys.size(); ++i)
    {
        QMap<QString, QString> &optionsMap = optionsGroupMap[optionGroupKeys[i]];
        for(const QString &option : optionsGroupList[i])
        {
            QString opt(option.trimmed());
            if(opt.isEmpty() || opt.startsWith('#')) continue;
            int eqPos = opt.indexOf('=');
            if(eqPos == -1) eqPos = opt.length();
            optionsMap.insert(opt.left(eqPos), opt.mid(eqPos+1));
        }
    }
    if(!optionsGroupMap.contains(curOptionGroup)) curOptionGroup = "default";
    emit optionGroupChanged();
}

void MPVPlayer::loadSettings()
{
    danmuHide = GlobalObjects::appSetting->value(SETTING_KEY_DANMUHIDE, false).toBool();
    playSpeed = GlobalObjects::appSetting->value(SETTING_KEY_PLAY_SPEED, "1").toDouble();
    setMPVProperty("speed", playSpeed);

    videoAspectIndex = GlobalObjects::appSetting->value(SETTING_KEY_VIDEO_ASPECT, 0).toInt();
    setMPVProperty("video-aspect", videoAspectVal[videoAspectIndex]);

    brightness = GlobalObjects::appSetting->value(SETTING_KEY_BRIGHTNESS, 0).toInt();
    setMPVProperty("brightness", brightness);

    contrast = GlobalObjects::appSetting->value(SETTING_KEY_CONTRAST, 0).toInt();
    setMPVProperty("contrast", contrast);

    saturation = GlobalObjects::appSetting->value(SETTING_KEY_SATURATION, 0).toInt();
    setMPVProperty("saturation", saturation);

    gamma = GlobalObjects::appSetting->value(SETTING_KEY_GAMMA, 0).toInt();
    setMPVProperty("gamma", gamma);

    hue = GlobalObjects::appSetting->value(SETTING_KEY_HUE, 0).toInt();
    setMPVProperty("hue", hue);

    sharpen = GlobalObjects::appSetting->value(SETTING_KEY_SHARPEN, 0).toInt();
    setMPVProperty("sharpen", sharpen / 100.0);

    clickBehavior = GlobalObjects::appSetting->value(SETTING_KEY_CLICK_BEHAVIOR, 0).toInt();
    dbClickBehaivior = GlobalObjects::appSetting->value(SETTING_KEY_DB_CLICK_BEHAVIOR, 0).toInt();
    isShowPreview = GlobalObjects::appSetting->value(SETTING_KEY_SHOW_PREVIEW, true).toBool();
    isShowRecent = GlobalObjects::appSetting->value(SETTING_KEY_SHOW_RECENT, true).toBool();

    subAuto = GlobalObjects::appSetting->value(SETTING_KEY_SUB_AUTO, "exact").toString();
    setMPVProperty("sub-auto", subAuto);

    subFont = GlobalObjects::appSetting->value(SETTING_KEY_SUB_FONT, "sans-serif").toString();
    setMPVProperty("sub-font", subFont);

    subFontSize = GlobalObjects::appSetting->value(SETTING_KEY_SUB_FONT_SIZE, 38).toInt();
    setMPVProperty("sub-font-size", subFontSize);

    subBold = GlobalObjects::appSetting->value(SETTING_KEY_SUB_BOLD, false).toBool();
    setMPVProperty("sub-bold", subBold ? "yes" : "no");

    subColor = GlobalObjects::appSetting->value(SETTING_KEY_SUB_COLOR, 0xFFFFFFFF).toUInt();
    setMPVProperty("sub-color", QColor(QRgba64::fromArgb32(subColor)).name(QColor::HexArgb));

    subOutlineColor = GlobalObjects::appSetting->value(SETTING_KEY_SUB_OUTLINE_COLOR, 0xFF000000).toUInt();
    setMPVProperty("sub-outline-color", QColor(QRgba64::fromArgb32(subOutlineColor)).name(QColor::HexArgb));

    subBackColor = GlobalObjects::appSetting->value(SETTING_KEY_SUB_BACK_COLOR, 0xAF000000).toUInt();
    setMPVProperty("sub-back-color", QColor(QRgba64::fromArgb32(subBackColor)).name(QColor::HexArgb));

    subOutlineSize = GlobalObjects::appSetting->value(SETTING_KEY_SUB_OUTLINE_SIZE, 1.65).toDouble();
    setMPVProperty("sub-outline-size", subOutlineSize);

    subBorderStyle = GlobalObjects::appSetting->value(SETTING_KEY_SUB_BORDER_STYLE, "outline-and-shadow").toString();
    setMPVProperty("sub-border-style", subBorderStyle);
}

void MPVPlayer::loadPredefineOptions(QStringList &optionGroupKeys, QVector<QStringList> &optionsGroupList)
{
    QFile predefineOptionFile("predefineMPVOptions");
    if (!predefineOptionFile.open(QFile::ReadOnly)) return;

    QSet<QString> groupKeys(optionGroupKeys.begin(), optionGroupKeys.end());
    char buf[2048];
    qint64 lineLength = -1;
    QString currentKey;
    QStringList currentOptions;
    while((lineLength = predefineOptionFile.readLine(buf, sizeof(buf))) > 0)
    {
        QByteArray data(buf, lineLength);
        data = data.trimmed();
        if (data.isEmpty()) continue;
        if (data.size() > 2 && data.front() == '[' && data.back() == ']')
        {
            if (!currentKey.isEmpty() && !currentOptions.isEmpty() && !groupKeys.contains(currentKey))
            {
                optionGroupKeys.append(currentKey);
                optionsGroupList.append(currentOptions);
            }
            currentOptions.clear();
            currentKey = data.mid(1, data.size() - 2);
        }
        else if (!currentKey.isEmpty())
        {
            currentOptions.append(data);
        }
    }
    if (!currentKey.isEmpty() && !currentOptions.isEmpty() && !groupKeys.contains(currentKey))
    {
        optionGroupKeys.append(currentKey);
        optionsGroupList.append(currentOptions);
    }
    GlobalObjects::appSetting->setValue("Play/ParameterGroupKeys", optionGroupKeys);
    GlobalObjects::appSetting->setValue("Play/MPVParameterGroups", QVariant::fromValue(optionsGroupList));
    GlobalObjects::appSetting->setValue("Play/PredefineOptionLoaded", true);
}

bool MPVPlayer::setOptionGroup(const QString &key)
{
    if(!optionsGroupMap.contains(key)) return false;
    const QMap<QString, QString> &optionsMap = optionsGroupMap[key];
    curOptionGroup = key;
    GlobalObjects::appSetting->setValue("Play/DefaultParameterGroup", curOptionGroup);
    Logger::logger()->log(Logger::MPV, QString("[kiko]Switch to option group: %1").arg(curOptionGroup));
    for(auto iter = optionsMap.cbegin(); iter != optionsMap.cend(); ++iter)
    {
        if(optionsBeforeInit.contains(iter.key()))
        {
            Logger::logger()->log(Logger::MPV, QString("[kiko]Option need restart: %1").arg(iter.key()));
            continue;
        }
        int ret = mpv::qt::set_option_variant(mpv, iter.key(), iter.value());
        if(ret != MPV_ERROR_SUCCESS)
        {
            Logger::logger()->log(Logger::MPV, QString("[kiko][option error]: %1, %2").arg(iter.key(), QString::number(ret)));
        }
    }
    return true;
}

void MPVPlayer::loadTracks()
{
    QVariantList allTracks=mpv::qt::get_property(mpv,"track-list").toList();
    loadTracks(allTracks);
}

void MPVPlayer::loadTracks(const QVariantList &allTracks)
{
    audioTracks.clear();
    subTracks.clear();
    for (const QVariant &track : allTracks)
    {
        QMap<QString, QVariant> trackMap = track.toMap();
        QString type(trackMap["type"].toString());
        QString externalFile = trackMap["external-filename"].toString();
        if (type == "audio")
        {
            QString title(trackMap["title"].toString());
            audioTracks.append({
                title.isEmpty()? trackMap["id"].toString():title,
                trackMap["id"].toInt(),
                !externalFile.isEmpty(),
                externalFile,
                trackMap["codec"].toString(),
                trackMap["ff-index"].toInt(),
            });
        }
        else if (type == "sub")
        {
            QString title(trackMap["title"].toString());
            subTracks.append({
                title.isEmpty()? trackMap["id"].toString():title,
                trackMap["id"].toInt(),
                !externalFile.isEmpty(),
                externalFile,
                trackMap["codec"].toString(),
                trackMap["ff-index"].toInt(),
            });
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

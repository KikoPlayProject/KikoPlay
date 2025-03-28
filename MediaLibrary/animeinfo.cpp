#include "animeinfo.h"
#include "animeworker.h"
#include "animeitemdelegate.h"
#include "Common/lrucache.h"
#include "qpainter.h"
#include "qpainterpath.h"
#include "globalobjects.h"
#include <QImageReader>

namespace
{
LRUCache<Anime *, QSharedPointer<QPixmap>> coverCache{"PreviewCover", 128, true},
    rawCoverCache{"RawCover", 16};
}
const QPixmap &Anime::cover(bool onlyCache)
{
    static QPixmap emptyCover;
    if (_coverData.isEmpty()) return emptyCover;
    QSharedPointer<QPixmap> cover = coverCache.get(this);
    if (cover) return *cover;
    if (onlyCache) return emptyCover;
    QBuffer bufferImage(&_coverData);
    bufferImage.open(QIODevice::ReadOnly);
    QImageReader reader(&bufferImage);
    reader.setScaledSize(QSize(AnimeItemDelegate::CoverWidth, AnimeItemDelegate::CoverHeight));
    const float pxR = GlobalObjects::context()->devicePixelRatioF;
    reader.setScaledSize(QSize(AnimeItemDelegate::CoverWidth*pxR, AnimeItemDelegate::CoverHeight*pxR));

    QPixmap dest(AnimeItemDelegate::CoverWidth*pxR, AnimeItemDelegate::CoverHeight*pxR);
    dest.setDevicePixelRatio(pxR);
    dest.fill(Qt::transparent);
    QPainter painter(&dest);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
    QPainterPath path;
    path.addRoundedRect(0, 0, AnimeItemDelegate::CoverWidth, AnimeItemDelegate::CoverHeight, 8, 8);
    painter.setClipPath(path);
    painter.drawPixmap(QRect(0, 0, AnimeItemDelegate::CoverWidth, AnimeItemDelegate::CoverHeight), QPixmap::fromImageReader(&reader));

    cover = QSharedPointer<QPixmap>::create(dest);
    coverCache.put(this, cover);
    return *cover;
}

const QPixmap &Anime::rawCover()
{
    QSharedPointer<QPixmap> cover = rawCoverCache.get(this);
    if(cover) return *cover;
    QPixmap tmp;
    if(!_coverData.isEmpty())
    {
        tmp.loadFromData(_coverData);
    }
    cover = QSharedPointer<QPixmap>::create(tmp);
    rawCoverCache.put(this, cover);
    return *cover;
}

Anime::Anime() : _addTime(0), _epCount(0), crtImagesLoaded(false), epLoaded(false)
{

}

void Anime::setAirDate(const QString &newAirDate)
{
    _airDate = newAirDate;
}

void Anime::setStaffs(const QVector<QPair<QString, QString> > &staffs)
{
    staff = staffs;
}

void Anime::setDesc(const QString &desc)
{
    _desc = desc;
}

void Anime::setEpCount(int epCount)
{
    _epCount = epCount;
}

void Anime::setCover(const QByteArray &data, bool updateDB, const QString &coverURL)
{
    if(updateDB)
        AnimeWorker::instance()->updateCoverImage(_name, data, coverURL);
    _coverData = data;
    coverCache.remove(this);
    rawCoverCache.remove(this);
    if(coverURL != emptyCoverURL) _coverURL = coverURL;
}

void Anime::setCrtImage(const QString &name, const QByteArray &data)
{
    AnimeWorker::instance()->updateCrtImage(_name, name, data);
    if(crtImagesLoaded)
    {
        for(auto &crt : characters)
        {
            if(crt.name == name)
            {
                if(data.isEmpty()) crt.image = QPixmap();
                else crt.image.loadFromData(data);
                break;
            }
        }
    }
}

void Anime::addCharacter(const Character &newCrtInfo)
{
    AnimeWorker::instance()->addCharacter(_name, newCrtInfo);
    Character crt;
    crt.name = newCrtInfo.name;
    crt.actor = newCrtInfo.actor;
    crt.link = newCrtInfo.link;
    characters.append(crt);
}

void Anime::removeCharacter(const QString &name)
{
    AnimeWorker::instance()->removeCharacter(_name, name);
    auto pos = std::remove_if(characters.begin(), characters.end(), [=](const Character &crt){
        return crt.name == name;
    });
    characters.erase(pos, characters.end());
}

void Anime::modifyCharacterInfo(const QString &srcName, const Character &newCrtInfo)
{
    for(auto &crt : characters)
    {
        if(crt.name == srcName)
        {
            AnimeWorker::instance()->modifyCharacter(_name, crt.name, newCrtInfo);
            crt.name = newCrtInfo.name;
            crt.actor = newCrtInfo.actor;
            crt.link = newCrtInfo.link;
            break;
        }
    }
}

void Anime::assign(const Anime *anime)
{
    _desc = anime->_desc;
    _url = anime->_url;
    _airDate = anime->_airDate;
    _coverURL = anime->_coverURL;
    _coverData = anime->_coverData;
    _scriptId = anime->_scriptId;
    _scriptData = anime->_scriptData;
    _epCount = anime->_epCount;
    staff = anime->staff;
    characters = anime->characters;
    crtImagesLoaded = anime->crtImagesLoaded;
    coverCache.remove(this);
    rawCoverCache.remove(this);
}

void Anime::setStaffs(const QString &staffStrs)
{
    staff.clear();
    QStringList staffs(staffStrs.split(';', Qt::SkipEmptyParts));
    for(int i=0;i<staffs.count();++i)
    {
        int pos=staffs.at(i).indexOf(':');
        staff.append(QPair<QString,QString>(staffs[i].left(pos),staffs[i].mid(pos+1)));
    }
}

QString Anime::staffToStr() const
{
    return staffListToStr(staff);
}

QString Anime::staffListToStr(const QVector<QPair<QString, QString>> &staffs)
{
    QStringList staffStrList;
    for(const auto &p : staffs)
        staffStrList.append(p.first+":"+p.second);
    return staffStrList.join(';');
}

const QVector<EpInfo> &Anime::epList()
{
    if(!epLoaded)
    {
        epInfoList.clear();
        epLoaded = AnimeWorker::instance()->loadEpInfo(this);
    }
    return epInfoList;
}

const QVector<Character> &Anime::crList(bool loadImage)
{
    if(!crtImagesLoaded && loadImage)
    {
        AnimeWorker::instance()->loadCrImages(this);
        crtImagesLoaded = true;
    }
    return characters;
}

void Anime::addEp(const EpInfo &ep)
{
    if(!epLoaded || ep.localFile.isEmpty()) return;
    for(auto &e : epInfoList)
    {
        if(ep.localFile==e.localFile)
        {
            e.index = ep.index;
            e.type = ep.type;
            e.name = ep.name;
            return;
        }
    }
    int pos = 0;
    while(pos<epInfoList.size() && epInfoList[pos]<ep) ++pos;
    epInfoList.insert(pos, ep);
}

void Anime::updateEpTime(const QString &path, qint64 time, bool isFinished)
{
    if(!epLoaded) return;
    for(auto &e : epInfoList)
    {
        if(e.localFile==path)
        {
            if(isFinished) e.finishTime = time;
            else e.lastPlayTime = time;
            return;
        }
    }
}

void Anime::updateEpInfo(const QString &path, const EpInfo &nInfo)
{
    if(!epLoaded) return;
    for(auto &e : epInfoList)
    {
        if(e.localFile==path)
        {
            e.name = nInfo.name;
            e.type = nInfo.type;
            e.index = nInfo.index;
            return;
        }
    }
}

void Anime::updateEpPath(const QString &path, const QString &nPath)
{
    if(!epLoaded) return;
    for(auto &e : epInfoList)
    {
        if(e.localFile==path)
        {
            e.localFile = nPath;
            return;
        }
    }
}

void Anime::removeEp(const QString &epPath)
{
    if(!epLoaded) return;
    for(auto iter=epInfoList.begin(); iter!=epInfoList.end();)
    {
        if(iter->localFile == epPath)
        {
            iter = epInfoList.erase(iter);
        } else {
            ++iter;
        }
    }
}

void Anime::removeEp(EpType type, double index)
{
    if(!epLoaded) return;
    for(auto iter=epInfoList.begin(); iter!=epInfoList.end();)
    {
        if(iter->type == type && iter->index == index)
        {
            iter = epInfoList.erase(iter);
        } else {
            ++iter;
        }
    }
}

QVariantMap Anime::toMap(bool fillEp)
{
    QVariantList eps, crts;
    if(fillEp)
    {
        for(const auto &ep : epList())
            eps.append(ep.toMap());
    }
    for(const auto &c : qAsConst(characters))
        crts.append(c.toMap());
    QVariantMap staffMap;
    for(const auto &p : qAsConst(staff))
        staffMap.insert(p.first, p.second);
    return  {
        {"name", _name},
        {"desc", _desc},
        {"url", _url},
        {"coverUrl", _coverURL},
        {"scriptId", _scriptId},
        {"data", _scriptData},
        {"airDate", _airDate},
        {"epCount", _epCount},
        {"addTime", QString::number(_addTime)},
        {"crt", crts},
        {"staff", staffMap},
        {"eps", eps}
    };
}

const AnimeLite Anime::toLite() const
{
    AnimeLite lite;
    lite.name = _name;
    lite.scriptId = _scriptId;
    lite.scriptData = _scriptData;
    if(epLoaded) lite.epList.reset(new QVector<EpInfo>(epInfoList));
    return lite;
}

Anime *Anime::fromMap(const QVariantMap &aobj)
{
    const QString name = aobj.value("name").toString(),
            airdate=aobj.value("airDate").toString();
    if (name.isEmpty() || airdate.isEmpty()) return nullptr;
    Anime *anime = new Anime;
    anime->_name = name;
    anime->_desc = aobj.value("desc").toString();
    anime->_url = aobj.value("url").toString();
    anime->_coverURL = aobj.value("coverUrl").toString();
    anime->_scriptId = aobj.value("scriptId").toString();
    anime->_scriptData = aobj.value("data").toString();
    anime->_airDate = airdate;
    anime->_epCount = aobj.value("epCount", 0).toInt();
    if (aobj.contains("staff"))
    {
        QString staffstr = aobj.value("staff").toString();
        auto strs = staffstr.split(';', Qt::SkipEmptyParts);
        for (const auto &s : qAsConst(strs))
        {
            int pos = s.indexOf(':');
            anime->staff.append({s.mid(0, pos), s.mid(pos+1)});
        }
    }
    if (aobj.contains("crt") && aobj.value("crt").type() == QVariant::List)
    {
        auto crts = aobj.value("crt").toList();
        for (auto &c : crts)
        {
            if (c.type()!=QVariant::Map) continue;
            auto cobj = c.toMap();
            QString cname = cobj.value("name").toString();
            if(cname.isEmpty()) continue;
            Character crt;
            crt.name = cname;
            crt.actor = cobj.value("actor").toString();
            crt.link = cobj.value("link").toString();
            crt.imgURL = cobj.value("imgurl").toString();
            anime->characters.append(crt);
        }
    }
    return anime;
}

Anime *AnimeLite::toAnime() const
{
    Anime *anime = new Anime;
    anime->_name = name;
    anime->_scriptId = scriptId;
    anime->_scriptData = scriptData;
    if(epList)
    {
        anime->epInfoList = *epList;
        anime->epLoaded = true;
    }
    return anime;
}

bool operator==(const EpInfo &ep1, const EpInfo &ep2)
{
    return ep1.type==ep2.type && ep1.index==ep2.index;
}

bool operator!=(const EpInfo &ep1, const EpInfo &ep2)
{
    return !(ep1==ep2);
}

void Character::scale(QPixmap &img)
{
    if(img.isNull()) return;
    const int maxSize = 600;
    int w = qMin(qMin(img.width(), img.height()), maxSize);
    img = img.scaled(w, w, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

void AnimeImage::setRoundedThumb(const QImage &src)
{
    QPixmap dest(thumbW * GlobalObjects::context()->devicePixelRatioF, thumbH * GlobalObjects::context()->devicePixelRatioF);
    dest.setDevicePixelRatio(GlobalObjects::context()->devicePixelRatioF);
    dest.fill(Qt::transparent);
    QPainter painter(&dest);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
    QPainterPath path;
    path.addRoundedRect(0, 0, thumbW, thumbH, 8, 8);
    painter.setClipPath(path);
    painter.drawImage(QRect(0, 0, thumbW, thumbH), src);

    thumb = dest;
}

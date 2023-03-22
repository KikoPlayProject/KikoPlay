#include "dlnamediaitem.h"
#include <QXmlStreamWriter>
#include <QMimeDatabase>
#include <QCryptographicHash>

namespace
{
static QMap<QString, QString> dlnaExtMap = {
    {"audio/L16",      "DLNA.ORG_PN=LPCM;DLNA.ORG_OP=01;DLNA.ORG_CI=1;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"audio/L16;rate=44100;channels=2", "DLNA.ORG_PN=LPCM;DLNA.ORG_OP=01;DLNA.ORG_CI=1;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"audio/L16;rate=44100;channels=1", "DLNA.ORG_PN=LPCM;DLNA.ORG_OP=01;DLNA.ORG_CI=1;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"audio/L16;rate=32000;channels=1", "DLNA.ORG_PN=LPCM;DLNA.ORG_OP=01;DLNA.ORG_CI=1;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"audio/mpeg",     "DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"audio/mp4",      "DLNA.ORG_PN=AAC_ISO;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"audio/x-ms-wma", "DLNA.ORG_PN=WMABASE;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"audio/wav",      "DLNA.ORG_OP=01;DLNA.ORG_CI=1;DLNA.ORG_FLAGS=01500000000000000000000000000000"}, // UVerse
    {"audio/x-wav",    "DLNA.ORG_OP=01;DLNA.ORG_CI=1;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"video/avi",      "DLNA.ORG_PN=AVI;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"video/mp4",      "DLNA.ORG_PN=MPEG4_P2_SP_AAC;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"video/mpeg",     "DLNA.ORG_PN=MPEG_PS_PAL;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"video/quicktime","DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"video/x-ms-wmv", "DLNA.ORG_PN=WMVHIGH_BASE;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"video/x-msvideo","DLNA.ORG_PN=AVI;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"video/x-ms-asf", "DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"},
    {"video/x-matroska","DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"video/x-flv",    "DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
};

#define MASK_CREATOR           0x00000001
#define MASK_ARTIST            0x00000002
#define MASK_ALBUM             0x00000004
#define MASK_GENRE             0x00000008
#define MASK_ALBUMARTURI       0x00000010
#define MASK_DESC              0x00000020
#define MASK_SEARCHABLE        0x00000040
#define MASK_CHILDCOUNT        0x00000080
#define MASK_ORITRACKNUM       0x00000100
#define MASK_ACTOR             0x00000200
#define MASK_AUTHOR            0x00000400
#define MASK_DATE              0x00000800
#define MASK_PROGRAMTITLE      0x00001000
#define MASK_SERIESTITLE       0x00002000
#define MASK_EPNUM             0x00004000
#define MASK_TITLE             0x00008000
#define MASK_RES               0x00010000
#define MASK_RES_DURATION      0x00020000
#define MASK_DURATION          0x00020000
#define MASK_RES_SIZE          0x00040000
#define MASK_RES_PROTECTION    0x00080000
#define MASK_RES_RESOLUTION    0x00100000
#define MASK_RES_BITRATE       0x00200000
#define MASK_RES_BITPERSAMPLE  0x00400000
#define MASK_RES_AUDIOCHANN    0x00800000
#define MASK_RES_SAMPLEFREQ    0x01000000

static QHash<QString, quint32> filterMask = {
    {"dc:creator",               MASK_CREATOR},
    {"upnp:artist",              MASK_ARTIST},
    {"upnp:album",               MASK_ALBUM},
    {"upnp:genre",               MASK_GENRE},
    {"upnp:albumArtURI",         MASK_ALBUMARTURI},
    {"dc:description",           MASK_DESC},
    {"@searchable",              MASK_SEARCHABLE},
    {"@childcount",              MASK_CHILDCOUNT},
    {"upnp:originalTrackNumber", MASK_ORITRACKNUM},
    {"upnp:actor",               MASK_ACTOR},
    {"upnp:author",              MASK_AUTHOR},
    {"dc:date",                  MASK_DATE},
    {"upnp:programTitle",        MASK_PROGRAMTITLE},
    {"upnp:seriesTitle",         MASK_SERIESTITLE},
    {"upnp:episodeNumber",       MASK_EPNUM},
    {"dc:title",                 MASK_TITLE},
    {"res",                      MASK_RES},
    {"res@duration",             MASK_RES_DURATION},
    {"@duration",                MASK_DURATION},
    {"res@size",                 MASK_RES_SIZE},
    {"res@protection",           MASK_RES_PROTECTION},
    {"res@resolution",           MASK_RES_RESOLUTION},
    {"res@bitrate",              MASK_RES_BITRATE},
    {"res@bitsPerSample",        MASK_RES_BITPERSAMPLE},
    {"res@nrAudioChannels",      MASK_RES_AUDIOCHANN},
    {"res@sampleFrequency",      MASK_RES_SAMPLEFREQ},
    {"*",                        0xFFFFFFFF}
};
}
void DLNAMediaItem::toDidl(QXmlStreamWriter &writer, quint32 mask) const
{
    writer.writeStartElement(type == Container? "container" : "item");
    writer.writeAttribute("id", objID);
    writer.writeAttribute("parentID", parentID);
    writer.writeAttribute("restricted", "1");
    if(type == Container)
    {
        if(mask & MASK_SEARCHABLE) writer.writeAttribute("searchable", "0");
        if(mask & MASK_CHILDCOUNT) writer.writeAttribute("childCount", QString(childSize));
    }
    writer.writeTextElement("http://purl.org/dc/elements/1.1/", "title", title);
    if(mask & MASK_CREATOR) writer.writeTextElement("http://purl.org/dc/elements/1.1/", "creator", "Unknown");
    if(mask & MASK_DATE) writer.writeTextElement("http://purl.org/dc/elements/1.1/", "date", "");
    if(mask & MASK_GENRE) writer.writeTextElement("urn:schemas-upnp-org:metadata-1-0/upnp/", "genre", "Unknown");

    if(type == Item)
    {
        if(mask & MASK_RES)
        {
            writer.writeStartElement("res");
            if(mask & MASK_RES_SIZE) writer.writeAttribute("size", QString::number(fileSize));
            writer.writeAttribute("protocolInfo", protocolInfo);
            writer.writeCharacters(url);
            writer.writeEndElement();
        }
    }
    writer.writeTextElement("urn:schemas-upnp-org:metadata-1-0/upnp/", "class",
                            type == Item ? "object.item.videoItem" : "object.container.playlistContainer");
    writer.writeEndElement();
}

void DLNAMediaItem::setResource(const QString &path, const QHostAddress &localAddress, quint16 port)
{
    static QMimeDatabase database;
    QString mimeName = database.mimeTypeForFile(path).name();
    protocolInfo = QString("http-get:*:%1:%2").arg(mimeName, dlnaExtMap.value(mimeName, "*"));
    QString pathHash(QCryptographicHash::hash(path.toUtf8(),QCryptographicHash::Md5).toHex());
    QString suffix = path.mid(path.lastIndexOf('.') + 1);
    url = QString("http://%1:%2/media/%3.%4").arg(localAddress.toString(), QString::number(port), pathHash, suffix);
}

quint32 DLNAMediaItem::getMask(const QString &filter)
{
    quint32 mask = 0x00008000;  //title
    QStringList filters = filter.split(',');
    for(const QString &f : filters)
    {
        mask |= filterMask.value(f, 0);
    }
    return mask;
}

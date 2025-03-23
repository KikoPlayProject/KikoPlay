#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H
#include <QImage>
#include <QObject>

class PlayList;
class QXmlStreamReader;
class QXmlStreamWriter;
struct ItemTrackInfo
{
    int subDelay = 0;
    int subIndex = -1;
    int audioIndex = -1;
    QStringList subFiles;
    QStringList audioFiles;
};
struct WebDAVInfo
{
    QString user;
    QString password;
};

struct PlayListItem
{
    PlayListItem(PlayListItem *p = nullptr, bool leaf = false, int insertPosition = -1);
    ~PlayListItem();

    static PlayListItem *parseCollection(QXmlStreamReader &reader, PlayListItem *parent = nullptr);
    static void writeCollection(QXmlStreamWriter &writer, PlayListItem *item);
    static PlayListItem *parseItem(QXmlStreamReader &reader, PlayListItem *parent = nullptr);
    static void writeItem(QXmlStreamWriter &writer, PlayListItem *item);

    inline bool isCollection() const { return children; }
    inline bool isWebDAVCollection() const { return children && webDAVInfo; }
    bool hasPool() const;
    void setLevel(int newLevel);
    void moveTo(PlayListItem *newParent, int insertPosition = -1);

    static PlayList *playlist;

    PlayListItem *parent;
    QVector<PlayListItem *> *children;
    enum ItemType
    {
        LOCAL_FILE, WEB_URL, COLLECTION
    };
    enum PlayState
    {
        UNPLAY, UNFINISH, FINISH
    };
    enum Marker
    {
        M_RED, M_BLUE, M_GREEN, M_ORANGE, M_PINK, M_YELLOW, M_NONE
    };

    ItemType type;
    PlayState playTimeState;
    Marker marker;
    int playTime;
    int duration;
    short level;
    bool isBgmCollection;

    qint64 addTime;

    QString title;
    QString animeTitle;
    QString path;
    QString poolID;
    QString pathHash;
    ItemTrackInfo *trackInfo;
    WebDAVInfo *webDAVInfo;
};

struct RecentlyPlayedItem
{
    QString path;
    QString title;
    PlayListItem::PlayState playTimeState;
    int duration;
    int playtime;
    QImage stopFrame;
};

#endif // PLAYLISTITEM_H

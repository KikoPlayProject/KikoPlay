#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H
#include <QObject>

class PlayList;
struct ItemTrackInfo
{
    int subDelay = 0;
    int subIndex = -1;
    int audioIndex = -1;
    QStringList subFiles;
    QStringList audioFiles;
};

struct PlayListItem
{
    PlayListItem(PlayListItem *p = nullptr, bool leaf = false, int insertPosition = -1);
    ~PlayListItem();

    inline bool isCollection() const { return children; }
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
    short level;
    bool isBgmCollection;

    qint64 addTime;

    QString title;
    QString animeTitle;
    QString path;
    QString poolID;
    QString pathHash;
    ItemTrackInfo *trackInfo;
};

#endif // PLAYLISTITEM_H
